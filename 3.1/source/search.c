/*******************************************************************************
*									       *
* search.c -- Nirvana Editor search and replace functions		       *
*									       *
* Copyright (c) 1991 Universities Research Association, Inc.		       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retains a paid-up,      *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warrenty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* May 10, 1991								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)search.c	1.23     9/23/94";
#include <stdio.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <X11/Shell.h>
#include <Xm/XmP.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <X11/Xatom.h>		/* for getting selection */
#include <X11/keysym.h>
#ifdef MOTIF10
#include <X11/Selection.h>	/* " " */
#endif
#include <X11/X.h>		/* " " */
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "regularExp.h"
#include "nedit.h"
#include "search.h"
#include "window.h" 
#include "preferences.h"

/*
** Module Global Variables
*/
/* the current search string */
static char TheSearchString[SEARCHMAX] = "";
/* current replace-with string */
static char TheReplaceString[SEARCHMAX] = "";
/* current selected search type, literal, case sensitive, or regular exp. */
static int TheSearchType = SEARCH_LITERAL;
/* sleeze for passing search direction to callback routines */
static int SearchDirection;
/* history mechanism for search and replace strings */
static char *SearchHistory[MAX_SEARCH_HISTORY];
static char *ReplaceHistory[MAX_SEARCH_HISTORY];
static int SearchTypeHistory[MAX_SEARCH_HISTORY];
static int HistStart = 0;
static int NHist = 0;

static void createReplaceDlog(Widget parent, WindowInfo *window);
static void createFindDlog(Widget parent, WindowInfo *window);
static void fFocusCB(Widget w, WindowInfo *window, caddr_t *callData);
static void rFocusCB(Widget w, WindowInfo *window, caddr_t *callData);
static void replaceCB(Widget w, WindowInfo *window,
		XmAnyCallbackStruct *callData); 
static void replaceAllCB(Widget w, WindowInfo *window,
		XmAnyCallbackStruct *callData);
static void rInSelCB(Widget w, WindowInfo *window,
		XmAnyCallbackStruct *callData); 
static void rCancelCB(Widget w, WindowInfo *window, caddr_t callData);
static void fCancelCB(Widget w, WindowInfo *window, caddr_t callData);
static void rFindCB(Widget w,WindowInfo *window,XmAnyCallbackStruct *callData);
static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void replaceArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void findArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData); 
static void flashTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void eraseFlash(WindowInfo *window);
static int storeReplaceDlogInfo(WindowInfo *window);
static int storeFindDlogInfo(WindowInfo *window);
static void selectedSearchCB(Widget w, WindowInfo *window, Atom *selection,
			     Atom *type, char *value, int *length, int *format);
static int searchLiteral(char *string, char *searchString, int caseSense, 
	      int direction, int wrap, int beginPos, int *startPos, int *endPos);
static int searchRegex(char *string, char *searchString, int direction,
		int wrap, int beginPos, int *startPos, int *endPos);
static int forwardRegexSearch(char *string, char *searchString,
		int wrap, int beginPos, int *startPos, int *endPos);
static int backwardRegexSearch(char *string, char *searchString,
		int wrap, int beginPos, int *startPos, int *endPos);
static void upCaseString(char *outString, char *inString);
static void downCaseString(char *outString, char *inString);
static void resetFindTabGroup(WindowInfo *window);
static void resetReplaceTabGroup(WindowInfo *window);
static int searchMatchesSelection(WindowInfo *window);
static int findMatchingChar(char *string, char c, int charPos, int *matchPos);
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen);
static void saveSearchHistory(void);
static int historyIndex(int nCycles);

typedef struct _charMatchTable {
    char c;
    char match;
    char direction;
} charMatchTable;

#define N_MATCH_CHARS 13
#define N_FLASH_CHARS 6
static charMatchTable MatchingChars[N_MATCH_CHARS] = {
    {'{', '}', SEARCH_FORWARD},
    {'}', '{', SEARCH_BACKWARD},
    {'(', ')', SEARCH_FORWARD},
    {')', '(', SEARCH_BACKWARD},
    {'[', ']', SEARCH_FORWARD},
    {']', '[', SEARCH_BACKWARD},
    {'<', '>', SEARCH_FORWARD},
    {'>', '<', SEARCH_BACKWARD},
    {'/', '/', SEARCH_FORWARD},
    {'"', '"', SEARCH_FORWARD},
    {'\'', '\'', SEARCH_FORWARD},
    {'`', '`', SEARCH_FORWARD},
    {'\\', '\\', SEARCH_FORWARD},
};
    
void DoReplaceDlog(WindowInfo *window, int direction)
{
    Widget button;
    int selStart, selEnd;

    /* Create the dialog if it doesn't already exist */
    if (window->replaceDlog == NULL)
    	createReplaceDlog(window->shell, window);
    
    /* Set the initial search type based on user preference */
    TheSearchType = GetPrefSearch();
    
    /* Set the buttons with the selected search type */
    switch (TheSearchType) {
      case SEARCH_LITERAL:
      	button = window->replaceLiteralBtn;
	break;
      case SEARCH_CASE_SENSE:
      	button = window->replaceCaseBtn;
	break;
      case SEARCH_REGEX:
      	button = window->replaceRegExpBtn;
	break;
    }
    XmToggleButtonSetState(button, True, True);
    
    /* Blank the text fields */
    XmTextSetString(window->replaceText, "");
    XmTextSetString(window->replaceWithText, "");
    
    /* Dim the "Replace in Selection" button if there is no selection	*/
    XtSetSensitive(window->replaceInSelBtn,
		   GetSelection(window->textArea, &selStart, &selEnd));
		   
    /* Pass the search direction to the callback routines whose single	*/
    /* parameter is already used up with the window structure.  We use	*/
    /* this global variable, so as not to muck up the window structure	*/
    SearchDirection = direction;
    
    /* start the search history mechanism at the current history item */
    window->rHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->replaceDlog);
    while (XtIsManaged(window->replaceDlog))
	XtAppProcessEvent(XtWidgetToApplicationContext(window->replaceDlog),
		XtIMAll);
}

void DoFindDlog(WindowInfo *window, int direction)
{
    Widget button;

    /* Create the dialog if it doesn't already exist */
    if (window->findDlog == NULL)
    	createFindDlog(window->shell, window);
    
    /* Set the initial search type based on user preference */
    TheSearchType = GetPrefSearch();
    
    /* Set the buttons with the currently selected search type */
    switch (TheSearchType) {
      case SEARCH_LITERAL:
      	button = window->findLiteralBtn;
	break;
      case SEARCH_CASE_SENSE:
      	button = window->findCaseBtn;
	break;
      case SEARCH_REGEX:
      	button = window->findRegExpBtn;
	break;
    }
    XmToggleButtonSetState(button, True, True);
    
    /* Blank the text field */
    XmTextSetString(window->findText, "");

    /* Pass the search direction to the callback routines whose single
       parameter is already used up with the window structure.  We use
       this global variable, so as not to muck up the window structure */
    SearchDirection = direction;
    
    /* start the search history mechanism at the current history item */
    window->fHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->findDlog);
    while (XtIsManaged(window->findDlog))
	XtAppProcessEvent(XtWidgetToApplicationContext(window->findDlog),
		XtIMAll);
}

static void createReplaceDlog(Widget parent, WindowInfo *window)
{
    Arg    	args[50];
    int    	argcnt, defaultBtnOffset;
    XmString	st1;
    Widget	form, btnForm;
    Widget	searchTypeBox, literalBtn, caseBtn, regExpBtn;
    Widget	label2, label1, label, replaceText, findText;
    Widget	findBtn, replaceAllBtn, rInSelBtn, cancelBtn, replaceBtn;
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    XtSetArg(args[argcnt], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    	    argcnt++;
    form = XmCreateFormDialog(parent, "replaceDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, 0);
    SET_ONE_RSRC(XtParent(form), XmNtitle, "Replace");
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("String to Find:"));
    	    argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 't'); argcnt++;
    label1 = XmCreateLabel(form, "label1", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label1);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_END); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING(
    	   "(use up arrow key to recall previous)")); argcnt++;
    label2 = XmCreateLabel(form, "label2", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label2);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label1); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNmaxLength, SEARCHMAX); argcnt++;
    findText = XmCreateText(form, "replaceString", args, argcnt);
    XtAddCallback(findText, XmNfocusCallback, (XtCallbackProc)rFocusCB, window);
    XtAddEventHandler(findText, KeyPressMask, False,
    	    (XtEventHandler)rFindArrowKeyCB, window);
    RemapDeleteKey(findText);
    XtManageChild(findText);
    XmAddTabGroup(findText);
    XtVaSetValues(label1, XmNuserData, findText, 0); /* mnemonic processing */
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, findText); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Replace With:")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'W'); argcnt++;
    label = XmCreateLabel(form, "label", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNmaxLength, SEARCHMAX); argcnt++;
    replaceText = XmCreateText(form, "replaceWithString", args, argcnt);
    XtAddEventHandler(replaceText, KeyPressMask, False,
    	    (XtEventHandler)replaceArrowKeyCB, window);
    RemapDeleteKey(replaceText);
    XtManageChild(replaceText);
    XmAddTabGroup(replaceText);
    XtVaSetValues(label, XmNuserData, replaceText, 0); /* mnemonic processing */

    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, replaceText); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchTypeBox = XmCreateRowColumn(form, "searchTypeBox", args, argcnt);
    XtManageChild(searchTypeBox);
    XmAddTabGroup(searchTypeBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'L'); argcnt++;
    literalBtn = XmCreateToggleButton(searchTypeBox, "literal", args, argcnt);
    XmStringFree(st1);
    XtManageChild(literalBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Case Sensitive Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    caseBtn = XmCreateToggleButton(searchTypeBox, "caseSenseLiteral", args, argcnt);
    XmStringFree(st1);
    XtManageChild(caseBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Regular Expression")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    regExpBtn = XmCreateToggleButton(searchTypeBox, "regExp", args, argcnt);
    XmStringFree(st1);
    XtManageChild(regExpBtn);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    btnForm = XmCreateForm(form, "buttons", args, argcnt);
    XtManageChild(btnForm);
    XmAddTabGroup(btnForm);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace")); argcnt++;
    XtSetArg(args[argcnt], XmNshowAsDefault, (short)1); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 0); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 21); argcnt++;
    replaceBtn = XmCreatePushButton(btnForm, "replace", args, argcnt);
    XtAddCallback(replaceBtn, XmNactivateCallback, (XtCallbackProc)replaceCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(replaceBtn);
    XtVaGetValues(replaceBtn, XmNshadowThickness, &shadowThickness, 0);
    defaultBtnOffset = shadowThickness + 4;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Replace All")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'A'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 21); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 44); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    replaceAllBtn = XmCreatePushButton(btnForm, "all", args, argcnt);
    XtAddCallback(replaceAllBtn, XmNactivateCallback,
    	    (XtCallbackProc)replaceAllCB, window);
    XmStringFree(st1);
    XtManageChild(replaceAllBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("R. In Selection")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 44); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 73); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    rInSelBtn = XmCreatePushButton(btnForm, "inSel", args, argcnt);
    XtAddCallback(rInSelBtn, XmNactivateCallback, (XtCallbackProc)rInSelCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(rInSelBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Find")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'F'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 73); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 85); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    findBtn = XmCreatePushButton(btnForm, "find", args, argcnt);
    XtAddCallback(findBtn, XmNactivateCallback, (XtCallbackProc)rFindCB,window);
    XmStringFree(st1);
    XtManageChild(findBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Cancel")); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 85); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 100); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    cancelBtn = XmCreatePushButton(btnForm, "cancel", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(cancelBtn, XmNactivateCallback, (XtCallbackProc)rCancelCB,
    	    window);
    XtManageChild(cancelBtn);

    XtVaSetValues(form, XmNcancelButton, cancelBtn, 0);
    AddDialogMnemonicHandler(form);
    
    window->replaceDlog = form;
    window->replaceText = findText;
    window->replaceWithText = replaceText;
    window->replaceLiteralBtn = literalBtn;
    window->replaceCaseBtn = caseBtn;
    window->replaceRegExpBtn = regExpBtn;
    window->replaceBtns = btnForm;
    window->replaceBtn = replaceBtn;
    window->replaceInSelBtn = rInSelBtn;
    window->replaceSearchTypeBox = searchTypeBox;
}

static void createFindDlog(Widget parent, WindowInfo *window)
{
    Arg    	args[50];
    int    	argcnt, defaultBtnOffset;
    XmString	st1;
    Widget	form, btnForm, searchTypeBox, literalBtn, caseBtn, regExpBtn;
    Widget	findText, label1, label2, cancelBtn, findBtn;
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    XtSetArg(args[argcnt], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    	    argcnt++;
    form = XmCreateFormDialog(parent, "findDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, 0);
    SET_ONE_RSRC(XtParent(form), XmNtitle, "Find");
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("String to Find:"));
    	    argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    label1 = XmCreateLabel(form, "label1", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label1);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_END); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING(
    	   "(use up arrow key to recall previous)")); argcnt++;
    label2 = XmCreateLabel(form, "label2", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label2);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label1); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNmaxLength, SEARCHMAX); argcnt++;
    findText = XmCreateText(form, "searchString", args, argcnt);
    XtAddCallback(findText, XmNfocusCallback, (XtCallbackProc)fFocusCB, window);
    XtAddEventHandler(findText, KeyPressMask, False,
    	    (XtEventHandler)findArrowKeyCB, window);
    RemapDeleteKey(findText);
    XtManageChild(findText);
    XmAddTabGroup(findText);
    XtVaSetValues(label1, XmNuserData, findText, 0); /* mnemonic processing */

    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, findText); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchTypeBox = XmCreateRowColumn(form, "searchTypeBox", args, argcnt);
    XtManageChild(searchTypeBox);
    XmAddTabGroup(searchTypeBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'L'); argcnt++;
    literalBtn = XmCreateToggleButton(searchTypeBox, "literal", args, argcnt);
    XmStringFree(st1);
    XtManageChild(literalBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, 
    	    st1=MKSTRING("Case Sensitive Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    caseBtn = XmCreateToggleButton(searchTypeBox, "caseSenseLiteral", args, argcnt);
    XmStringFree(st1);
    XtManageChild(caseBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, 
    	     st1=MKSTRING("Regular Expression")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    regExpBtn = XmCreateToggleButton(searchTypeBox, "regExp", args, argcnt);
    XmStringFree(st1);
    XtManageChild(regExpBtn);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    btnForm = XmCreateForm(form, "buttons", args, argcnt);
    XtManageChild(btnForm);
    XmAddTabGroup(btnForm);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Find")); argcnt++;
    XtSetArg(args[argcnt], XmNshowAsDefault, (short)1); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 20); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    findBtn = XmCreatePushButton(btnForm, "find", args, argcnt);
    XtAddCallback(findBtn, XmNactivateCallback, (XtCallbackProc)findCB, window);
    XmStringFree(st1);
    XtManageChild(findBtn);
    XtVaGetValues(findBtn, XmNshadowThickness, &shadowThickness, 0);
    defaultBtnOffset = shadowThickness + 4;

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Cancel")); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 80); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    cancelBtn = XmCreatePushButton(btnForm, "cancel", args, argcnt);
    XtAddCallback(cancelBtn, XmNactivateCallback, (XtCallbackProc)fCancelCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(cancelBtn);
    XtVaSetValues(form, XmNcancelButton, cancelBtn, 0);
    AddDialogMnemonicHandler(form);
    
    window->findDlog = form;
    window->findText = findText;
    window->findLiteralBtn = literalBtn;
    window->findCaseBtn = caseBtn;
    window->findRegExpBtn = regExpBtn;
    window->findBtns = btnForm;
    window->findBtn = findBtn;
    window->findSearchTypeBox = searchTypeBox;
}

/*
** These callbacks fix a Motif 1.1 problem that the default button gets the
** keyboard focus when a dialog is created.  We want the first text field
** to get the focus, so we don't set the default button until the text field
** has the focus for sure.  I have tried many other ways and this is by far
** the least nasty.
*/
static void fFocusCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    SET_ONE_RSRC(window->findDlog, XmNdefaultButton, window->findBtn);
}
static void rFocusCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    SET_ONE_RSRC(window->replaceDlog, XmNdefaultButton, window->replaceBtn);
}

static void replaceCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData) 
{
    /* validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!storeReplaceDlogInfo(window))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    /* find the text and mark it */
    SearchAndReplace(window, SearchDirection);
    
    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void replaceAllCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    /* validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!storeReplaceDlogInfo(window))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* do replacement based on strings stored in window structure */
    ReplaceAll(window);
    
    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void rInSelCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    /* validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!storeReplaceDlogInfo(window))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* do replacement based on strings stored in window structure */
    ReplaceInSelection(window);
    
    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void rCancelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void fCancelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    /* Set the initial focus of the dialog back to the search string	*/
    resetFindTabGroup(window);
    
    /* pop down the dialog */
    XtUnmanageChild(window->findDlog);
}

static void rFindCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    /* validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!storeReplaceDlogInfo(window))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);
    
    /* find the text and mark it */
    SearchAndSelect(window, SearchDirection, CurrentTime);

    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->rHistIndex;
    char *searchStr, *replaceStr;
    int searchType;
    Widget button;
    
    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down)
    	return;
    
    /* increment or decrement the index depending on which arrow was pressed */
    index += (keysym == XK_Up) ? 1 : -1;

    /* if the index is out of range, beep and return */
    if (index != 0 && historyIndex(index) == -1) {
    	XBell(TheDisplay, 100);
    	return;
    }
    
    /* determine the strings and button settings to use */
    if (index == 0) {
    	searchStr = "";
    	replaceStr = "";
    	searchType = GetPrefSearch();
    } else {
	searchStr = SearchHistory[historyIndex(index)];
	replaceStr = ReplaceHistory[historyIndex(index)];
	searchType = SearchTypeHistory[historyIndex(index)];
    }
    
    /* Set the buttons and fields with the selected search type */
    switch (searchType) {
      case SEARCH_LITERAL:
      	button = window->replaceLiteralBtn;
	break;
      case SEARCH_CASE_SENSE:
      	button = window->replaceCaseBtn;
	break;
      case SEARCH_REGEX:
      	button = window->replaceRegExpBtn;
	break;
    }
    XmToggleButtonSetState(button, True, True);
    XmTextSetString(window->replaceText, searchStr);
    XmTextSetString(window->replaceWithText, replaceStr);
    window->rHistIndex = index;
}

static void replaceArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->rHistIndex;
    
    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down)
    	return;
    
    /* increment or decrement the index depending on which arrow was pressed */
    index += (keysym == XK_Up) ? 1 : -1;

    /* if the index is out of range, beep and return */
    if (index != 0 && historyIndex(index) == -1) {
    	XBell(TheDisplay, 100);
    	return;
    }
    
    /* change only the replace field information */
    if (index == 0)
    	XmTextSetString(window->replaceWithText, "");
    else
    	XmTextSetString(window->replaceWithText,
    		ReplaceHistory[historyIndex(index)]);
    window->rHistIndex = index;
}

static void findArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->fHistIndex;
    char *searchStr;
    int searchType;
    Widget button;
    
    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down)
    	return;
    
    /* increment or decrement the index depending on which arrow was pressed */
    index += (keysym == XK_Up) ? 1 : -1;

    /* if the index is out of range, beep and return */
    if (index != 0 && historyIndex(index) == -1) {
    	XBell(TheDisplay, 100);
    	return;
    }
    
    /* determine the strings and button settings to use */
    if (index == 0) {
    	searchStr = "";
    	searchType = GetPrefSearch();
    } else {
	searchStr = SearchHistory[historyIndex(index)];
	searchType = SearchTypeHistory[historyIndex(index)];
    }
    
    /* Set the buttons and fields with the selected search type */
    switch (searchType) {
      case SEARCH_LITERAL:
      	button = window->findLiteralBtn;
	break;
      case SEARCH_CASE_SENSE:
      	button = window->findCaseBtn;
	break;
      case SEARCH_REGEX:
      	button = window->findRegExpBtn;
	break;
    }
    XmToggleButtonSetState(button, True, True);
    XmTextSetString(window->findText, searchStr);
    window->fHistIndex = index;
}

static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    /* save find string from the dialog in the global search string */
    if (!storeFindDlogInfo(window))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetFindTabGroup(window);
    
    /* find the text and mark it */
    SearchAndSelect(window, SearchDirection, CurrentTime);    

    /* pop down the dialog */
    XtUnmanageChild(window->findDlog);
}

/*
** Save search and replace strings and search type from the Replace dialog
*/
static int storeReplaceDlogInfo(WindowInfo *window)
{
    char *replaceText, *replaceWithText;
    int searchType;
    regexp *compiledRE = NULL;
    char *compileMsg;
    
    /* Get the search and replace strings and search type from the dialog */
    replaceText = XmTextGetString(window->replaceText);
    replaceWithText = XmTextGetString(window->replaceWithText);
    if (XmToggleButtonGetState(window->replaceLiteralBtn))
    	searchType = SEARCH_LITERAL;
    else if (XmToggleButtonGetState(window->replaceCaseBtn))
    	searchType = SEARCH_CASE_SENSE;
    else
    	searchType = SEARCH_REGEX;
    
    /* If the search type is a regular expression, test compile it immediately
       and present error messages */
    if (searchType == SEARCH_REGEX) {
	compiledRE = CompileRE(replaceText, &compileMsg);
	if (compiledRE == NULL) {
   	    DialogF(DF_WARN, XtParent(window->replaceDlog), 1,
   	    	   "Please respecify the search string:\n%s", "OK", compileMsg);
 	    return FALSE;
 	}
    }
    
    /* Store strings and search type for use by all of the search functions */
    strcpy(TheSearchString, replaceText);
    strcpy(TheReplaceString, replaceWithText);
    TheSearchType = searchType;
    saveSearchHistory();
    XtFree(replaceText);
    XtFree(replaceWithText);
    return TRUE;
}

/*
** Save search and find strings and search type from the Find dialog
*/
static int storeFindDlogInfo(WindowInfo *window)
{
    char *findText;
    int searchType;
    regexp *compiledRE = NULL;
    char *compileMsg;
    
    /* Get the search string and search type from the dialog */
    findText = XmTextGetString(window->findText);
    if (XmToggleButtonGetState(window->findLiteralBtn))
    	searchType = SEARCH_LITERAL;
    else if (XmToggleButtonGetState(window->findCaseBtn))
    	searchType = SEARCH_CASE_SENSE;
    else
    	searchType = SEARCH_REGEX;

    /* If the search type is a regular expression, test compile it immediately
       and present error messages */
    if (searchType == SEARCH_REGEX) {
	compiledRE = CompileRE(findText, &compileMsg);
	if (compiledRE == NULL) {
   	    DialogF(DF_WARN, XtParent(window->findDlog), 1,
   	    	   "Please respecify the search string:\n%s", "OK", compileMsg);
 	    return FALSE;
 	}
 	XtFree((char *)compiledRE);
    }

    /* Store search string and type for use by all of the search functions */
    strcpy(TheSearchString, findText);
    TheSearchType = searchType;
    saveSearchHistory();
    XtFree(findText);
    return TRUE;
}

int SearchAndSelect(WindowInfo *window, int direction, Time time)
{
    int startPos, endPos;
    int beginPos, cursorPos, selStart, selEnd;
        
    /* set the position to start the search so we don't find the same
       string that was found on the last search	*/
    if (searchMatchesSelection(window)) {
    	/* selection matches search string, start before or after sel.	*/
    	GetSelection(window->textArea, &selStart, &selEnd);
	if (direction == SEARCH_BACKWARD) {
	    beginPos = selStart-1;
	} else {
	    beginPos = selEnd;
	}
    } else {
    	selStart = -1; selEnd = -1;
    	/* no selection, or no match, search relative cursor */
    	cursorPos = XmTextGetInsertionPosition(window->lastFocus);
	if (direction == SEARCH_BACKWARD) {
	    /* use the insert position - 1 for backward searches */
	    beginPos = cursorPos-1;
	} else {
	    /* use the insert position for forward searches */
	    beginPos = cursorPos;
	}
    }

    /* do the search.  SearchWindow does appropriate dialogs and beeps */
    if (!SearchWindow(window, direction, beginPos, &startPos, &endPos))
    	return FALSE;
    	
    /* if the search matched an empty string (possible with regular exps)
       beginning at the start of the search, go to the next occurrence,
       otherwise repeated finds will get "stuck" at zero-length matches */
    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
    	if (!SearchWindow(window, direction, beginPos+1, &startPos, &endPos))
    	    return FALSE;
    
    /* if matched text is already selected, just beep */
    if (selStart==startPos && selEnd==endPos) {
    	XBell(TheDisplay, 100);
    	return FALSE;
    }

    /* select the text found string */
    XmTextSetSelection(window->lastFocus, startPos, endPos, time);
    MakeSelectionVisible(window, window->lastFocus);
    
    return TRUE;
}

void SearchForSelected(WindowInfo *window, int direction, Time time)
{
    SearchDirection = direction;
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)selectedSearchCB, window, time);
}

static void selectedSearchCB(Widget w, WindowInfo *window, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
    /* skip if we can't get the selection data or it's too long */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1,
   	    	    "Selection not appropriate for searching", "OK");
    	else
    	    XBell(TheDisplay, 100);
	return;
    }
    if (*length > SEARCHMAX) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1, "Selection too long", "OK");
    	else
    	    XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    if (*length == 0) {
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: can't handle non 8-bit text\n");
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* make the selection the current search string */
    strncpy(TheSearchString, value, *length);
    TheSearchString[*length] = '\0';
    XtFree(value);
    
    /* Use the default method for searching, unless it is regex, since this
       kind of search is by definition a literal search */
    TheSearchType = GetPrefSearch();
    if (TheSearchType == SEARCH_REGEX)
    	TheSearchType = SEARCH_LITERAL;

    /* record the search parameters for recall with up arrow */
    saveSearchHistory();

    /* search for it in the window */
    SearchAndSelect(window, SearchDirection, CurrentTime);
}

/*
** Check the character before the insertion cursor of textW and flash
** matching parenthesis, brackets, or braces, by temporarily highlighting
** the matching character (a timer procedure is scheduled for removing the
** highlights)
*/
void FlashMatching(WindowInfo *window, Widget textW, XmTextVerifyPtr callData,
	int isModifyVerify)
{
    char *charStr, c;
    XmTextPosition left, right, pos;
    char *findText, matchChar[2];
    XmTextPosition startPos, endPos, searchPos, matchPos;
    int matchIndex, matchOffset;
    Position x, y;
    
    /* if a marker is already drawn, erase it and cancel the timeout */
    if (window->flashTimeoutID != 0) {
    	eraseFlash(window);
    	XtRemoveTimeOut(window->flashTimeoutID);
    	window->flashTimeoutID = 0;
    }
    
    /* don't do anything if showMatching isn't on */
    if (!window->showMatching)
    	return;

    /* don't flash matching characters if there's a selection */
    if (XmTextGetSelectionPosition(textW, &left, &right) && left != right)
   	return;

    /* get the character to match and the position to start from */
    if (isModifyVerify) {
        if (callData->startPos!=callData->endPos || callData->text->length!=1)
            return;
        pos = callData->newInsert;
        c = *callData->text->ptr;
    } else {
	pos = callData->newInsert-1;
	charStr = GetTextRange(textW, pos, pos+1);
	c = *charStr;
	XtFree(charStr);
    }
    
    /* is the character one we want to flash? */
    for (matchIndex = 0; matchIndex<N_FLASH_CHARS; matchIndex++) {
        if (MatchingChars[matchIndex].c == c)
	    break;
    }
    if (matchIndex == N_FLASH_CHARS)
	return;

    /* Constrain the search to visible text and get the string to search */
    if (MatchingChars[matchIndex].direction == SEARCH_BACKWARD) {
    	startPos = XmTextGetTopCharacter(textW);
    	endPos = pos;
    	searchPos = endPos;
    } else {
    	startPos = pos;
    	endPos = XmTextGetLastPosition(textW); /*... wasteful */
    	searchPos = startPos;
    }
    findText = GetTextRange(textW, startPos, endPos+1);
    
    /* do the search */
    if (!findMatchingChar(findText, c, searchPos-startPos, &matchOffset)) {
    	XtFree(findText);
    	return;
    }
    XtFree(findText);
    matchPos = startPos + matchOffset;
    
    /* see if the matched character is visible in the window */
    if (!XmTextPosToXY(textW, matchPos, &x, &y))
    	return;

    /* highlight the matched character */
    XmTextSetHighlight(textW, matchPos, matchPos+1, XmHIGHLIGHT_SELECTED);
      
    /* Set up a timer to erase the box after 1.5 seconds */
    window->flashTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 1500,
    	    flashTimeoutProc, window);
    window->flashPane = textW;
    window->flashPos = matchPos;
}

void MatchSelectedCharacter(WindowInfo *window, Time time)
{
    char *findText;
    char *fileString;
    int selStart, selEnd;
    int startPos, endPos, matchPos;
    int found;

    /*
    ** get the character to match from the text widget selection, or
    ** the character before the insert point if nothing is selected
    ** give up if too many characters are selected
    */
    if (!GetSelection(window->textArea, &selStart, &selEnd)) {
        selEnd = XmTextGetInsertionPosition(window->lastFocus);
	selStart = selEnd - 1;
	if (selStart < 0) {
	    XBell(TheDisplay, 100);
	    return;
	}
    }
    if ((selEnd - selStart) != 1) {
    	XBell(TheDisplay, 100);
	return;
    }
    findText = GetTextRange(window->textArea, selStart, selEnd);
    if (findText == NULL) {
    	XBell(TheDisplay, 100);
	return;
    }
    XtFree(findText);
    
    /*
    ** Search for it in the text widget
    */
    /* get the entire (sigh) text buffer from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1,
		"Out of memory!\nTry closing some windows.\nSave your files!",
		"OK");
	return;
    }
    /* lookup the matching character and search for it */
    found = findMatchingChar(fileString, fileString[selStart],
    	    selStart, &matchPos);
    startPos = (matchPos > selStart) ? selStart : matchPos;
    endPos = (matchPos > selStart) ? matchPos : selStart;
    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
    if (!found) {
    	XBell(TheDisplay, 100);
	return;
    }

    /*
    ** select the text between the matching characters
    */
    XmTextSetSelection(window->lastFocus, startPos, endPos+1, time);
}

static int findMatchingChar(char *string, char c, int charPos, int *matchPos)
{
    int nestDepth, matchIndex, direction, beginPos;
    char *stringPtr;
    char matchC;
    
    /* Look up the matching character */
    for (matchIndex = 0; matchIndex<N_MATCH_CHARS; matchIndex++) {
        if (MatchingChars[matchIndex].c == c)
	    break;
    }
    if (matchIndex == N_MATCH_CHARS)
	return FALSE;
    matchC = MatchingChars[matchIndex].match;
    
    /* find it in string */
    direction = MatchingChars[matchIndex].direction;
    beginPos = (direction==SEARCH_FORWARD) ? charPos+1 : charPos-1;
    nestDepth = 1;
    if (direction == SEARCH_FORWARD) {
    	for (stringPtr= &string[beginPos]; *stringPtr!='\0'; stringPtr++) {
	    if (*stringPtr == matchC) {
	    	nestDepth--;
		if (nestDepth == 0) {
		    *matchPos = stringPtr - string;
		    return TRUE;
		}
	    } else if (*stringPtr == c) {
	    	nestDepth++;
	    }
	}
    } else {
    	/* SEARCH_BACKWARD */
	for (stringPtr= &string[beginPos]; stringPtr>=string; stringPtr--) {
	    if (*stringPtr == matchC) {
	    	nestDepth--;
		if (nestDepth == 0) {
		    *matchPos = stringPtr - string;
		    return TRUE;
		}
	    } else if (*stringPtr == c) {
	    	nestDepth++;
	    }
	}
    }
    return FALSE;
}

/*
** Xt timer procedure for erasing the matching parenthesis marker.
*/
static void flashTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    eraseFlash((WindowInfo *)clientData);
    ((WindowInfo *)clientData)->flashTimeoutID = 0;
}

/*
** Erase the marker drawn on a matching parenthesis bracket or brace
** character.
*/
static void eraseFlash(WindowInfo *window)
{
    XmTextPosition left, right, erasePos = window->flashPos;
    char *eraseChar;

    /* Unhighlight the highlighted character by unhighlighting the whole
       text widget (this is ok even when there's a selection) */
    XmTextSetHighlight(window->flashPane, 0,
     	    XmTextGetLastPosition(window->flashPane), XmHIGHLIGHT_NORMAL);

    /* if the position to be erased intersects the selection, doing a
       replace will re-anchor the selection, so just leave it */
    if (XmTextGetSelectionPosition(window->flashPane, &left, &right) &&
    	    left != right && erasePos >= left && erasePos <= right)
    	return;

    /* There is a bug in the text widget thru Motif 1.2 which makes it
       sometimes not erase the highlighted character.  Replacing the
       character with itself forces the erase.  Test if this code is
       still necessary by running NEdit without it occasionally and
       making sure all of the parenthesis flashes are removed.  There
       is yet another bug in the Motif 1.1 text widget that makes it move
       the insertion position after a replace of the character in front
       of the cursor.  In the Motif 1.1 case, check and restore the
       insertion cursor position if it gets moved by the replace. */ 
    eraseChar = GetTextRange(window->flashPane, erasePos, erasePos+1);
    window->ignoreModify = True;
#if XmVersion < 1002
    {   XmTextPosition insPos = XmTextGetInsertionPosition(window->flashPane);
        XmTextReplace(window->flashPane, erasePos, erasePos+1, eraseChar);
        if (insPos != XmTextGetInsertionPosition(window->flashPane))
    	    XmTextSetInsertionPosition(window->flashPane, insPos);
    }
#else
    XmTextReplace(window->flashPane, erasePos, erasePos+1, eraseChar);
#endif
    window->ignoreModify = False;
    XtFree(eraseChar);
}

int SearchAndReplace(WindowInfo *window, int direction)
{
    int startPos, endPos, replaceLen;
    int found;
    int beginPos, cursorPos;
    int selectionMatches;
    
    /* If the text selected in the window matches the search string, 	*/
    /* the user is probably using search then replace method, so	*/
    /* replace the selected text regardless of where the cursor is	*/
    selectionMatches = searchMatchesSelection(window);
    if (selectionMatches) {
    	GetSelection(window->textArea, &startPos, &endPos);
	
    /* Otherwise, search for the string */
    } else {
	/* get the position to start the search */
	cursorPos = XmTextGetInsertionPosition(window->lastFocus);
	if (direction == SEARCH_BACKWARD) {
	    /* use the insert position - 1 for backward searches */
	    beginPos = cursorPos-1;
	} else {
	    /* use the insert position for forward searches */
	    beginPos = cursorPos;
	}
	/* do the search */
	found = SearchWindow(window, direction, beginPos, &startPos, &endPos);
	if (!found)
	    return FALSE;
    }
    
    /* replace the text */
    if (TheSearchType == SEARCH_REGEX) {
    	char replaceString[SEARCHMAX], *foundString;
	foundString = GetTextRange(window->textArea, startPos, endPos);
    	replaceUsingRE(TheSearchString, TheReplaceString, foundString,
		replaceString, SEARCHMAX);
	XtFree(foundString);
    	XmTextReplace(window->lastFocus, startPos, endPos, replaceString);
    	replaceLen = strlen(replaceString);
    } else {
    	XmTextReplace(window->lastFocus, startPos, endPos, TheReplaceString);
    	replaceLen = strlen(TheReplaceString);
    }
    
    /* set the cursor position so autoShowCursorPosition will show the
       selected string */
    XmTextSetInsertionPosition(window->lastFocus, startPos +
    	((direction == SEARCH_FORWARD) ? replaceLen : 0));
    
    /* after successfully completing a replace, selected text attracts	*/
    /* attention away from the area of the replacement, particularly	*/
    /* when the selection represents a previous search. so deselect	*/
    XmTextClearSelection(window->lastFocus, CurrentTime);

    return TRUE;
}

int ReplaceInSelection(WindowInfo *window)
{
    int selStart, selEnd, beginPos, startPos, endPos, realOffset, replaceLen;
    int found, anyFound;
    char *fileString;
    
    /* find out where the selection is */
    if (!GetSelection(window->textArea, &selStart, &selEnd))
    	return FALSE;
	
    /* get the ENTIRE selected text */
    fileString = XmTextGetSelection(window->textArea);
    if (fileString == NULL)
    	return FALSE;
	
    replaceLen = strlen(TheReplaceString);
    found = TRUE;
    anyFound = FALSE;
    beginPos = 0;
    realOffset = selStart;
    while (found) {
	found = SearchString(fileString, TheSearchString, SEARCH_FORWARD,
			TheSearchType, FALSE, beginPos, &startPos, &endPos);
	if (found) {
	    /* replace the string and compensate for length change */
	    if (TheSearchType == SEARCH_REGEX) {
    		char replaceString[SEARCHMAX], *foundString;
		foundString = GetTextRange(window->textArea,
			startPos+realOffset, endPos+realOffset);
    		replaceUsingRE(TheSearchString, TheReplaceString, foundString,
			replaceString, SEARCHMAX);
		XtFree(foundString);
    		XmTextReplace(window->lastFocus, startPos+realOffset,
    			endPos+realOffset, replaceString);
    		replaceLen = strlen(replaceString);
	    } else
    		XmTextReplace(window->lastFocus, startPos+realOffset,
    			endPos+realOffset, TheReplaceString);
    	    realOffset += replaceLen - (endPos - startPos);
    	    /* start again after match unless match was empty, then endPos+1 */
    	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    anyFound = TRUE;
	}
    }
    XtFree(fileString);
    if (!anyFound) {
    	if (GetPrefSearchDlogs()) {
    	    /* Avoid bug in Motif by putting away search dialog before DialogF */
    	    if (window->findDlog)
    		XtUnmanageChild(window->findDlog);
    	    if (window->replaceDlog)
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 100);
 	return FALSE;
    }
    
    /* set the insert point at the end of the last replacement */
    XmTextSetInsertionPosition(window->lastFocus, endPos + realOffset);

    return TRUE;
}

int ReplaceAll(WindowInfo *window)
{
    int beginPos, startPos, endPos, lastEndPos;
    int found, nFound, removeLen, replaceLen, copyLen, addLen;
    int copyStart, copyEnd;
    char *fileString, *newFileString, *fillPtr;
    
    /* reject empty string */
    if (*TheSearchString == '\0')
    	return FALSE;
	
    /* get the entire text buffer from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL)
	return FALSE;		/* rely on XtMalloc to print error */
    
    /* rehearse the search first to determine the size of the buffer needed
       to hold the substituted text.  No substitution done here yet */
    replaceLen = strlen(TheReplaceString);
    found = TRUE;
    nFound = 0;
    removeLen = 0;
    addLen = 0;
    beginPos = 0;
    copyStart = -1;
    while (found) {
    	found = SearchString(fileString, TheSearchString, SEARCH_FORWARD,
    			 TheSearchType, FALSE, beginPos, &startPos, &endPos);
	if (found) {
	    if (copyStart < 0)
	    	copyStart = startPos;
    	    copyEnd = endPos;
    	    /* start next after match unless match was empty, then endPos+1 */
    	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    nFound++;
	    removeLen += endPos - startPos;
	    if (TheSearchType == SEARCH_REGEX) {
    		char replaceString[SEARCHMAX];
    		replaceUsingRE(TheSearchString, TheReplaceString,
    			&fileString[startPos], replaceString, SEARCHMAX);
    		addLen += strlen(replaceString);
    	    } else
    	    	addLen += replaceLen;
	}
    }
    
    /* If no matches were found, beep to tell the user, and return */
    if (nFound == 0) {
    	if (GetPrefSearchDlogs()) {
    	    /* Avoid bug in Motif by putting away search dialog before DialogF */
    	    if (window->findDlog)
    		XtUnmanageChild(window->findDlog);
    	    if (window->replaceDlog)
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 100);
	return FALSE;
    }
    
    /* Allocate a new buffer to hold all of the new text between the first
       and last substitutions */
    copyLen = copyEnd - copyStart;
    newFileString = XtMalloc(copyLen - removeLen + addLen + 1);
    if (newFileString == NULL)
	return FALSE;		/* rely on XtMalloc to print error */
    
    /* Scan through the text buffer again, substituting the replace string
       and copying the part between replaced text to the new buffer  */
    found = TRUE;
    beginPos = 0;
    lastEndPos = 0;
    fillPtr = newFileString;
    while (found) {
    	found = SearchString(fileString, TheSearchString, SEARCH_FORWARD,
    			 TheSearchType, FALSE, beginPos, &startPos, &endPos);
	if (found) {
	    if (beginPos != 0) {
		memcpy(fillPtr, &fileString[lastEndPos], startPos - lastEndPos);
		fillPtr += startPos - lastEndPos;
	    }
	    if (TheSearchType == SEARCH_REGEX) {
    		char replaceString[SEARCHMAX];
    		replaceUsingRE(TheSearchString, TheReplaceString,
    			&fileString[startPos], replaceString, SEARCHMAX);
    		replaceLen = strlen(replaceString);
    		memcpy(fillPtr, replaceString, replaceLen);
	    } else {
		memcpy(fillPtr, TheReplaceString, replaceLen);
	    }
	    fillPtr += replaceLen;
	    lastEndPos = endPos;
	    /* start next after match unless match was empty, then endPos+1 */
	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	}
    }
    *fillPtr = 0;
    XtFree(fileString);
    
    /* replace the contents of the text widget with the substituted text */
    XmTextReplace(window->lastFocus, copyStart, copyEnd, newFileString);
    XtFree(newFileString);
    
    /* Move the cursor to the end of the last replacement */
    XmTextSetInsertionPosition(window->lastFocus,
	   copyStart + fillPtr - newFileString);

    return TRUE;
}

/*
** searches through the text in window, attempting to match the
** current search string
*/
int SearchWindow(WindowInfo *window, int direction,
	int beginPos, int *startPos, int *endPos)
{
    char *fileString;
    int found, resp, fileEnd;
    
    /* reject empty string */
    if (*TheSearchString == '\0')
    	return FALSE;
	
    /* get the entire (sigh) text buffer from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1,
		"Out of memory!\nTry closing some windows.\nSave your files!",
		"OK");
	return FALSE;
    }
    
    /* search the string copied from the text area widget, and present
       dialogs, or just beep */
    if (GetPrefSearchDlogs()) {
    	found = SearchString(fileString, TheSearchString, direction,
    		TheSearchType, FALSE, beginPos, startPos, endPos);
    	/* Avoid bug in Motif by putting away search dialog before DialogF */
    	if (window->findDlog)
    	    XtUnmanageChild(window->findDlog);
    	if (window->replaceDlog)
    	    XtUnmanageChild(window->replaceDlog);
    	if (!found) {
    	    fileEnd = TextLength(window->textArea) - 1;
    	    if (direction == SEARCH_FORWARD && beginPos != 0) {
    		resp = DialogF(DF_QUES, window->shell, 2,
    			"Continue search from\nbeginning of file?", "Continue",
    			"Cancel");
    		if (resp == 2) {
 		    XtFree(fileString);
		    return False;
		}
   	    	found = SearchString(fileString, TheSearchString, direction,
    			TheSearchType, FALSE, 0, startPos, endPos);
	    } else if (direction == SEARCH_BACKWARD && beginPos != fileEnd) {
    		resp = DialogF(DF_QUES, window->shell, 2,
    			"Continue search\nfrom end of file?", "Continue",
    			"Cancel");
    		if (resp == 2) {
 		    XtFree(fileString);
		    return False;
		}
    	    	found = SearchString(fileString, TheSearchString, direction,
    			TheSearchType, FALSE, fileEnd, startPos, endPos);
	    }
	    if (!found)
    	    	DialogF(DF_INF, window->shell,1,"String was not found","OK");
    	}
    } else { /* no dialogs */
    	found = SearchString(fileString, TheSearchString, direction,
    		TheSearchType, TRUE, beginPos, startPos, endPos);
    	if (!found)
    	    XBell(TheDisplay, 100);
    }
    
    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);

    return found;
}

int SearchString(char *string, char *searchString, int direction,
	   int searchType, int wrap, int beginPos, int *startPos, int *endPos)
{
    switch (searchType) {
      case SEARCH_CASE_SENSE:
      	 return searchLiteral(string, searchString, TRUE, direction, wrap,
	 		       beginPos, startPos, endPos);
      case SEARCH_LITERAL:
      	 return  searchLiteral(string, searchString, FALSE, direction, wrap,
	 		       beginPos, startPos, endPos);
      case SEARCH_REGEX:
      	 return  searchRegex(string, searchString, direction, wrap,
      	 		     beginPos, startPos, endPos);
    }
    return FALSE; /* never reached, just makes compilers happy */
}

static int searchLiteral(char *string, char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos)
{
/* This is critical code for the speed of searches.			    */
/* For efficiency, we define the macro DOSEARCH with the guts of the search */
/* routine and repeat it, changing the parameters of the outer loop for the */
/* searching, forwards, backwards, and before and after the begin point	    */
#define DOSEARCH() \
    if (*filePtr == *ucString || *filePtr == *lcString) { \
	/* matched first character */ \
	ucPtr = ucString; \
	lcPtr = lcString; \
	tempPtr = filePtr; \
	while (*tempPtr == *ucPtr || *tempPtr == *lcPtr) { \
	    tempPtr++; ucPtr++; lcPtr++; \
	    if (*ucPtr == 0) { \
		/* matched whole string */ \
		*startPos = filePtr - string; \
		*endPos = tempPtr - string; \
		return TRUE; \
	    } \
	} \
    } \

    register char *filePtr, *tempPtr, *ucPtr, *lcPtr;
    char lcString[SEARCHMAX], ucString[SEARCHMAX];

    if (caseSense) {
        strcpy(ucString, searchString);
        strcpy(lcString, searchString);
    } else {
    	upCaseString(ucString, searchString);
    	downCaseString(lcString, searchString);
    }

    if (direction == SEARCH_FORWARD) {
	/* search from beginPos to end of string */
	for (filePtr=string+beginPos; *filePtr!=0; filePtr++) {
	    DOSEARCH()
	}
	if (!wrap)
	    return FALSE;
	/* search from start of file to beginPos	*/
	for (filePtr=string; filePtr<=string+beginPos; filePtr++) {
	    DOSEARCH()
	}
	return FALSE;
    } else {
    	/* SEARCH_BACKWARD */
	/* search from beginPos to start of file.  A negative begin pos	*/
	/* says begin searching from the far end of the file		*/
	if (beginPos >= 0) {
	    for (filePtr=string+beginPos; filePtr>=string; filePtr--) {
		DOSEARCH()
	    }
	}
	if (!wrap)
	    return FALSE;
	/* search from end of file to beginPos */
	/*... this strlen call is extreme inefficiency, but it's not obvious */
	/* how to get the text string length from the text widget (under 1.1)*/
	for (filePtr=string+strlen(string);
		filePtr>=string+beginPos; filePtr--) {
	    DOSEARCH()
	}
	return FALSE;
    }
}

static int searchRegex(char *string, char *searchString, int direction,
		int wrap, int beginPos, int *startPos, int *endPos)
{
    if (direction == SEARCH_FORWARD)
	return forwardRegexSearch(string, searchString, wrap, 
				  beginPos, startPos, endPos);
    else
    	return backwardRegexSearch(string, searchString, wrap, 
				   beginPos, startPos, endPos);
}

static int forwardRegexSearch(char *string, char *searchString,
		int wrap, int beginPos, int *startPos, int *endPos)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    
    /* compile the search string for searching with ExecRE.  Note that
       this does not process errors from compiling the expression.  It
       assumes that the expression was checked earlier. */
    compiledRE = CompileRE(searchString, &compileMsg);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to end of string */
    if (ExecRE(compiledRE, string + beginPos, NULL, FALSE,
    		beginPos == 0 ? TRUE : (string[beginPos-1] == '\n'))) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }
    
    /* if wrap turned off, we're done */
    if (!wrap) {
    	XtFree((char *)compiledRE);
	return FALSE;
    }
    
    /* search from the beginning of the string to beginPos */
    if (ExecRE(compiledRE, string, string + beginPos, FALSE, TRUE)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }

    XtFree((char *)compiledRE);
    return FALSE;
}

static int backwardRegexSearch(char *string, char *searchString,
		int wrap, int beginPos, int *startPos, int *endPos)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    int length;

    /* compile the search string for searching with ExecRE */
    compiledRE = CompileRE(searchString, &compileMsg);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to start of file.  A negative begin pos	*/
    /* says begin searching from the far end of the file.		*/
    if (beginPos >= 0) {
	if (ExecRE(compiledRE, string, string + beginPos, TRUE, TRUE)) {
	    *startPos = compiledRE->startp[0] - string;
	    *endPos = compiledRE->endp[0] - string;
	    XtFree((char *)compiledRE);
	    return TRUE;
	}
    }
    
    /* if wrap turned off, we're done */
    if (!wrap && beginPos >= 0) {
    	XtFree((char *)compiledRE);
    	return FALSE;
    }
    
    /* search from the end of the string to beginPos */
    if (beginPos < 0)
    	beginPos = 0;
    length = strlen(string); /* sadly, this means scanning entire string */
    if (ExecRE(compiledRE, string + beginPos, string + length, TRUE,
    		beginPos == 0 ? TRUE : (string[beginPos-1] == '\n'))) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }
    XtFree((char *)compiledRE);
    return FALSE;
}

static void upCaseString(char *outString, char *inString)
{
    char *outPtr, *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = toupper(*inPtr);
    }
    *outPtr = 0;
}

static void downCaseString(char *outString, char *inString)
{
    char *outPtr, *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = tolower(*inPtr);
    }
    *outPtr = 0;
}

/*
** resetFindTabGroup & resetReplaceTabGroup are really gruesome kludges to
** set the keyboard traversal.  XmProcessTraversal does not work at
** all on these dialogs.  ...It seems to have started working around
** Motif 1.1.2
*/
static void resetFindTabGroup(WindowInfo *window)
{
#ifdef MOTIF10
    XmRemoveTabGroup(window->findText);
    XmRemoveTabGroup(window->findSearchTypeBox);
    XmRemoveTabGroup(window->findBtns);
    XmAddTabGroup(window->findText);
    XmAddTabGroup(window->findSearchTypeBox);
    XmAddTabGroup(window->findBtns);
#endif
    XmProcessTraversal(window->findText, XmTRAVERSE_CURRENT);
}
static void resetReplaceTabGroup(WindowInfo *window)
{
#ifdef MOTIF10
    XmRemoveTabGroup(window->replaceText);
    XmRemoveTabGroup(window->replaceWithText);
    XmRemoveTabGroup(window->replaceSearchTypeBox);
    XmRemoveTabGroup(window->replaceBtns);
    XmAddTabGroup(window->replaceText);
    XmAddTabGroup(window->replaceWithText);
    XmAddTabGroup(window->replaceSearchTypeBox);
    XmAddTabGroup(window->replaceBtns);
#endif
    XmProcessTraversal(window->replaceText, XmTRAVERSE_CURRENT);
}

static int searchMatchesSelection(WindowInfo *window)
{
    int selLen, selStart, selEnd, startPos, endPos;
    char *string;
    int found;
    
    /* find length of selection, give up on no selection or too long */
    if (!GetSelection(window->textArea, &selStart, &selEnd))
	return FALSE;
    selLen = selEnd - selStart;
    if (selLen > SEARCHMAX)
	return FALSE;
    
    /* get the selected text */
    string = XmTextGetSelection(window->textArea);
    if (string == NULL)
    	return FALSE;
	
    /* search for the string in the selection (we are only interested 	*/
    /* in an exact match, but the procedure searchString does important */
    /* stuff like applying the correct matching algorithm)		*/
    found = SearchString(string, TheSearchString, SEARCH_FORWARD,
    			 TheSearchType, FALSE, 0, &startPos, &endPos);
    XtFree(string);
    
    /* decide if it is an exact match */
    if (!found)
    	return FALSE;
    if (startPos != 0 || endPos != selLen)
    	return FALSE;
    return TRUE;
}

/*
** Substitutes a replace string for a string that was matched using a
** regular expression.  This was added later and is very inneficient
** because instead of using the compiled regular expression that was used
** to make the match in the first place, it re-compiles the expression
** and redoes the search on the already-matched string.  This allows the
** code to continue using strings to represent the search and replace
** items.
*/  
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen)
{
    regexp *compiledRE;
    char *compileMsg;
    
    compiledRE = CompileRE(searchStr, &compileMsg);
    ExecRE(compiledRE, sourceStr, NULL, False, True);
    SubstituteRE(compiledRE, replaceStr, destStr, maxDestLen);
    XtFree((char *)compiledRE);
}

static void saveSearchHistory(void)
{
    char *sStr, *rStr;
    
    /* compare the current search and replace strings against the saved ones.
       If they are identical, don't bother saving */
    if (NHist >= 1 && TheSearchType == SearchTypeHistory[historyIndex(1)] &&
    	    !strcmp(SearchHistory[historyIndex(1)], TheSearchString) &&
    	    !strcmp(ReplaceHistory[historyIndex(1)], TheReplaceString)) {
    	return;
    }
    
    /* if there are more than MAX_SEARCH_HISTORY strings saved, recycle
       some space, free the entry that's about to be overwritten */
    if (NHist == MAX_SEARCH_HISTORY) {
    	XtFree(SearchHistory[HistStart]);
    	XtFree(ReplaceHistory[HistStart]);
    } else
    	NHist++;
    
    /* allocate and copy the search and replace strings and add them to the
       circular buffers at HistStart, bump the buffer pointer to next pos. */
    sStr = XtMalloc(strlen(TheSearchString) + 1);
    rStr = XtMalloc(strlen(TheReplaceString) + 1);
    strcpy(sStr, TheSearchString);
    strcpy(rStr, TheReplaceString);
    SearchHistory[HistStart] = sStr;
    ReplaceHistory[HistStart] = rStr;
    SearchTypeHistory[HistStart] = TheSearchType;
    HistStart++;
    if (HistStart >= MAX_SEARCH_HISTORY)
    	HistStart = 0;
}

/*
** return an index into the circular buffer arrays of history information
** for search strings, given the number of saveSearchHistory cycles back from
** the current time.
*/

static int historyIndex(int nCycles)
{
    int index;
    
    if (nCycles > NHist || nCycles <= 0)
    	return -1;
    index = HistStart - nCycles;
    if (index < 0)
    	index = MAX_SEARCH_HISTORY + index;
    return index;
}

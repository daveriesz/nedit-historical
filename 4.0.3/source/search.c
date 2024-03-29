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
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "search.h"
#include "window.h" 
#include "preferences.h"

/* History mechanism for search and replace strings */
static char *SearchHistory[MAX_SEARCH_HISTORY];
static char *ReplaceHistory[MAX_SEARCH_HISTORY];
static int SearchTypeHistory[MAX_SEARCH_HISTORY];
static int HistStart = 0;
static int NHist = 0;

static void createReplaceDlog(Widget parent, WindowInfo *window);
static void createFindDlog(Widget parent, WindowInfo *window);
static void fFocusCB(Widget w, WindowInfo *window, caddr_t *callData);
static void rFocusCB(Widget w, WindowInfo *window, caddr_t *callData);
static void rKeepCB(Widget w, WindowInfo *window, caddr_t *callData);
static void fKeepCB(Widget w, WindowInfo *window, caddr_t *callData);
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
static int getReplaceDlogInfo(WindowInfo *window, int *direction,
	char *searchString, char *replaceString, int *searchType);
static int getFindDlogInfo(WindowInfo *window, int *direction,
	char *searchString, int *searchType);
static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
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
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right);
static int findMatchingChar(textBuffer *buf, char toMatch, int charPos,
	int startLimit, int endLimit, int *matchPos);
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen);
static void saveSearchHistory(char *searchString, char *replaceString,
	int searchType);
static int historyIndex(int nCycles);
static char *searchTypeArg(int searchType);
static char *directionArg(int direction);

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

    /* Create the dialog if it doesn't already exist */
    if (window->replaceDlog == NULL)
    	createReplaceDlog(window->shell, window);
    
    /* If the window is already up, just pop it to the top */
    if (XtIsManaged(window->replaceDlog)) {
	XMapRaised(TheDisplay, XtWindow(XtParent(window->replaceDlog)));
	return;
    }
    	
    /* Set the initial search type */
    switch (GetPrefSearch()) {
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
    
    /* Set the initial direction based on the direction argument */
    XmToggleButtonSetState(direction == SEARCH_FORWARD ?
	    window->replaceFwdBtn : window->replaceRevBtn, True, True);
    
    /* Set the state of the Keep Dialog Up button */
    XmToggleButtonSetState(window->replaceKeepBtn, GetPrefKeepSearchDlogs(),
    	    True);
    
    /* Blank the text fields */
    XmTextSetString(window->replaceText, "");
    XmTextSetString(window->replaceWithText, "");
    
    /* Start the search history mechanism at the current history item */
    window->rHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->replaceDlog);
}

void DoFindDlog(WindowInfo *window, int direction)
{
    Widget button;

    /* Create the dialog if it doesn't already exist */
    if (window->findDlog == NULL)
    	createFindDlog(window->shell, window);
    
    /* If the window is already up, just pop it to the top */
    if (XtIsManaged(window->findDlog)) {
	XMapRaised(TheDisplay, XtWindow(XtParent(window->findDlog)));
	return;
    }

    /* Set the initial search type */
    switch (GetPrefSearch()) {
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
    
    /* Set the initial direction based on the direction argument */
    XmToggleButtonSetState(direction == SEARCH_FORWARD ?
	    window->findFwdBtn : window->findRevBtn, True, True);
    
    /* Set the state of the Keep Dialog Up button */
    XmToggleButtonSetState(window->findKeepBtn, GetPrefKeepSearchDlogs(),
    	    True);
    
    /* Blank the text field */
    XmTextSetString(window->findText, "");
    
    /* start the search history mechanism at the current history item */
    window->fHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->findDlog);
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
    Widget	searchDirBox, forwardBtn, reverseBtn, keepBtn;
    char 	title[MAXPATHLEN + 14];
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    form = XmCreateFormDialog(parent, "replaceDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, 0);
    if (GetPrefKeepSearchDlogs()) {
    	sprintf(title, "Replace (in %s)", window->filename);
    	XtVaSetValues(XtParent(form), XmNtitle, title, 0);
    } else
    	XtVaSetValues(XtParent(form), XmNtitle, "Replace", 0);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 4); argcnt++;
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
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
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
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchDirBox = XmCreateRowColumn(form, "searchDirBox", args, argcnt);
    XtManageChild(searchDirBox);
    XmAddTabGroup(searchDirBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Forward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'o'); argcnt++;
    forwardBtn = XmCreateToggleButton(searchDirBox, "forward", args, argcnt);
    XmStringFree(st1);
    XtManageChild(forwardBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Backward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'B'); argcnt++;
    reverseBtn = XmCreateToggleButton(searchDirBox, "reverse", args, argcnt);
    XmStringFree(st1);
    XtManageChild(reverseBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Keep Dialog")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'K'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    keepBtn = XmCreateToggleButton(form, "keep", args, argcnt);
    XtAddCallback(keepBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)rKeepCB, window);
    XmStringFree(st1);
    XtManageChild(keepBtn);
    XmAddTabGroup(keepBtn);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchDirBox); argcnt++;
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
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Find")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'F'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 21); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 33); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    findBtn = XmCreatePushButton(btnForm, "find", args, argcnt);
    XtAddCallback(findBtn, XmNactivateCallback, (XtCallbackProc)rFindCB,window);
    XmStringFree(st1);
    XtManageChild(findBtn);
 
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
    XtSetArg(args[argcnt], XmNleftPosition, 33); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 56); argcnt++;
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
    XtSetArg(args[argcnt], XmNsensitive, window->wasSelected); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 56); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 85); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    rInSelBtn = XmCreatePushButton(btnForm, "inSel", args, argcnt);
    XtAddCallback(rInSelBtn, XmNactivateCallback, (XtCallbackProc)rInSelCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(rInSelBtn);
 
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
    window->replaceFwdBtn = forwardBtn;
    window->replaceRevBtn = reverseBtn;
    window->replaceKeepBtn = keepBtn;
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
    Widget	searchDirBox, forwardBtn, reverseBtn, keepBtn;
    char 	title[MAXPATHLEN + 11];
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    form = XmCreateFormDialog(parent, "findDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, 0);
    if (GetPrefKeepSearchDlogs()) {
    	sprintf(title, "Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(form), XmNtitle, title, 0);
    } else
    	XtVaSetValues(XtParent(form), XmNtitle, "Find", 0);
 
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
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
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
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchDirBox = XmCreateRowColumn(form, "searchDirBox", args, argcnt);
    XtManageChild(searchDirBox);
    XmAddTabGroup(searchDirBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Forward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'o'); argcnt++;
    forwardBtn = XmCreateToggleButton(searchDirBox, "forward", args, argcnt);
    XmStringFree(st1);
    XtManageChild(forwardBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Backward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'B'); argcnt++;
    reverseBtn = XmCreateToggleButton(searchDirBox, "reverse", args, argcnt);
    XmStringFree(st1);
    XtManageChild(reverseBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Keep Dialog")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'K'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    keepBtn = XmCreateToggleButton(form, "keep", args, argcnt);
    XtAddCallback(keepBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)fKeepCB, window);
    XmStringFree(st1);
    XtManageChild(keepBtn);
    XmAddTabGroup(keepBtn);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchDirBox); argcnt++;
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
    window->findFwdBtn = forwardBtn;
    window->findRevBtn = reverseBtn;
    window->findKeepBtn = keepBtn;
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

/* when keeping a window up, clue the user what window it's associated with */
static void rKeepCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    char title[MAXPATHLEN + 14];

    if (XmToggleButtonGetState(w)) {
    	sprintf(title, "Replace (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, 0);
    } else
    	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, "Replace", 0);
}
static void fKeepCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    char title[MAXPATHLEN + 11];

    if (XmToggleButtonGetState(w)) {
    	sprintf(title, "Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->findDlog), XmNtitle, title, 0);
    } else
    	XtVaSetValues(XtParent(window->findDlog), XmNtitle, "Find", 0);
}

static void replaceCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[4];
    
    /* Validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    /* Find the text and replace it */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = directionArg(direction);
    params[3] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace", callData->event, params, 4);
    
    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void replaceAllCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* Validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace-all", callData->event,
    	    params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void rInSelCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* Validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace-in-selection",
    	    callData->event, params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
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
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* Validate and transfer find and replace strings from the
       dialog to the global saved search information */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);
    
    /* find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 3);

    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
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
    	XBell(TheDisplay, 0);
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
    	XBell(TheDisplay, 0);
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
    	XBell(TheDisplay, 0);
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
    char searchString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* save find string from the dialog in the global search string */
    if (!getFindDlogInfo(window, &direction, searchString, &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetFindTabGroup(window);
    
    /* find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 3);

    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->findKeepBtn))
        XtUnmanageChild(window->findDlog);
}

/*
** Fetch and verify (particularly regular expression) search and replace
** strings and search type from the Replace dialog.  If the strings are ok,
** save a copy in the search history, copy them in to "searchString",
** "replaceString', which are assumed to be at least SEARCHMAX in length,
** return search type in "searchType", and return TRUE as the function
** value.  Otherwise, return FALSE.
*/
static int getReplaceDlogInfo(WindowInfo *window, int *direction,
	char *searchString, char *replaceString, int *searchType)
{
    char *replaceText, *replaceWithText;
    regexp *compiledRE = NULL;
    char *compileMsg;
    int type;
    
    /* Get the search and replace strings, search type, and direction
       from the dialog */
    replaceText = XmTextGetString(window->replaceText);
    replaceWithText = XmTextGetString(window->replaceWithText);
    if (XmToggleButtonGetState(window->replaceLiteralBtn))
    	type = SEARCH_LITERAL;
    else if (XmToggleButtonGetState(window->replaceCaseBtn))
    	type = SEARCH_CASE_SENSE;
    else
    	type = SEARCH_REGEX;
    *direction = XmToggleButtonGetState(window->replaceFwdBtn) ? SEARCH_FORWARD :
    	    SEARCH_BACKWARD;
    *searchType = type;
    
    /* If the search type is a regular expression, test compile it immediately
       and present error messages */
    if (type == SEARCH_REGEX) {
	compiledRE = CompileRE(replaceText, &compileMsg);
	if (compiledRE == NULL) {
   	    DialogF(DF_WARN, XtParent(window->replaceDlog), 1,
   	    	   "Please respecify the search string:\n%s", "OK", compileMsg);
 	    return FALSE;
 	}
    }
    
    /* Save a copy in the search history */
    saveSearchHistory(replaceText, replaceWithText, type);
    
    /* Return strings */
    strcpy(searchString, replaceText);
    strcpy(replaceString, replaceWithText);
    XtFree(replaceText);
    XtFree(replaceWithText);
    return TRUE;
}

/*
** Fetch and verify (particularly regular expression) search string,
** direction, and search type from the Find dialog.  If the search string
** is ok, save a copy in the search history, copy it to "searchString",
** which is assumed to be at least SEARCHMAX in length, return search type
** in "searchType", and return TRUE as the function value.  Otherwise,
** return FALSE.
*/
static int getFindDlogInfo(WindowInfo *window, int *direction,
	char *searchString, int *searchType)
{
    char *findText;
    regexp *compiledRE = NULL;
    char *compileMsg;
    int type;
    
    /* Get the search string, search type, and direction from the dialog */
    findText = XmTextGetString(window->findText);
    if (XmToggleButtonGetState(window->findLiteralBtn))
    	type = SEARCH_LITERAL;
    else if (XmToggleButtonGetState(window->findCaseBtn))
    	type = SEARCH_CASE_SENSE;
    else
    	type = SEARCH_REGEX;
    *direction = XmToggleButtonGetState(window->findFwdBtn) ? SEARCH_FORWARD :
    	    SEARCH_BACKWARD;
    *searchType = type;

    /* If the search type is a regular expression, test compile it immediately
       and present error messages */
    if (type == SEARCH_REGEX) {
	compiledRE = CompileRE(findText, &compileMsg);
	if (compiledRE == NULL) {
   	    DialogF(DF_WARN, XtParent(window->findDlog), 1,
   	    	   "Please respecify the search string:\n%s", "OK", compileMsg);
 	    return FALSE;
 	}
 	XtFree((char *)compiledRE);
    }

    /* Save a copy in the search history */
    saveSearchHistory(findText, "", type);

    /* Return the search string */
    strcpy(searchString, findText);
    XtFree(findText);
    return TRUE;
}

int SearchAndSelectSame(WindowInfo *window, int direction)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 0);
    	return FALSE;
    }
    
    return SearchAndSelect(window, direction, SearchHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)]);
}

int SearchAndSelect(WindowInfo *window, int direction, char *searchString,
	int searchType)
{
    int startPos, endPos;
    int beginPos, cursorPos, selStart, selEnd;
        
    /* set the position to start the search so we don't find the same
       string that was found on the last search	*/
    if (searchMatchesSelection(window, searchString, searchType,
    	    &selStart, &selEnd)) {
    	/* selection matches search string, start before or after sel.	*/
	if (direction == SEARCH_BACKWARD) {
	    beginPos = selStart-1;
	} else {
	    beginPos = selEnd;
	}
    } else {
    	selStart = -1; selEnd = -1;
    	/* no selection, or no match, search relative cursor */
    	cursorPos = TextGetCursorPos(window->lastFocus);
	if (direction == SEARCH_BACKWARD) {
	    /* use the insert position - 1 for backward searches */
	    beginPos = cursorPos-1;
	} else {
	    /* use the insert position for forward searches */
	    beginPos = cursorPos;
	}
    }

    /* do the search.  SearchWindow does appropriate dialogs and beeps */
    if (!SearchWindow(window, direction, searchString, searchType,
    	    beginPos, &startPos, &endPos))
    	return FALSE;
    	
    /* if the search matched an empty string (possible with regular exps)
       beginning at the start of the search, go to the next occurrence,
       otherwise repeated finds will get "stuck" at zero-length matches */
    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
    	if (!SearchWindow(window, direction, searchString, searchType,
    		beginPos+1, &startPos, &endPos))
    	    return FALSE;
    
    /* if matched text is already selected, just beep */
    if (selStart==startPos && selEnd==endPos) {
    	XBell(TheDisplay, 0);
    	return FALSE;
    }

    /* select the text found string */
    BufSelect(window->buffer, startPos, endPos);
    MakeSelectionVisible(window, window->lastFocus);
    TextSetCursorPos(window->lastFocus, endPos);
    
    return TRUE;
}

void SearchForSelected(WindowInfo *window, int direction, Time time)
{
   XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)selectedSearchCB, (XtPointer)direction,
    	    time);
}

static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
    WindowInfo *window = WidgetToWindow(w);
    int direction = (int)callData;
    int searchType;
    char searchString[SEARCHMAX+1];
    
    /* skip if we can't get the selection data or it's too long */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1,
   	    	    "Selection not appropriate for searching", "OK");
    	else
    	    XBell(TheDisplay, 0);
	return;
    }
    if (*length > SEARCHMAX) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1, "Selection too long", "OK");
    	else
    	    XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    if (*length == 0) {
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: can't handle non 8-bit text\n");
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    /* make the selection the current search string */
    strncpy(searchString, value, *length);
    searchString[*length] = '\0';
    XtFree(value);
    
    /* Use the default method for searching, unless it is regex, since this
       kind of search is by definition a literal search */
    searchType = GetPrefSearch();
    if (searchType == SEARCH_REGEX)
    	searchType = SEARCH_LITERAL;

    /* record the search parameters for recall with up arrow */
    saveSearchHistory(searchString, NULL, searchType);

    /* search for it in the window */
    SearchAndSelect(window, direction, searchString, searchType);
}

/*
** Check the character before the insertion cursor of textW and flash
** matching parenthesis, brackets, or braces, by temporarily highlighting
** the matching character (a timer procedure is scheduled for removing the
** highlights)
*/
void FlashMatching(WindowInfo *window, Widget textW)
{
    char c;
    int pos, matchIndex;
    int startPos, endPos, searchPos, matchPos;
    
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
    if (window->buffer->primary.selected)
   	return;

    /* get the character to match and the position to start from */
    pos = TextGetCursorPos(textW) - 1;
    if (pos < 0)
    	return;
    c = BufGetCharacter(window->buffer, pos);
    
    /* is the character one we want to flash? */
    for (matchIndex = 0; matchIndex<N_FLASH_CHARS; matchIndex++) {
        if (MatchingChars[matchIndex].c == c)
	    break;
    }
    if (matchIndex == N_FLASH_CHARS)
	return;

    /* Constrain the search to visible text (unless we're in split-window mode,
       then search the whole buffer), and get the string to search */
    if (MatchingChars[matchIndex].direction == SEARCH_BACKWARD) {
    	startPos = window->nPanes == 0 ? TextFirstVisiblePos(textW) : 0;
    	endPos = pos;
    	searchPos = endPos;
    } else {
    	startPos = pos;
    	endPos = window->nPanes == 0 ? TextLastVisiblePos(textW) :
    	    	window->buffer->length;
    	searchPos = startPos;
    }
    
    /* do the search */
    if (!findMatchingChar(window->buffer, c, searchPos, startPos, endPos,
    	    &matchPos))
    	return;

    /* highlight the matched character */
    BufHighlight(window->buffer, matchPos, matchPos+1);
      
    /* Set up a timer to erase the box after 1.5 seconds */
    window->flashTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 1500,
    	    flashTimeoutProc, window);
    window->flashPos = matchPos;
}

void MatchSelectedCharacter(WindowInfo *window)
{
    int selStart, selEnd;
    int startPos, endPos, matchPos;
    textBuffer *buf = window->buffer;

    /* get the character to match and its position from the selection, or
       the character before the insert point if nothing is selected.
       Give up if too many characters are selected */
    if (!GetSimpleSelection(buf, &selStart, &selEnd)) {
        selEnd = TextGetCursorPos(window->lastFocus);
	selStart = selEnd - 1;
	if (selStart < 0) {
	    XBell(TheDisplay, 0);
	    return;
	}
    }
    if ((selEnd - selStart) != 1) {
    	XBell(TheDisplay, 0);
	return;
    }
    
    /* Search for it in the buffer */
    if (!findMatchingChar(buf, BufGetCharacter(buf, selStart), selStart, 0,
    		buf->length, &matchPos)) {
    	XBell(TheDisplay, 0);
	return;
    }
    startPos = (matchPos > selStart) ? selStart : matchPos;
    endPos = (matchPos > selStart) ? matchPos : selStart;

    /* select the text between the matching characters */
    BufSelect(buf, startPos, endPos+1);
}

static int findMatchingChar(textBuffer *buf, char toMatch, int charPos,
	int startLimit, int endLimit, int *matchPos)
{
    int nestDepth, matchIndex, direction, beginPos, pos;
    char matchChar, c;
    
    /* Look up the matching character and match direction */
    for (matchIndex = 0; matchIndex<N_MATCH_CHARS; matchIndex++) {
        if (MatchingChars[matchIndex].c == toMatch)
	    break;
    }
    if (matchIndex == N_MATCH_CHARS)
	return FALSE;
    matchChar = MatchingChars[matchIndex].match;
    direction = MatchingChars[matchIndex].direction;
    
    /* find it in the buffer */
    beginPos = (direction==SEARCH_FORWARD) ? charPos+1 : charPos-1;
    nestDepth = 1;
    if (direction == SEARCH_FORWARD) {
    	for (pos=beginPos; pos<endLimit; pos++) {
	    c=BufGetCharacter(buf, pos);
	    if (c == matchChar) {
	    	nestDepth--;
		if (nestDepth == 0) {
		    *matchPos = pos;
		    return TRUE;
		}
	    } else if (c == toMatch)
	    	nestDepth++;
	}
    } else { /* SEARCH_BACKWARD */
	for (pos=beginPos; pos>=startLimit; pos--) {
	    c=BufGetCharacter(buf, pos);
	    if (c == matchChar) {
	    	nestDepth--;
		if (nestDepth == 0) {
		    *matchPos = pos;
		    return TRUE;
		}
	    } else if (c == toMatch)
	    	nestDepth++;
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
    BufUnhighlight(window->buffer);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceSame(WindowInfo *window, int direction)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 0);
    	return FALSE;
    }

    return SearchAndReplace(window, direction, SearchHistory[historyIndex(1)],
    	    ReplaceHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)]);
}

/*
** Search for string "searchString" in window "window", using algorithm
** "searchType" and direction "direction", and replace it with "replaceString"
*/
int SearchAndReplace(WindowInfo *window, int direction, char *searchString,
	char *replaceString, int searchType)
{
    int startPos, endPos, replaceLen;
    int found;
    int beginPos, cursorPos;
    
    /* If the text selected in the window matches the search string, 	*/
    /* the user is probably using search then replace method, so	*/
    /* replace the selected text regardless of where the cursor is.	*/
    /* Otherwise, search for the string.				*/
    if (!searchMatchesSelection(window, searchString, searchType,
    	    &startPos, &endPos)) {
	/* get the position to start the search */
	cursorPos = TextGetCursorPos(window->lastFocus);
	if (direction == SEARCH_BACKWARD) {
	    /* use the insert position - 1 for backward searches */
	    beginPos = cursorPos-1;
	} else {
	    /* use the insert position for forward searches */
	    beginPos = cursorPos;
	}
	/* do the search */
	found = SearchWindow(window, direction, searchString, searchType,
		beginPos, &startPos, &endPos);
	if (!found)
	    return FALSE;
    }
    
    /* replace the text */
    if (searchType == SEARCH_REGEX) {
    	char replaceResult[SEARCHMAX], *foundString;
	foundString = BufGetRange(window->buffer, startPos, endPos);
    	replaceUsingRE(searchString, replaceString, foundString,
		replaceResult, SEARCHMAX);
	XtFree(foundString);
    	BufReplace(window->buffer, startPos, endPos, replaceResult);
    	replaceLen = strlen(replaceResult);
    } else {
    	BufReplace(window->buffer, startPos, endPos, replaceString);
    	replaceLen = strlen(replaceString);
    }
    
    /* after successfully completing a replace, selected text attracts
       attention away from the area of the replacement, particularly
       when the selection represents a previous search. so deselect */
    BufUnselect(window->buffer);
    
    /* temporarily shut off autoShowInsertPos before setting the cursor
       position so MakeSelectionVisible gets a chance to place the replaced
       string at a pleasing position on the screen (otherwise, the cursor would
       be automatically scrolled on screen and MakeSelectionVisible would do
       nothing) */
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, False, 0);
    TextSetCursorPos(window->lastFocus, startPos +
    	((direction == SEARCH_FORWARD) ? replaceLen : 0));
    MakeSelectionVisible(window, window->lastFocus);
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, True, 0);
    
    return TRUE;
}

/*
** Replace all occurances of "searchString" in "window" with "replaceString"
** within the current primary selection in "window".
*/
int ReplaceInSelection(WindowInfo *window, char *searchString,
	char *replaceString, int searchType)
{
    int selStart, selEnd, beginPos, startPos, endPos, realOffset, replaceLen;
    int found, anyFound, isRect, rectStart, rectEnd, lineStart, cursorPos;
    char *fileString;
    textBuffer *tempBuf;
    
    /* find out where the selection is */
    if (!BufGetSelectionPos(window->buffer, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
    	return FALSE;
	
    /* get the selected text */
    if (isRect) {
    	selStart = BufStartOfLine(window->buffer, selStart);
    	selEnd = BufEndOfLine(window->buffer, selEnd);
    	fileString = BufGetRange(window->buffer, selStart, selEnd);
    } else
    	fileString = BufGetSelectionText(window->buffer);
    
    /* create a temporary buffer in which to do the replacements to hide the
       intermediate steps from the display routines, and so everything can
       be undone in a single operation */
    tempBuf = BufCreate();
    BufSetAll(tempBuf, fileString);
    
    /* search the string and do the replacements in the temporary buffer */
    replaceLen = strlen(replaceString);
    found = TRUE;
    anyFound = FALSE;
    beginPos = 0;
    realOffset = 0;
    while (found) {
	found = SearchString(fileString, searchString, SEARCH_FORWARD,
			searchType, FALSE, beginPos, &startPos, &endPos);
	if (!found)
	    break;
	/* if the selection is rectangular, verify that the found
	   string is in the rectangle */
	if (isRect) {
	    lineStart = BufStartOfLine(tempBuf, startPos+realOffset);
	    if (BufCountDispChars(tempBuf, lineStart, startPos+realOffset) <
	    	    rectStart || BufCountDispChars(tempBuf, lineStart,
	    	    endPos+realOffset) > rectEnd) {
		beginPos = (startPos == endPos) ? endPos+1 : endPos;
		continue;
	    }
	}
	/* Make sure the match did not start past the end (regular expressions
	   can consider the artificial end of the range as the end of a line,
	   and match a fictional whole line beginning there) */
	if (startPos == selEnd - selStart) {
	    found = False;
	    break;
	}
	/* replace the string and compensate for length change */
	if (searchType == SEARCH_REGEX) {
    	    char replaceResult[SEARCHMAX], *foundString;
	    foundString = BufGetRange(tempBuf, startPos+realOffset,
		    endPos+realOffset);
    	    replaceUsingRE(searchString, replaceString, foundString,
		    replaceResult, SEARCHMAX);
	    XtFree(foundString);
    	    BufReplace(tempBuf, startPos+realOffset, endPos+realOffset,
    		    replaceResult);
    	    replaceLen = strlen(replaceResult);
	} else
    	    BufReplace(tempBuf, startPos+realOffset, endPos+realOffset,
    		    replaceString);
    	realOffset += replaceLen - (endPos - startPos);
    	/* start again after match unless match was empty, then endPos+1 */
    	beginPos = (startPos == endPos) ? endPos+1 : endPos;
    	cursorPos = endPos;
	anyFound = TRUE;
    }
    XtFree(fileString);
    
    /* if nothing was found, tell user and return */
    if (!anyFound) {
    	if (GetPrefSearchDlogs()) {
    	    /* Avoid bug in Motif 1.1 by putting away search dialog
    	       before DialogF */
    	    if (window->findDlog && XtIsManaged(window->findDlog) &&
    	    	    !XmToggleButtonGetState(window->findKeepBtn))
    		XtUnmanageChild(window->findDlog);
    	    if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	    !XmToggleButtonGetState(window->replaceKeepBtn))
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 0);
 	BufFree(tempBuf);
 	return FALSE;
    }
    
    /* replace the selected range in the real buffer */
    fileString = BufGetAll(tempBuf);
    BufFree(tempBuf);
    BufReplace(window->buffer, selStart, selEnd, fileString);
    XtFree(fileString);
    
    /* set the insert point at the end of the last replacement */
    TextSetCursorPos(window->lastFocus, selStart + cursorPos + realOffset);
    
    /* leave non-rectangular selections selected (rect. ones after replacement
       are less useful since left/right positions are randomly adjusted) */
    if (!isRect)
    	BufSelect(window->buffer, selStart, selEnd + realOffset);

    return TRUE;
}

/*
** Replace all occurances of "searchString" in "window" with "replaceString"
*/
int ReplaceAll(WindowInfo *window, char *searchString, char *replaceString,
	int searchType)
{
    int beginPos, startPos, endPos, lastEndPos;
    int found, nFound, removeLen, replaceLen, copyLen, addLen;
    int copyStart, copyEnd;
    char *fileString, *newFileString, *fillPtr;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->buffer);
    if (fileString == NULL)
	return FALSE;		/* rely on XtMalloc to print error */
    
    /* rehearse the search first to determine the size of the buffer needed
       to hold the substituted text.  No substitution done here yet */
    replaceLen = strlen(replaceString);
    found = TRUE;
    nFound = 0;
    removeLen = 0;
    addLen = 0;
    beginPos = 0;
    copyStart = -1;
    while (found) {
    	found = SearchString(fileString, searchString, SEARCH_FORWARD,
    			 searchType, FALSE, beginPos, &startPos, &endPos);
	if (found) {
	    if (copyStart < 0)
	    	copyStart = startPos;
    	    copyEnd = endPos;
    	    /* start next after match unless match was empty, then endPos+1 */
    	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    nFound++;
	    removeLen += endPos - startPos;
	    if (searchType == SEARCH_REGEX) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString,
    			&fileString[startPos], replaceResult, SEARCHMAX);
    		addLen += strlen(replaceResult);
    	    } else
    	    	addLen += replaceLen;
	}
    }
    
    /* If no matches were found, beep to tell the user, and return */
    if (nFound == 0) {
    	if (GetPrefSearchDlogs()) {
    	    if (window->findDlog && XtIsManaged(window->findDlog) &&
    	    	    !XmToggleButtonGetState(window->findKeepBtn))
    		XtUnmanageChild(window->findDlog);
    	    if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	    !XmToggleButtonGetState(window->replaceKeepBtn))
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 0);
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
    	found = SearchString(fileString, searchString, SEARCH_FORWARD,
    			 searchType, FALSE, beginPos, &startPos, &endPos);
	if (found) {
	    if (beginPos != 0) {
		memcpy(fillPtr, &fileString[lastEndPos], startPos - lastEndPos);
		fillPtr += startPos - lastEndPos;
	    }
	    if (searchType == SEARCH_REGEX) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString,
    			&fileString[startPos], replaceResult, SEARCHMAX);
    		replaceLen = strlen(replaceResult);
    		memcpy(fillPtr, replaceResult, replaceLen);
	    } else {
		memcpy(fillPtr, replaceString, replaceLen);
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
    BufReplace(window->buffer, copyStart, copyEnd, newFileString);
    
    /* Move the cursor to the end of the last replacement */
    TextSetCursorPos(window->lastFocus, copyStart + fillPtr - newFileString);

    XtFree(newFileString);
    return TRUE;
}

/*
** Search the text in "window", attempting to match "searchString"
*/
int SearchWindow(WindowInfo *window, int direction, char *searchString,
	int searchType, int beginPos, int *startPos, int *endPos)
{
    char *fileString;
    int found, resp, fileEnd;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->buffer);
    
    /* search the string copied from the text area widget, and present
       dialogs, or just beep */
    if (GetPrefSearchDlogs()) {
    	found = SearchString(fileString, searchString, direction,
    		searchType, FALSE, beginPos, startPos, endPos);
    	/* Avoid Motif 1.1 bug by putting away search dialog before DialogF */
    	if (window->findDlog && XtIsManaged(window->findDlog) &&
    	    	!XmToggleButtonGetState(window->findKeepBtn))
    	    XtUnmanageChild(window->findDlog);
    	if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	!XmToggleButtonGetState(window->replaceKeepBtn))
    	    XtUnmanageChild(window->replaceDlog);
    	if (!found) {
    	    fileEnd = window->buffer->length - 1;
    	    if (direction == SEARCH_FORWARD && beginPos != 0) {
    		resp = DialogF(DF_QUES, window->shell, 2,
    			"Continue search from\nbeginning of file?", "Continue",
    			"Cancel");
    		if (resp == 2) {
 		    XtFree(fileString);
		    return False;
		}
   	    	found = SearchString(fileString, searchString, direction,
    			searchType, FALSE, 0, startPos, endPos);
	    } else if (direction == SEARCH_BACKWARD && beginPos != fileEnd) {
    		resp = DialogF(DF_QUES, window->shell, 2,
    			"Continue search\nfrom end of file?", "Continue",
    			"Cancel");
    		if (resp == 2) {
 		    XtFree(fileString);
		    return False;
		}
    	    	found = SearchString(fileString, searchString, direction,
    			searchType, FALSE, fileEnd, startPos, endPos);
	    }
	    if (!found)
    	    	DialogF(DF_INF, window->shell,1,"String was not found","OK");
    	}
    } else { /* no dialogs */
    	found = SearchString(fileString, searchString, direction,
    		searchType, TRUE, beginPos, startPos, endPos);
    	if (!found)
    	    XBell(TheDisplay, 0);
    }
    
    /* Free the text buffer copy returned from BufGetAll */
    XtFree(fileString);

    return found;
}

/*
** Search the null terminated string "string" for "searchString", beginning at
** "beginPos".  Returns the boundaries of the match in "startPos" and "endPos"
*/
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

/*
** Return TRUE if "searchString" exactly matches the text in the window's
** current primary selection using search algorithm "searchType".  If true,
** also return the position of the selection in "left" and "right".
*/
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right)
{
    int selLen, selStart, selEnd, startPos, endPos;
    char *string;
    int found, isRect, rectStart, rectEnd, lineStart;
    
    /* find length of selection, give up on no selection or too long */
    if (!BufGetSelectionPos(window->buffer, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
	return FALSE;
    if (selEnd - selStart > SEARCHMAX)
	return FALSE;
    
    /* if the selection is rectangular, don't match if it spans lines */
    if (isRect) {
    	lineStart = BufStartOfLine(window->buffer, selStart);
    	if (lineStart != BufStartOfLine(window->buffer, selEnd))
    	    return FALSE;
    }
    
    /* get the selected text */
    string = BufGetSelectionText(window->buffer);
    if (*string == '\0') {
    	XtFree(string);
    	return FALSE;
    }
    selLen = strlen(string);
    
    /* search for the string in the selection (we are only interested 	*/
    /* in an exact match, but the procedure SearchString does important */
    /* stuff like applying the correct matching algorithm)		*/
    found = SearchString(string, searchString, SEARCH_FORWARD,
    			 searchType, FALSE, 0, &startPos, &endPos);
    XtFree(string);
    
    /* decide if it is an exact match */
    if (!found)
    	return FALSE;
    if (startPos != 0 || endPos != selLen)
    	return FALSE;
    
    /* return the start and end of the selection */
    if (isRect)
    	GetSimpleSelection(window->buffer, left, right);
    else {
    	*left = selStart;
    	*right = selEnd;
    }
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

/*
** Store the search and replace strings, and search type for later recall.
** If replaceString is NULL, duplicate the last replaceString used.
*/
static void saveSearchHistory(char *searchString, char *replaceString,
	int searchType)
{
    char *sStr, *rStr;
    
    /* If replaceString is NULL, duplicate the last one (if any) */
    if (replaceString == NULL)
    	replaceString = NHist >= 1 ? ReplaceHistory[historyIndex(1)] : "";
    
    /* Compare the current search and replace strings against the saved ones.
       If they are identical, don't bother saving */
    if (NHist >= 1 && searchType == SearchTypeHistory[historyIndex(1)] &&
    	    !strcmp(SearchHistory[historyIndex(1)], searchString) &&
    	    !strcmp(ReplaceHistory[historyIndex(1)], replaceString)) {
    	return;
    }
    
    /* If there are more than MAX_SEARCH_HISTORY strings saved, recycle
       some space, free the entry that's about to be overwritten */
    if (NHist == MAX_SEARCH_HISTORY) {
    	XtFree(SearchHistory[HistStart]);
    	XtFree(ReplaceHistory[HistStart]);
    } else
    	NHist++;
    
    /* Allocate and copy the search and replace strings and add them to the
       circular buffers at HistStart, bump the buffer pointer to next pos. */
    sStr = XtMalloc(strlen(searchString) + 1);
    rStr = XtMalloc(strlen(replaceString) + 1);
    strcpy(sStr, searchString);
    strcpy(rStr, replaceString);
    SearchHistory[HistStart] = sStr;
    ReplaceHistory[HistStart] = rStr;
    SearchTypeHistory[HistStart] = searchType;
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

/*
** Return a pointer to the string describing search type for search action
** routine parameters (see menu.c for processing of action routines)
*/
static char *searchTypeArg(int searchType)
{
    if (searchType == SEARCH_LITERAL)
    	return "literal";
    else if (searchType == SEARCH_CASE_SENSE)
    	return "case";
    return "regex";
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters (see menu.c for processing of action routines)
*/
static char *directionArg(int direction)
{
    if (direction == SEARCH_BACKWARD)
    	return "backward";
    return "forward";
}

static const char CVSID[] = "$Id: search.c,v 1.33.2.1 2001/09/11 10:17:59 edg Exp $";
/*******************************************************************************
*									       *
* search.c -- Nirvana Editor search and replace functions		       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version.							               *
* 									       *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
* May 10, 1991								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifndef __MVS__
#include <sys/param.h>
#endif
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
#include <Xm/List.h>
#include <X11/Xatom.h>		/* for getting selection */
#include <X11/keysym.h>
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
#ifdef REPLACE_SCOPE
#if XmVersion >= 1002
#include <Xm/PrimitiveP.h>
#endif
#include "textDisp.h"
#include "textP.h"
#endif

typedef struct _SelectionInfo {
    int done;
    WindowInfo* window;
    char* selection;
} SelectionInfo;

typedef struct {
    int direction;
    int searchType;
    int searchWrap;
} SearchSelectedCallData;

/* History mechanism for search and replace strings */
static char *SearchHistory[MAX_SEARCH_HISTORY];
static char *ReplaceHistory[MAX_SEARCH_HISTORY];
static int SearchTypeHistory[MAX_SEARCH_HISTORY];
static int HistStart = 0;
static int NHist = 0;

static int textFieldNonEmpty(Widget w);
static void setTextField(WindowInfo* window, Time time, Widget textField);
static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static void createReplaceDlog(Widget parent, WindowInfo *window);
static void createFindDlog(Widget parent, WindowInfo *window);
static void createReplaceMultiFileDlog(Widget parent, WindowInfo *window);
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
static void rFindTextValueChangedCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);

static void rSetActionButtons(WindowInfo* window,
                              int replaceBtn,
                              int replaceFindBtn,
                              int replaceAndFindBtn,
#ifndef REPLACE_SCOPE
                              int replaceInWinBtn,
                              int replaceInSelBtn,
#endif      
                              int replaceAllBtn);
#ifdef REPLACE_SCOPE
static void rScopeWinCB(Widget w, WindowInfo *window, 
	XmAnyCallbackStruct *callData);
static void rScopeSelCB(Widget w, WindowInfo *window, 
	XmAnyCallbackStruct *callData);
static void rScopeMultiCB(Widget w, WindowInfo *window, 
	XmAnyCallbackStruct *callData);
static void replaceAllScopeCB(Widget w, WindowInfo *window, 
	XmAnyCallbackStruct *callData);
#endif

static void replaceArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void fUpdateActionButtons(WindowInfo *window);
static void findTextValueChangedCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void findArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void replaceFindCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData);
static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData); 
static void replaceMultiFileCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void rMultiFileReplaceCB(Widget w, WindowInfo *window,  
       XmAnyCallbackStruct * callData);
static void rMultiFileCancelCB(Widget w, WindowInfo *window, caddr_t callData);
static void rMultiFileSelectAllCB(Widget w, WindowInfo *window, 
       XmAnyCallbackStruct *callData);
static void rMultiFileDeselectAllCB(Widget w, WindowInfo *window, 
       XmAnyCallbackStruct * callData);
static void rMultiFilePathCB(Widget w, WindowInfo *window,  
	XmAnyCallbackStruct *callData);
static void uploadFileListItems(WindowInfo* window, Bool replace);
static int countWritableWindows(const WindowInfo* window);
static void collectWritableWindows(WindowInfo* window);
static void freeWritableWindowsCB(Widget* w, WindowInfo* window,
                                  XmAnyCallbackStruct *callData);
static void checkMultiReplaceListForDoomedW(WindowInfo* window, 
                                                     WindowInfo* doomedWindow);
static void removeDoomedWindowFromList(WindowInfo* window, int index);
static void unmanageReplaceDialogs(WindowInfo *window);
static void flashTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void eraseFlash(WindowInfo *window);
static int getReplaceDlogInfo(WindowInfo *window, int *direction,
	char *searchString, char *replaceString, int *searchType);
static int getFindDlogInfo(WindowInfo *window, int *direction,
	char *searchString, int *searchType);
static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static void iSearchTextActivateCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void iSearchTextValueChangedCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void iSearchTextKeyEH(Widget w, WindowInfo *window,
	XKeyEvent *event, Boolean *continueDispatch);
static int searchLiteral(const char *string, const char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos,
	int *searchExtent);
static int searchLiteralWord(const char *string, const char *searchString, int caseSense,
 	int direction, int wrap, int beginPos, int *startPos, int *endPos, 
        const char * delimiters);
static int searchRegex(const char *string, const char *searchString, int direction,
	int wrap, int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags);
static int forwardRegexSearch(const char *string, const char *searchString, int wrap,
	int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags);
static int backwardRegexSearch(const char *string, const char *searchString, int wrap,
	int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags);
static void upCaseString(char *outString, const char *inString);
static void downCaseString(char *outString, const char *inString);
static void resetFindTabGroup(WindowInfo *window);
static void resetReplaceTabGroup(WindowInfo *window);
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right, int *searchExtent);
static int findMatchingChar(textBuffer *buf, char toMatch, int charPos,
	int startLimit, int endLimit, int *matchPos);
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen, int prevChar, char *delimiters, 
        int defaultFlags);
static void saveSearchHistory(char *searchString, char *replaceString,
	int searchType, int isIncremental);
static int historyIndex(int nCycles);
static char *searchTypeArg(int searchType);
static char *searchWrapArg(int searchWrap);
static char *directionArg(int direction);
static int isRegexType(int searchType);
static int defaultRegexFlags(int searchType);
static void findRegExpToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void replaceRegExpToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void iSearchRegExpToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void findCaseToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void replaceCaseToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void iSearchCaseToggleCB(Widget w, XtPointer clientData, 
	XtPointer callData);
static void iSearchTryBeepOnWrap(WindowInfo *window, int direction, 
      	int beginPos, int startPos); 
static void iSearchRecordLastBeginPos(WindowInfo *window, int direction, 
	int initPos); 

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

/*
** Shared routine for replace and find dialogs and i-search bar to initialize
** the state of the regex/case/word toggle buttons, and the sticky case 
** sensitivity states.
*/    
static void initToggleButtons(int searchType, Widget regexToggle,
                              Widget caseToggle, Widget* wordToggle,
                              Bool* lastLiteralCase,
                              Bool* lastRegexCase)
{
    /* Set the initial search type and remember the corresponding case
       sensitivity states in case sticky case sensitivity is required. */
    switch (searchType) {
      case SEARCH_LITERAL:
              *lastLiteralCase = False;
              *lastRegexCase   = True;
	      XmToggleButtonSetState(regexToggle, False, False);
	      XmToggleButtonSetState(caseToggle,  False, False);
	      if (wordToggle) {
		  XmToggleButtonSetState(*wordToggle, False, False);
                  XtSetSensitive(*wordToggle, True);
              }
      break;
      case SEARCH_CASE_SENSE:
              *lastLiteralCase = True;
              *lastRegexCase   = True;
	      XmToggleButtonSetState(regexToggle, False, False);
	      XmToggleButtonSetState(caseToggle,  True,  False);
	      if (wordToggle) {
                  XmToggleButtonSetState(*wordToggle, False, False);
                  XtSetSensitive(*wordToggle, True);
              }
      break;
      case SEARCH_LITERAL_WORD:
              *lastLiteralCase = False;
              *lastRegexCase   = True;
	      XmToggleButtonSetState(regexToggle, False, False);
	      XmToggleButtonSetState(caseToggle, False, False);
	      if (wordToggle) {
                  XmToggleButtonSetState(*wordToggle,  True,  False);
                  XtSetSensitive(*wordToggle, True);
              }
      break;
      case SEARCH_CASE_SENSE_WORD:
              *lastLiteralCase = True;
              *lastRegexCase   = True;
	      XmToggleButtonSetState(regexToggle, False, False);
	      XmToggleButtonSetState(caseToggle,  True,  False);
	      if (wordToggle) {
                  XmToggleButtonSetState(*wordToggle,  True,  False);
                  XtSetSensitive(*wordToggle, True);
              }
      break;
      case SEARCH_REGEX:
              *lastLiteralCase = False;
              *lastRegexCase   = True;
	      XmToggleButtonSetState(regexToggle, True,  False);
	      XmToggleButtonSetState(caseToggle,  True,  False);
	      if (wordToggle) {
                  XmToggleButtonSetState(*wordToggle,  False, False);
                  XtSetSensitive(*wordToggle, False);
              }
      break;
      case SEARCH_REGEX_NOCASE:
              *lastLiteralCase = False;
              *lastRegexCase   = False;
	      XmToggleButtonSetState(regexToggle, True,  False);
	      XmToggleButtonSetState(caseToggle,  False, False);
	      if (wordToggle) {
                  XmToggleButtonSetState(*wordToggle,  False, False);
                  XtSetSensitive(*wordToggle, False);
              }
      break;
    }
}

#ifdef REPLACE_SCOPE
/*
** Checks whether a selection spans multiple lines. Used to decide on the
** default scope for replace dialogs. 
** This routine introduces a dependency on textDisp.h, which is not so nice,
** but I currently don't have a cleaner solution.
*/
static int selectionSpansMultipleLines(WindowInfo *window)
{
    int selStart, selEnd, isRect, rectStart, rectEnd, lineStartStart,
        lineStartEnd;
    int lineWidth;
    textDisp *textD;
    
    if (!BufGetSelectionPos(window->buffer, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
    	return FALSE;

    /* This is kind of tricky. The perception of a line depends on the
       line wrap mode being used. So in theory, we should take into 
       account the layout of the text on the screen. However, the 
       routine to calculate a line number for a given character position
       (TextDPosToLineAndCol) only works for displayed lines, so we cannot
       use it. Therefore, we use this simple heuristic:
        - If a newline is found between the start and end of the selection,
	  we obviously have a multi-line selection.
	- If no newline is found, but the distance between the start and the
          end of the selection is larger than the number of characters 
	  displayed on a line, and we're in continuous wrap mode,
	  we also assume a multi-line selection.
    */
     
    lineStartStart = BufStartOfLine(window->buffer, selStart);
    lineStartEnd = BufStartOfLine(window->buffer, selEnd);
    /* If the line starts differ, we have a "\n" in between. */
    if (lineStartStart != lineStartEnd ) 
	return TRUE;     
    
    if (window->wrapMode != CONTINUOUS_WRAP)
	return FALSE; /* Same line */
	    
    /* Estimate the number of characters on a line */
    textD = ((TextWidget)window->textArea)->text.textD;
    if (textD->fontStruct->max_bounds.width > 0)
	lineWidth = textD->width / textD->fontStruct->max_bounds.width;
    else
	lineWidth = 1;
    if (lineWidth < 1) lineWidth = 1; /* Just in case */
    
    /* Estimate the numbers of line breaks from the start of the line to
       the start and ending positions of the selection and compare.*/
    if ((selStart-lineStartStart)/lineWidth !=
        (selEnd-lineStartStart)/lineWidth )
       return TRUE; /* Spans multiple lines */
       
    return FALSE; /* Small selection; probably doesn't span lines */
}
#endif

void DoFindReplaceDlog(WindowInfo *window, int direction, int searchType,
    int keepDialogs, Time time)
{

    /* Create the dialog if it doesn't already exist */
    if (window->replaceDlog == NULL)
    	createReplaceDlog(window->shell, window);
    
    setTextField(window, time, window->replaceText);

    /* If the window is already up, just pop it to the top */
    if (XtIsManaged(window->replaceDlog)) {
	RaiseShellWindow(XtParent(window->replaceDlog));
	return;
    }
    	
    /* Blank the Replace with field */
    XmTextSetString(window->replaceWithText, "");
        
    /* Set the initial search type */
    initToggleButtons(GetPrefSearch(), window->replaceRegexToggle,
                      window->replaceCaseToggle, &window->replaceWordToggle,
                      &window->replaceLastLiteralCase,
                      &window->replaceLastRegexCase);
    
    /* Set the initial direction based on the direction argument */
    XmToggleButtonSetState(window->replaceRevToggle, 
	direction == SEARCH_FORWARD ? False: True, True);
    
    /* Set the state of the Keep Dialog Up button */
    XmToggleButtonSetState(window->replaceKeepBtn, keepDialogs, True);
    
#ifdef REPLACE_SCOPE
    /* Set the state of the scope radio buttons to "In Window".
       Notify to make sure that callbacks are called. 
       NOTE: due to an apparent bug in OpenMotif, the radio buttons may
       get stuck after resetting the scope to "In Window". I currently 
       don't have a workaround. */
    if (window->wasSelected) {
	/* If a selection exists, the default scope depends on the preference
           of the user. */
	switch(GetPrefReplaceDefScope()) {
	   case REPL_DEF_SCOPE_SELECTION:
		/* The user prefers selection scope, no matter what the
		   size of the selection is. */	   
		XmToggleButtonSetState(window->replaceScopeSelToggle, 
		    True, True);
		break;
	   case REPL_DEF_SCOPE_SMART:
		if (selectionSpansMultipleLines(window)) {
		    /* If the selection spans multiple lines, the user most
		       likely wants to perform a replacement in the selection */
		    XmToggleButtonSetState(window->replaceScopeSelToggle, 
		    	True, True);
		}
		else {
		    /* It's unlikely that the user wants a replacement in a
		       tiny selection only. */
		    XmToggleButtonSetState(window->replaceScopeWinToggle, 
		    	True, True);
		}
		break;
	   default:
	   	/* The user always wants window scope as default. */
		XmToggleButtonSetState(window->replaceScopeWinToggle, 
		   True, True);
		break;
	}
    }
    else {
       /* No selection -> always choose "In Window" as default. */
       XmToggleButtonSetState(window->replaceScopeWinToggle, True, True);
    }

    /* multiple implies more than one */
    window->nWritableWindows = countWritableWindows(window);
    XtSetSensitive(window->replaceScopeMultiToggle, window->nWritableWindows > 1);
#else
    /* multiple implies more than one */
    window->nWritableWindows = countWritableWindows(window);
    XtSetSensitive(window->replaceAllBtn, window->nWritableWindows > 1);
#endif

    UpdateReplaceActionButtons(window);
    
    /* Start the search history mechanism at the current history item */
    window->rHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->replaceDlog);
    
    /* Workaround: LessTif (as of version 0.89) needs reminding of who had
       the focus when the dialog was unmanaged.  When re-managed, focus is
       lost and events fall through to the window below. */
    XmProcessTraversal(window->replaceText, XmTRAVERSE_CURRENT);
}

static void setTextField(WindowInfo *window, Time time, Widget textField)
{
    XEvent nextEvent;
    char *primary_selection = 0;
    SelectionInfo *selectionInfo = XtNew(SelectionInfo);

    if (GetPrefFindReplaceUsesSelection()) {
        selectionInfo->done = 0;
        selectionInfo->window = window;
        selectionInfo->selection = 0;
        XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
                            (XtSelectionCallbackProc)getSelectionCB, selectionInfo, time);
        while (selectionInfo->done == 0) {
            XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
            XtDispatchEvent(&nextEvent);
        }
        primary_selection = selectionInfo->selection;
    }
    if (primary_selection == 0) {
        primary_selection = XtNewString("");
    }

    /* Update the field */
    XmTextSetString(textField, primary_selection);

    XtFree(primary_selection);
    XtFree((char*)selectionInfo);
}    

static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
        Atom *type, char *value, int *length, int *format)
{
    WindowInfo *window = selectionInfo->window;

    /* return an empty string if we can't get the selection data */
    if (*type == XT_CONVERT_FAIL || *type != XA_STRING || value == NULL || *length == 0) {
        XtFree(value);
        selectionInfo->selection = 0;
        selectionInfo->done = 1;
        return;
    }
    /* return an empty string if the data is not of the correct format. */
    if (*format != 8) {
        DialogF(DF_WARN, window->shell, 1, "NEdit can't handle non 8-bit text", "OK");
        XtFree(value);
        selectionInfo->selection = 0;
        selectionInfo->done = 1;
        return;
    }
    selectionInfo->selection = XtMalloc(*length+1);
    memcpy(selectionInfo->selection, value, *length);
    selectionInfo->selection[*length] = 0;
    XtFree(value);
    selectionInfo->done = 1;
}

void DoFindDlog(WindowInfo *window, int direction, int searchType,
    int keepDialogs, Time time)
{

    /* Create the dialog if it doesn't already exist */
    if (window->findDlog == NULL)
    	createFindDlog(window->shell, window);
    
    setTextField(window, time, window->findText);

    /* If the window is already up, just pop it to the top */
    if (XtIsManaged(window->findDlog)) {
	RaiseShellWindow(XtParent(window->findDlog));
	return;
    }

    /* Set the initial search type */
    initToggleButtons(GetPrefSearch(), window->findRegexToggle,
                      window->findCaseToggle, &window->findWordToggle,
                      &window->findLastLiteralCase,
                      &window->findLastRegexCase);
  
    /* Set the initial direction based on the direction argument */
    XmToggleButtonSetState(window->findRevToggle,
	direction == SEARCH_FORWARD ? False : True, True);
    
    /* Set the state of the Keep Dialog Up button */
    XmToggleButtonSetState(window->findKeepBtn, keepDialogs, True);
    
    /* Set the state of the Find button */
    fUpdateActionButtons(window);

    /* start the search history mechanism at the current history item */
    window->fHistIndex = 0;
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->findDlog);
}

void DoReplaceMultiFileDlog(WindowInfo *window)
{
    char	searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int		direction, searchType;
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
  	return;
    
    /* Don't let the user select files when no replacement can be made */
    if (*searchString == '\0') {
       /* Set the initial focus of the dialog back to the search string */
       resetReplaceTabGroup(window);
       /* pop down the replace dialog */
       if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	   unmanageReplaceDialogs(window);
       return;
    }
    
    /* Create the dialog if it doesn't already exist */
    if (window->replaceMultiFileDlog == NULL)
    	createReplaceMultiFileDlog(window->shell, window);

    /* Raising the window doesn't make sense. It is modal, so we 
       can't get here unless it is unmanaged */
    /* Prepare a list of writable windows */
    collectWritableWindows(window);
    
    /* Initialize/update the list of files. */
    uploadFileListItems(window, False);
    
    /* Display the dialog */
    ManageDialogCenteredOnPointer(window->replaceMultiFileDlog);
}

/*
** If a window is closed (possibly via the window manager) while it is on the
** multi-file replace dialog list of any other window (or even the same one),
** we must update those lists or we end up with dangling references.
** Normally, there can be only one of those dialogs at the same time
** (application modal), but Lesstif doesn't (always) honor application
** modalness, so there can be more than one dialog. 
*/
void RemoveFromMultiReplaceDialog(WindowInfo *doomedWindow)
{
    WindowInfo *w;
    
    for (w=WindowList; w!=NULL; w=w->next) 
       if (w->writableWindows) 
          /* A multi-file replacement dialog is up for this window */
          checkMultiReplaceListForDoomedW(w, doomedWindow);
}

static void createReplaceDlog(Widget parent, WindowInfo *window)
{
    Arg    	args[50];
    int    	argcnt, defaultBtnOffset;
    XmString	st1;
    Widget	form, btnForm;
#ifdef REPLACE_SCOPE
    Widget	scopeForm, replaceAllBtn;
#else
    Widget	label3, allForm;
#endif
    Widget	inWinBtn, inSelBtn, inMultiBtn;
    Widget    	searchTypeBox;
    Widget    	label2, label1, label, replaceText, findText;
    Widget    	findBtn,  cancelBtn, replaceBtn;
    Widget    	replaceFindBtn;
    Widget	searchDirBox, reverseBtn, keepBtn;
    char 	title[MAXPATHLEN + 19];
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    form = CreateFormDialog(parent, "replaceDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
    if (GetPrefKeepSearchDlogs()) {
    	sprintf(title, "Replace/Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(form), XmNtitle, title, NULL);
    } else
    	XtVaSetValues(XtParent(form), XmNtitle, "Replace/Find", NULL);

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
    XtAddCallback(findText, XmNvalueChangedCallback, 
      (XtCallbackProc)rFindTextValueChangedCB, window);
    XtAddEventHandler(findText, KeyPressMask, False,
    	    (XtEventHandler)rFindArrowKeyCB, window);
    RemapDeleteKey(findText);
    XtManageChild(findText);
    XmAddTabGroup(findText);
    XtVaSetValues(label1, XmNuserData, findText, NULL); /* mnemonic processing */
 
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
    XtVaSetValues(label, XmNuserData, replaceText, NULL); /* mnemonic processing */

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
    searchTypeBox = XmCreateRowColumn(form, "searchTypeBox", args, argcnt);
    XtManageChild(searchTypeBox);
    XmAddTabGroup(searchTypeBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, 
    	     st1=MKSTRING("Regular Expression")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    window->replaceRegexToggle = XmCreateToggleButton(searchTypeBox, "regExp", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->replaceRegexToggle);
    XtAddCallback(window->replaceRegexToggle, XmNvalueChangedCallback, (XtCallbackProc) replaceRegExpToggleCB, window);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Case Sensitive")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    window->replaceCaseToggle = XmCreateToggleButton(searchTypeBox, "caseSensitive", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->replaceCaseToggle);
    XtAddCallback(window->replaceCaseToggle, XmNvalueChangedCallback, (XtCallbackProc) replaceCaseToggleCB, window);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Whole Word")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'h'); argcnt++;
    window->replaceWordToggle = XmCreateToggleButton(searchTypeBox, "wholeWord", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->replaceWordToggle);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, False); argcnt++;
    searchDirBox = XmCreateRowColumn(form, "searchDirBox", args, argcnt);
    XtManageChild(searchDirBox);
    XmAddTabGroup(searchDirBox);
 
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
    
#ifdef REPLACE_SCOPE
    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchDirBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    scopeForm = XmCreateRowColumn(form, "scope", args, argcnt);
    XtManageChild(scopeForm);
    XmAddTabGroup(scopeForm);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("In Window")); 
        argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'i'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    inWinBtn = XmCreateToggleButton(scopeForm, "inWindow", args, argcnt);
    XtAddCallback(inWinBtn, XmNvalueChangedCallback, 
    	(XtCallbackProc)rScopeWinCB, window);
    XmStringFree(st1);
    XtManageChild(inWinBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("In Selection")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, inWinBtn); argcnt++;
    inSelBtn = XmCreateToggleButton(scopeForm, "inSel", args, argcnt);
    XtAddCallback(inSelBtn, XmNvalueChangedCallback, 
	(XtCallbackProc)rScopeSelCB, window);
    XmStringFree(st1);
    XtManageChild(inSelBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("In Multiple Files")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'M'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, inSelBtn); argcnt++;
    inMultiBtn = XmCreateToggleButton(scopeForm, "multiFile", args, argcnt);
    XtAddCallback(inMultiBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)rScopeMultiCB, window);
    XmStringFree(st1);
    XtManageChild(inMultiBtn);
#else
    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchDirBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    allForm = XmCreateForm(form, "all", args, argcnt);
    XtManageChild(allForm);
    XmAddTabGroup(allForm);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace all in:"));
    	    argcnt++;
    label3 = XmCreateLabel(allForm, "label3", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label3);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Window")); 
        argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'i'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, label3); argcnt++;
    inWinBtn = XmCreatePushButton(allForm, "inWindow", args, argcnt);
    XtAddCallback(inWinBtn, XmNactivateCallback, 
    	(XtCallbackProc)replaceAllCB, window);
    XmStringFree(st1);
    XtManageChild(inWinBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Selection")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, inWinBtn); argcnt++;
    inSelBtn = XmCreatePushButton(allForm, "inSel", args, argcnt);
    XtAddCallback(inSelBtn, XmNactivateCallback, 
	(XtCallbackProc)rInSelCB, window);
    XmStringFree(st1);
    XtManageChild(inSelBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Multiple Files...")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'M'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, inSelBtn); argcnt++;
    inMultiBtn = XmCreatePushButton(allForm, "multiFile", args, argcnt);
    XtAddCallback(inMultiBtn, XmNactivateCallback,
    	    (XtCallbackProc)replaceMultiFileCB, window);
    XmStringFree(st1);
    XtManageChild(inMultiBtn);
    
#endif
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
#ifdef REPLACE_SCOPE
    XtSetArg(args[argcnt], XmNtopWidget, scopeForm); argcnt++;
#else
    XtSetArg(args[argcnt], XmNtopWidget, allForm); argcnt++;
#endif
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
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
#ifdef REPLACE_SCOPE
    XtSetArg(args[argcnt], XmNleftPosition, 0); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 21); argcnt++;
#else
    XtSetArg(args[argcnt], XmNleftPosition, 0); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 25); argcnt++;
#endif
    replaceBtn = XmCreatePushButton(btnForm, "replace", args, argcnt);
    XtAddCallback(replaceBtn, XmNactivateCallback, (XtCallbackProc)replaceCB, window);
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
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
#ifdef REPLACE_SCOPE
    XtSetArg(args[argcnt], XmNleftPosition, 21); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 33); argcnt++;
#else
    XtSetArg(args[argcnt], XmNleftPosition, 25); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 42); argcnt++;
#endif
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, defaultBtnOffset); argcnt++;
    findBtn = XmCreatePushButton(btnForm, "find", args, argcnt);
    XtAddCallback(findBtn, XmNactivateCallback, (XtCallbackProc)rFindCB,  window);
    XmStringFree(st1);
    XtManageChild(findBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace & Find")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'n'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
#ifdef REPLACE_SCOPE
    XtSetArg(args[argcnt], XmNleftPosition, 33); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 62); argcnt++;
#else
    XtSetArg(args[argcnt], XmNleftPosition, 42); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 79); argcnt++;
#endif
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, defaultBtnOffset); argcnt++;
    replaceFindBtn = XmCreatePushButton(btnForm, "replacefind", args, argcnt);
    XtAddCallback(replaceFindBtn, XmNactivateCallback, (XtCallbackProc)replaceFindCB, window);
    XmStringFree(st1);
    XtManageChild(replaceFindBtn);
 
#ifdef REPLACE_SCOPE    
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
    XtSetArg(args[argcnt], XmNleftPosition, 62); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 85); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    replaceAllBtn = XmCreatePushButton(btnForm, "all", args, argcnt);
    XtAddCallback(replaceAllBtn, XmNactivateCallback,
    	    (XtCallbackProc)replaceAllScopeCB, window);
    XmStringFree(st1);
    XtManageChild(replaceAllBtn);
#endif
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Cancel")); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
#ifdef REPLACE_SCOPE
    XtSetArg(args[argcnt], XmNleftPosition, 85); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 100); argcnt++;
#else
    XtSetArg(args[argcnt], XmNleftPosition, 79); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 100); argcnt++;
#endif
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, defaultBtnOffset); argcnt++;
    cancelBtn = XmCreatePushButton(btnForm, "cancel", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(cancelBtn, XmNactivateCallback, (XtCallbackProc)rCancelCB,
    	    window);
    XtManageChild(cancelBtn);

    XtVaSetValues(form, XmNcancelButton, cancelBtn, NULL);
    AddDialogMnemonicHandler(form, FALSE);
    
    window->replaceDlog = form;
    window->replaceText = findText;
    window->replaceWithText = replaceText;
    window->replaceRevToggle = reverseBtn;
    window->replaceKeepBtn = keepBtn;
    window->replaceBtns = btnForm;
    window->replaceBtn = replaceBtn;
    window->replaceAndFindBtn = replaceFindBtn;
    window->replaceFindBtn = findBtn;
    window->replaceSearchTypeBox = searchTypeBox;
#ifdef REPLACE_SCOPE
    window->replaceAllBtn = replaceAllBtn;
    window->replaceScopeWinToggle = inWinBtn;
    window->replaceScopeSelToggle = inSelBtn;
    window->replaceScopeMultiToggle = inMultiBtn;
#else
    window->replaceInWinBtn = inWinBtn;
    window->replaceAllBtn = inMultiBtn;
    window->replaceInSelBtn = inSelBtn;
#endif
}

static void createFindDlog(Widget parent, WindowInfo *window)
{
    Arg    	args[50];
    int    	argcnt, defaultBtnOffset;
    XmString	st1;
    Widget	form, btnForm, searchTypeBox;
    Widget	findText, label1, label2, cancelBtn, findBtn;
    Widget	searchDirBox, reverseBtn, keepBtn;
    char 	title[MAXPATHLEN + 11];
    Dimension	shadowThickness;
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    form = CreateFormDialog(parent, "findDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
    if (GetPrefKeepSearchDlogs()) {
    	sprintf(title, "Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(form), XmNtitle, title, NULL);
    } else
    	XtVaSetValues(XtParent(form), XmNtitle, "Find", NULL);
 
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
    XtAddCallback(findText, XmNvalueChangedCallback, 
      (XtCallbackProc)findTextValueChangedCB, window);
    XtAddEventHandler(findText, KeyPressMask, False,
    	    (XtEventHandler)findArrowKeyCB, window);
    RemapDeleteKey(findText);
    XtManageChild(findText);
    XmAddTabGroup(findText);
    XtVaSetValues(label1, XmNuserData, findText, NULL); /* mnemonic processing */

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
    
    searchTypeBox = XmCreateRowColumn(form, "searchTypeBox", args, argcnt);
    XtManageChild(searchTypeBox);
    XmAddTabGroup(searchTypeBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, 
    	     st1=MKSTRING("Regular Expression")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    window->findRegexToggle = XmCreateToggleButton(searchTypeBox, "regExp", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->findRegexToggle);
    XtAddCallback(window->findRegexToggle, XmNvalueChangedCallback, (XtCallbackProc) findRegExpToggleCB, window);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Case Sensitive")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    window->findCaseToggle = XmCreateToggleButton(searchTypeBox, "caseSensitive", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->findCaseToggle);
    XtAddCallback(window->findCaseToggle, XmNvalueChangedCallback, (XtCallbackProc) findCaseToggleCB, window);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Whole Word")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'h'); argcnt++;
    window->findWordToggle = XmCreateToggleButton(searchTypeBox, "wholeWord", args, argcnt);
    XmStringFree(st1);
    XtManageChild(window->findWordToggle);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, False); argcnt++;
    searchDirBox = XmCreateRowColumn(form, "searchDirBox", args, argcnt);
    XtManageChild(searchDirBox);
    XmAddTabGroup(searchDirBox);
    
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
    XtVaGetValues(findBtn, XmNshadowThickness, &shadowThickness, NULL);
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
    XtVaSetValues(form, XmNcancelButton, cancelBtn, NULL);
    AddDialogMnemonicHandler(form, FALSE);
    
    window->findDlog = form;
    window->findText = findText;
    window->findRevToggle = reverseBtn;
    window->findKeepBtn = keepBtn;
    window->findBtns = btnForm;
    window->findBtn = findBtn;
    window->findSearchTypeBox = searchTypeBox;
}

static void createReplaceMultiFileDlog(Widget parent, WindowInfo *window) 
{
    Arg		args[50];
    int		argcnt, defaultBtnOffset;
    XmString	st1;
    Widget	list, label1, form, pathBtn;
    Widget	btnForm, replaceBtn, selectBtn, deselectBtn, cancelBtn;
    Dimension	shadowThickness;
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    XtSetArg (args[argcnt], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	    argcnt ++;

    /* Ideally, we should create the multi-file dialog as a child widget
       of the replace dialog. However, if we do this, the main window
       can hide the multi-file dialog when raised (I'm not sure why, but 
       it's something that I observed with fvwm). By using the main window
       as the parent, it is possible that the replace dialog _partially_
       covers the multi-file dialog, but this much better than the multi-file
       dialog being covered completely by the main window */
    form = CreateFormDialog(window->shell, "replaceMultiFileDialog", 
           			     args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
    XtVaSetValues(XtParent(form), XmNtitle, "Replace All in Multiple Files", 
		  NULL);
    
    /* Label at top left. */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    /* Offset = 6 + (highlightThickness + detailShadowThickness) of the
       toggle button (see below). Unfortunately, detailShadowThickness is
       a Motif 2.x property, so we can't measure it. The default is 2 pixels.
       To make things even more complicated, the SunOS 5.6 / Solaris 2.6 
       version of Motif 1.2 seems to use a detailShadowThickness of 0 ...
       So we'll have to live with a slight misalignment on that platform
       (those Motif libs are known to have many other problems). */
    XtSetArg(args[argcnt], XmNtopOffset, 10); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, 
       st1=MKSTRING("Files in which to Replace All:")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'F'); argcnt++;
    label1 = XmCreateLabel(form, "label1", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label1);
    
    /* Pathname toggle button at top right (always unset by default) */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNset, False); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_END); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Show Path Names")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'P'); argcnt++;
    pathBtn = XmCreateToggleButton(form, "path", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(pathBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)rMultiFilePathCB, window);
    XtManageChild(pathBtn);
    
    /*
     * Buttons at bottom. Place them before the list, such that we can
     * attach the list to the label and the button box. In that way only
     * the lists resizes vertically when the dialog is resized; users expect
     * the list to resize, not the buttons.
     */
     
    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNresizable, (short)0); argcnt++;
    btnForm = XmCreateForm(form, "buttons", args, argcnt);
    XtManageChild(btnForm);
    
    /* Replace */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace")); argcnt++;
    XtSetArg(args[argcnt], XmNshowAsDefault, (short)1); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 0); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 25); argcnt++;
    replaceBtn = XmCreatePushButton(btnForm, "replace", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(replaceBtn, XmNactivateCallback,
       (XtCallbackProc)rMultiFileReplaceCB, window);
    /*
     * _DON'T_ set the replace button as default (as in other dialogs).
     * Multi-selection lists have the nasty property of selecting the 
     * current item when <enter> is pressed.
     * In that way, the user could inadvertently select an additional file
     * (most likely the last one that was deselected). 
     * The user has to activate the replace button explictly (either with
     * a mouse click or with the shortcut key).
     *
     * XtVaSetValues(form, XmNdefaultButton, replaceBtn, NULL); */
     
    XtManageChild(replaceBtn);
    XtVaGetValues(replaceBtn, XmNshadowThickness, &shadowThickness, NULL);
    defaultBtnOffset = shadowThickness + 4;

    /* Select All */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Select All")); 
       argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 25); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 50); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    selectBtn = XmCreatePushButton(btnForm, "select", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(selectBtn, XmNactivateCallback,
       (XtCallbackProc)rMultiFileSelectAllCB, window);
    XtManageChild(selectBtn);

    /* Deselect All */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Deselect All")); 
       argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'D'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 50); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 75); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    deselectBtn = XmCreatePushButton(btnForm, "deselect", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(deselectBtn, XmNactivateCallback,
       (XtCallbackProc)rMultiFileDeselectAllCB, window);
    XtManageChild(deselectBtn);

    /* Cancel */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Cancel")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION); argcnt++;
    XtSetArg(args[argcnt], XmNleftPosition, 75); argcnt++;
    XtSetArg(args[argcnt], XmNrightPosition, 100); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, defaultBtnOffset); argcnt++;
    cancelBtn = XmCreatePushButton(btnForm, "cancel", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(cancelBtn, XmNactivateCallback, 
       (XtCallbackProc)rMultiFileCancelCB, window);
    XtManageChild(cancelBtn);
    
    /* The list of files */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomWidget, btnForm); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label1); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 10); argcnt++;
    XtSetArg(args[argcnt], XmNvisibleItemCount, 10); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 10); argcnt++;
    /* An alternative is to use the EXTENDED_SELECT, but that one
       is less suited for keyboard manipulation (moving the selection cursor
       with the keyboard deselects everything). */
    XtSetArg(args[argcnt], XmNselectionPolicy, XmMULTIPLE_SELECT); argcnt++;
    list = XmCreateScrolledList(form, "list_of_files", args, argcnt);
    XtManageChild(list);
    
    /* Traverse: list -> buttons -> path name toggle button */
    XmAddTabGroup(list);
    XmAddTabGroup(btnForm);
    XmAddTabGroup(pathBtn);
    
    XtVaSetValues(label1, XmNuserData, list, NULL); /* mnemonic processing */
    
    /* Cancel/Mnemonic stuff. */
    XtVaSetValues(form, XmNcancelButton, cancelBtn, NULL);
    AddDialogMnemonicHandler(form, FALSE);
    
    window->replaceMultiFileDlog = form;
    window->replaceMultiFileList = list;
    window->replaceMultiFilePathBtn = pathBtn;
       
    /* Install a handler that frees the list of writable windows when
       the dialog is unmapped. */
    XtAddCallback(form, XmNunmapCallback, 
	    	    (XtCallbackProc)freeWritableWindowsCB, window); 
} 

/*
** Iterates through the list of writable windows of a window, and removes
** the doomed window if necessary.
*/
static void checkMultiReplaceListForDoomedW(WindowInfo* window, 
						     WindowInfo* doomedWindow)
{
    WindowInfo        *w;
    int               i;

    /* If the window owning the list and the doomed window are one and the
       same, we just close the multi-file replacement dialog. */
    if (window == doomedWindow) {
       XtUnmanageChild(window->replaceMultiFileDlog);
       return;
    }

    /* Check whether the doomed window is currently listed */
    for (i = 0; i < window->nWritableWindows; ++i) {
      w = window->writableWindows[i];
      if (w == doomedWindow) {
          removeDoomedWindowFromList(window, i);
          break;
      } 
    }   
}

/*
** Removes a window that is about to be closed from the list of files in
** which to replace. If the list becomes empty, the dialog is popped down.
*/
static void removeDoomedWindowFromList(WindowInfo* window, int index)
{
    int       entriesToMove;

    /* If the list would become empty, we remove the dialog */
    if (window->nWritableWindows <= 1) {
      XtUnmanageChild(window->replaceMultiFileDlog);
      return;
    }

    entriesToMove = window->nWritableWindows - index - 1;
    memmove(&(window->writableWindows[index]),
            &(window->writableWindows[index+1]),
            (size_t)(entriesToMove*sizeof(WindowInfo*)));
    window->nWritableWindows -= 1;
    
    XmListDeletePos(window->replaceMultiFileList, index + 1);
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
    char title[MAXPATHLEN + 19];

    if (XmToggleButtonGetState(w)) {
    	sprintf(title, "Replace/Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, NULL);
    } else
    	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, "Replace/Find", NULL);
}
static void fKeepCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    char title[MAXPATHLEN + 11];

    if (XmToggleButtonGetState(w)) {
    	sprintf(title, "Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->findDlog), XmNtitle, title, NULL);
    } else
    	XtVaSetValues(XtParent(window->findDlog), XmNtitle, "Find", NULL);
}

static void replaceCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[5];
    
    /* Validate and fetch the find and replace strings from the dialog */
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
    params[4] = searchWrapArg(GetPrefSearchWraps());
    XtCallActionProc(window->lastFocus, "replace", callData->event, params, 5);
    
    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	unmanageReplaceDialogs(window);
}

static void replaceAllCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace_all", callData->event,
    	    params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	unmanageReplaceDialogs(window);
}

static void replaceMultiFileCB(Widget w, WindowInfo *window,
				   XmAnyCallbackStruct *callData) 
{
   DoReplaceMultiFileDlog(window);
}

/*
** Callback that frees the list of windows the multi-file replace
** dialog is unmapped.
**/
static void freeWritableWindowsCB(Widget* w, WindowInfo* window,
                                  XmAnyCallbackStruct *callData)
{
    XtFree((XtPointer)window->writableWindows);
    window->writableWindows = NULL;
    window->nWritableWindows = 0;
}

/*
** Comparison function for sorting windows by title for the window menu
*/
static int compareWindowNames(const void *windowA, const void *windowB)
{
      return strcmp((*((WindowInfo**)windowA))->filename,
      	    (*((WindowInfo**)windowB))->filename);
}
 
/* Count no. of writable windows */
static int countWritableWindows(const WindowInfo* window)
{
    int nWritable;
    const WindowInfo *w;

    for (w=WindowList, nWritable=0; w!=NULL; w=w->next)
       if (!IS_ANY_LOCKED(w->lockReasons)) ++nWritable;
    
    return(nWritable);
}

/*
** Collects a list of writable windows (sorted by file name). 
** The previous list, if any is freed first. 
**/
static void collectWritableWindows(WindowInfo* window)
{
    int nWritable = countWritableWindows(window);
    int i;
    WindowInfo *w;
    WindowInfo **windows;
    
    if (window->writableWindows)
    {
       XtFree((XtPointer)window->writableWindows);
    }
   
    /* Make a sorted list of writable windows */
    windows = (WindowInfo **)XtMalloc(sizeof(WindowInfo *) * nWritable);
    for (w=WindowList, i=0; w!=NULL; w=w->next)
       if (!IS_ANY_LOCKED(w->lockReasons)) windows[i++] = w;
    qsort(windows, nWritable, sizeof(WindowInfo *), compareWindowNames);
    
    window->writableWindows = windows;
    window->nWritableWindows = nWritable;
} 

static void rMultiFileReplaceCB(Widget w, WindowInfo *window, 
   XmAnyCallbackStruct *callData) 
{
    char 	searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int 	direction, searchType;
    char 	*params[4];
    int 	nSelected, i;
    WindowInfo 	*writableWin;
    Bool 	replaceFailed, noWritableLeft;

    nSelected = 0;
    for (i=0; i<window->nWritableWindows; ++i)
       if (XmListPosSelected(window->replaceMultiFileList, i+1))
          ++nSelected;

    if (!nSelected) {
       DialogF(DF_INF, XtParent(window->replaceMultiFileDlog), 1,
  	   	  "No files selected!", "OK");
       return; /* Give the user another chance */
    }

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    /*
     * Protect the user against him/herself; Maybe this is a bit too much?
     */
    if (DialogF(DF_QUES, window->shell, 2,
	        "Multi-file replacements are difficult to undo.\n"
                "Proceed with the replacement ?", "Yes", "Cancel") != 1) {

       /* pop down the multi-file dialog only */
       XtUnmanageChild(window->replaceMultiFileDlog);

       return;
    }

    /* Fetch the find and replace strings from the dialog; 
       they should have been validated already, but since Lesstif may not
       honor modal dialogs, it is possible that the user modified the 
       strings again, so we should verify them again too. */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
 	 		  &searchType))
	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);

    replaceFailed = True;
    noWritableLeft = True;
    /* Perform the replacements and mark the selected files (history) */
    for (i=0; i<window->nWritableWindows; ++i) {
	writableWin = window->writableWindows[i];
	if (XmListPosSelected(window->replaceMultiFileList, i+1)) {
	/* First check again whether the file is still writable. If the
	   file status has changed or the file was locked in the mean time
	   (possible due to Lesstif modal dialog bug), we just skip the 
	   window. */
	    if (!IS_ANY_LOCKED(writableWin->lockReasons)) {
		noWritableLeft = False;
		writableWin->multiFileReplSelected = True;
		writableWin->multiFileBusy = True; /* Avoid multi-beep/dialog */
		writableWin->replaceFailed = False;
		XtCallActionProc(writableWin->lastFocus, "replace_all",
		    callData->event, params, 3);
		writableWin->multiFileBusy = False;
		if (!writableWin->replaceFailed) replaceFailed = False;
	    }
	} else {
	    writableWin->multiFileReplSelected = False;
	}
    }                          

    if (!XmToggleButtonGetState(window->replaceKeepBtn)) {
       /* Pop down both replace dialogs. */
       unmanageReplaceDialogs(window);
    } else {
       /* pow down only the file selection dialog */
       XtUnmanageChild(window->replaceMultiFileDlog);
    }
    
    /* We suppressed multiple beeps/dialogs. If there wasn't any file in
       which the replacement succeeded, we should still warn the user */
    if (replaceFailed) {
	if (GetPrefSearchDlogs()) {
	    if (noWritableLeft) {
		DialogF(DF_INF, window->shell, 1, 
			"All selected files have become read-only.", "OK");
	    } else {
		DialogF(DF_INF, window->shell, 1, 
			"String was not found", "OK");
            }
	} else {
           XBell(TheDisplay, 0);
        }
    }
}

static void rMultiFileCancelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* pop down the multi-window replace dialog */
    XtUnmanageChild(window->replaceMultiFileDlog);
}

static void rMultiFileSelectAllCB(Widget w, WindowInfo *window, 
   XmAnyCallbackStruct *callData) 
{
    int i;
    char policy;
    Widget list = window->replaceMultiFileList;
    
    /*
     * If the list is in extended selection mode, we can't select more 
     * than one item (probably because XmListSelectPos is equivalent 
     * to a button1 click; I don't think that there is an equivalent 
     * for CTRL-button1). Therefore, we temporarily put the list into 
     * multiple selection mode.
     * Note: this is not really necessary if the list is in multiple select
     *       mode all the time (as it currently is). 
     */
    XtVaGetValues(list, XmNselectionPolicy, &policy, NULL);
    XtVaSetValues(list, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
    
    /* Is there no other way (like "select all") ? */
    XmListDeselectAllItems(window->replaceMultiFileList); /* select toggles */
    
    for (i=0; i<window->nWritableWindows; ++i) {
       XmListSelectPos(list, i+1, FALSE);
    }
    
    /* Restore the original policy. */
    XtVaSetValues(list, XmNselectionPolicy, policy, NULL);
}

static void rMultiFileDeselectAllCB(Widget w, WindowInfo *window,  
   XmAnyCallbackStruct *callData) 
{
   XmListDeselectAllItems(window->replaceMultiFileList);
}

static void rMultiFilePathCB(Widget w, WindowInfo *window,  
   XmAnyCallbackStruct *callData) 
{
   uploadFileListItems(window, True);  /* Replace */
}

/*
 * Uploads the file items to the multi-file replament dialog list.
 * A boolean argument indicates whether the elements currently in the 
 * list have to be replaced or not.
 * Depending on the state of the "Show path names" toggle button, either
 * the file names or the path names are listed.
 */
static void uploadFileListItems(WindowInfo* window, Bool replace)
{
    XmStringTable names;
    int           nWritable, i, *selected, selectedCount;
    char          buf[MAXPATHLEN+1], policy;
    Bool          usePathNames;
    WindowInfo    *w;
    Widget        list;

    nWritable = window->nWritableWindows;
    list = window->replaceMultiFileList;
    
    names = (XmStringTable) XtMalloc(nWritable * sizeof(XmString*));
    
    usePathNames = XmToggleButtonGetState(window->replaceMultiFilePathBtn);
    
    /* Note: the windows are sorted alphabetically by _file_ name. This
             order is _not_ changed when we switch to path names. That
             would be confusing for the user */
    
    for (i = 0; i < nWritable; ++i) {
       w = window->writableWindows[i];
       if (usePathNames && window->filenameSet) {
          sprintf(buf, "%s%s", w->path, w->filename);
       } else {
          sprintf(buf, "%s", w->filename);
       }
       names[i] = XmStringCreateSimple(buf);
    }
    
    /*
     * If the list is in extended selection mode, we can't pre-select 
     * more than one item in (probably because XmListSelectPos is 
     * equivalent to a button1 click; I don't think that there is an 
     * equivalent for CTRL-button1). Therefore, we temporarily put the 
     * list into multiple selection mode.
     */
    XtVaGetValues(list, XmNselectionPolicy, &policy, NULL);
    XtVaSetValues(list, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
    if (replace) {
       /* Note: this function is obsolete in Motif 2.x, but it is available
                for compatibility reasons */
       XmListGetSelectedPos(list, &selected,  &selectedCount);
       
       XmListReplaceItemsPos(list, names, nWritable, 1);
       
       /* Maintain the selections */
       XmListDeselectAllItems(list);
       for (i = 0; i < selectedCount; ++i) {
          XmListSelectPos(list, selected[i], False);
       }
       
       XtFree((XtPointer)selected);
    } else {
       Arg args[1];
       int nVisible;
       int firstSelected = 0;
       
       /* Remove the old list, if any */
       XmListDeleteAllItems(list);
       
       /* Initial settings */
       XmListAddItems(list, names, nWritable, 1);
       
       /* Pre-select the files from the last run. */   
       selectedCount = 0;
       for (i = 0; i < nWritable; ++i) {
          if (window->writableWindows[i]->multiFileReplSelected) {
             XmListSelectPos(list, i+1, False);
             ++selectedCount;
             /* Remember the first selected item */
             if (firstSelected == 0) firstSelected = i+1;
          }
       }
       /* If no files are selected, we select them all. Normally this only
          happens the first time the dialog is used, but it looks "silly" 
          if the dialog pops up with nothing selected. */
       if (selectedCount == 0) {
          for (i = 0; i < nWritable; ++i) {
             XmListSelectPos(list, i+1, False);
          }
          firstSelected = 1;
       }
       
       /* Make sure that the first selected item is visible; otherwise, the
          user could get the impression that nothing is selected. By
          visualizing at least the first selected item, the user will more
          easily be confident that the previous selection is still active. */
       XtSetArg(args[0], XmNvisibleItemCount, &nVisible);
       XtGetValues(list, args, 1);
       /* Make sure that we don't create blank lines at the bottom by
          positioning too far. */
       if (nWritable <= nVisible) {
          /* No need to shift the visible position */
          firstSelected = 1;
       }
       else {
          int maxFirst = nWritable - nVisible + 1;
          if (firstSelected > maxFirst)
             firstSelected = maxFirst;
       }
       XmListSetPos(list, firstSelected);
    }
    
    /* Put the list back into its original selection policy. */
    XtVaSetValues(list, XmNselectionPolicy, policy, NULL);
    
    for (i = 0; i < nWritable; ++i)
       XmStringFree(names[i]);
    XtFree((XtPointer)names);
}

/*
** Unconditionally pops down the replace dialog and the
** replace-in-multiple-files dialog, if it exists.
*/
static void unmanageReplaceDialogs(WindowInfo *window)
{
    /* If the replace dialog goes down, the multi-file replace dialog must
       go down too */
    if (window->replaceMultiFileDlog &&
      XtIsManaged(window->replaceMultiFileDlog)) {
          XtUnmanageChild(window->replaceMultiFileDlog);
    }
        
    if (window->replaceDlog &&
      XtIsManaged(window->replaceDlog)) {
          XtUnmanageChild(window->replaceDlog);
    }
}

static void rInSelCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX], replaceString[SEARCHMAX];
    int direction, searchType;
    char *params[3];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace_in_selection",
    	    callData->event, params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	unmanageReplaceDialogs(window);
}

static void rCancelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* pop down the dialog */
    unmanageReplaceDialogs(window);
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
    char *params[4];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);
    
    /* Find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    params[3] = searchWrapArg(GetPrefSearchWraps());
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 4);
    
    /* Doctor the search history generated by the action to include the
       replace string (if any), so the replace string can be used on
       subsequent replaces, even though no actual replacement was done. */
    if (historyIndex(1) != -1 &&
    		!strcmp(SearchHistory[historyIndex(1)], searchString)) {
	XtFree(ReplaceHistory[historyIndex(1)]);
	ReplaceHistory[historyIndex(1)] = XtNewString(replaceString);
    }

    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	unmanageReplaceDialogs(window);
}

static void replaceFindCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[4];
    
    /* Validate and fetch the find and replace strings from the dialog */
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
    XtCallActionProc(window->lastFocus, "replace_find", callData->event, params, 4);
    
    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	unmanageReplaceDialogs(window);
}

static void rSetActionButtons(WindowInfo* window,
                              int replaceBtn,
                              int replaceFindBtn,
                              int replaceAndFindBtn,
#ifndef REPLACE_SCOPE
                              int replaceInWinBtn,
                              int replaceInSelBtn,
#endif      
                              int replaceAllBtn)
{
    XtSetSensitive(window->replaceBtn,        replaceBtn);
    XtSetSensitive(window->replaceFindBtn,    replaceFindBtn);
    XtSetSensitive(window->replaceAndFindBtn, replaceAndFindBtn);
#ifndef REPLACE_SCOPE
    XtSetSensitive(window->replaceInWinBtn, replaceInWinBtn);
    XtSetSensitive(window->replaceInSelBtn, replaceInSelBtn);
#endif
    XtSetSensitive(window->replaceAllBtn,     replaceAllBtn);
} 

void UpdateReplaceActionButtons(WindowInfo* window)
{
    /* Is there any text in the search for field */
    int searchText = textFieldNonEmpty(window->replaceText);
#ifdef REPLACE_SCOPE
    switch (window->replaceScope)
    {
        case REPL_SCOPE_WIN:
	    /* Enable all buttons, if there is any text in the search field. */
	    rSetActionButtons(window, searchText, searchText, searchText, searchText);
            break;

        case REPL_SCOPE_SEL:
	    /* Only enable Replace All, if a selection exists and text in search field. */
	    rSetActionButtons(window, False, False, False, searchText && window->wasSelected);
            break;

        case REPL_SCOPE_MULTI:
	    /* Only enable Replace All, if text in search field. */
	    rSetActionButtons(window, False, False, False, searchText);
            break;
    }
#else
    rSetActionButtons(window, searchText, searchText, searchText,
                      searchText, searchText && window->wasSelected,
                      searchText && (countWritableWindows(window) > 1));
#endif
}

#ifdef REPLACE_SCOPE
/*
** The next 3 callback adapt the sensitivity of the replace dialog push 
** buttons to the state of the scope radio buttons.
*/
static void rScopeWinCB(Widget w, WindowInfo *window, 
    XmAnyCallbackStruct *callData)
{
    if (XmToggleButtonGetState(window->replaceScopeWinToggle)) {
	window->replaceScope = REPL_SCOPE_WIN;
        UpdateReplaceActionButtons(window);
    }
}

static void rScopeSelCB(Widget w, WindowInfo *window, 
    XmAnyCallbackStruct *callData)
{
    if (XmToggleButtonGetState(window->replaceScopeSelToggle)) {
	window->replaceScope = REPL_SCOPE_SEL;
        UpdateReplaceActionButtons(window);
    }
}

static void rScopeMultiCB(Widget w, WindowInfo *window, 
    XmAnyCallbackStruct *callData)
{
    if (XmToggleButtonGetState(window->replaceScopeMultiToggle)) {
	window->replaceScope = REPL_SCOPE_MULTI;
        UpdateReplaceActionButtons(window);
    }
}

/*
** This routine dispatches a push on the replace-all button to the appropriate
** callback, depending on the state of the scope radio buttons.
*/
static void replaceAllScopeCB(Widget w, WindowInfo *window, 
    XmAnyCallbackStruct *callData)
{
    switch(window->replaceScope) {
	case REPL_SCOPE_WIN:
           replaceAllCB(w, window, callData);
           break;
        case REPL_SCOPE_SEL:
           rInSelCB(w, window, callData);
           break;
        case REPL_SCOPE_MULTI:
           replaceMultiFileCB(w, window, callData);
           break;
    }        
}
#endif

static int textFieldNonEmpty(Widget w)
{
    char *str = XmTextGetString(w);
    int nonEmpty = (str[0] != '\0');
    XtFree(str);
    return(nonEmpty);
}

static void rFindTextValueChangedCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    UpdateReplaceActionButtons(window);
}

static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->rHistIndex;
    char *searchStr, *replaceStr;
    int searchType;
    
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
    initToggleButtons(searchType, window->replaceRegexToggle,
                      window->replaceCaseToggle, &window->replaceWordToggle,
                      &window->replaceLastLiteralCase,
                      &window->replaceLastRegexCase);
    
    XmTextSetString(window->replaceText, searchStr);
    XmTextSetString(window->replaceWithText, replaceStr);
    
    /* Set the state of the Replace, Find ... buttons */
    UpdateReplaceActionButtons(window);

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

static void fUpdateActionButtons(WindowInfo *window)
{
    int buttonState = textFieldNonEmpty(window->findText);
    XtSetSensitive(window->findBtn, buttonState);
}

static void findTextValueChangedCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
   fUpdateActionButtons(window);
}

static void findArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->fHistIndex;
    char *searchStr;
    int searchType;
    
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
    initToggleButtons(searchType, window->findRegexToggle,
                      window->findCaseToggle, &window->findWordToggle,
                      &window->findLastLiteralCase,
                      &window->findLastRegexCase);
    XmTextSetString(window->findText, searchStr);

    /* Set the state of the Find ... button */
    fUpdateActionButtons(window);

    window->fHistIndex = index;
}

static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX];
    int direction, searchType;
    char *params[4];
    
    /* fetch find string, direction and type from the dialog */
    if (!getFindDlogInfo(window, &direction, searchString, &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetFindTabGroup(window);
    
    /* find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    params[3] = searchWrapArg(GetPrefSearchWraps());
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 4);

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
    
    /* Get the search and replace strings, search type, and direction
       from the dialog */
    replaceText = XmTextGetString(window->replaceText);
    replaceWithText = XmTextGetString(window->replaceWithText);
    
    if(XmToggleButtonGetState(window->replaceRegexToggle)) {
      int regexDefault;
      if(XmToggleButtonGetState(window->replaceCaseToggle)) {
      	*searchType = SEARCH_REGEX;
	regexDefault = REDFLT_STANDARD;
      } else {
      	*searchType = SEARCH_REGEX_NOCASE;
	regexDefault = REDFLT_CASE_INSENSITIVE;
      }
      /* If the search type is a regular expression, test compile it 
         immediately and present error messages */
      compiledRE = CompileRE(replaceText, &compileMsg, regexDefault);
      if (compiledRE == NULL) {
   	  DialogF(DF_WARN, XtParent(window->replaceDlog), 1,
   	    	 "Please respecify the search string:\n%s", "OK", compileMsg);
	  XtFree(replaceText);
	  XtFree(replaceWithText);
 	  return FALSE;
      }
      free((char*)compiledRE);
    } else {
      if(XmToggleButtonGetState(window->replaceCaseToggle)) {
      	if(XmToggleButtonGetState(window->replaceWordToggle))
	  *searchType = SEARCH_CASE_SENSE_WORD;
	else
	  *searchType = SEARCH_CASE_SENSE;
      } else {
      	if(XmToggleButtonGetState(window->replaceWordToggle))
	  *searchType = SEARCH_LITERAL_WORD;
	else
	  *searchType = SEARCH_LITERAL;
      }
    }
    
    *direction = XmToggleButtonGetState(window->replaceRevToggle) ? 
	SEARCH_BACKWARD : SEARCH_FORWARD;
    
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
    
    /* Get the search string, search type, and direction from the dialog */
    findText = XmTextGetString(window->findText);
    
    if(XmToggleButtonGetState(window->findRegexToggle)) {
      int regexDefault;
      if(XmToggleButtonGetState(window->findCaseToggle)) {
      	*searchType = SEARCH_REGEX;
	regexDefault = REDFLT_STANDARD;
      } else {
      	*searchType = SEARCH_REGEX_NOCASE;
	regexDefault = REDFLT_CASE_INSENSITIVE;
      }
      /* If the search type is a regular expression, test compile it 
         immediately and present error messages */
      compiledRE = CompileRE(findText, &compileMsg, regexDefault);
      if (compiledRE == NULL) {
   	  DialogF(DF_WARN, XtParent(window->findDlog), 1,
   	    	 "Please respecify the search string:\n%s", "OK", compileMsg);
 	  return FALSE;
      }
      free((char *)compiledRE);
    } else {
      if(XmToggleButtonGetState(window->findCaseToggle)) {
      	if(XmToggleButtonGetState(window->findWordToggle))
	  *searchType = SEARCH_CASE_SENSE_WORD;
	else
	  *searchType = SEARCH_CASE_SENSE;
      } else {
      	if(XmToggleButtonGetState(window->findWordToggle))
	  *searchType = SEARCH_LITERAL_WORD;
	else
	  *searchType = SEARCH_LITERAL;
      }
    }
    
    *direction = XmToggleButtonGetState(window->findRevToggle) ? 
	SEARCH_BACKWARD : SEARCH_FORWARD;
    
    if (isRegexType(*searchType)) {
    }

    /* Return the search string */
    strcpy(searchString, findText);
    XtFree(findText);
    return TRUE;
}

int SearchAndSelectSame(WindowInfo *window, int direction, int searchWrap)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 0);
    	return FALSE;
    }
    
    return SearchAndSelect(window, direction, SearchHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Search for "searchString" in "window", and select the matching text in
** the window when found (or beep or put up a dialog if not found).  Also
** adds the search string to the global search history.
*/
int SearchAndSelect(WindowInfo *window, int direction, char *searchString,
	int searchType, int searchWrap)
{
    int startPos, endPos;
    int beginPos, cursorPos, selStart, selEnd;
    
    /* Save a copy of searchString in the search history */
    saveSearchHistory(searchString, NULL, searchType, FALSE);
        
    /* set the position to start the search so we don't find the same
       string that was found on the last search	*/
    if (searchMatchesSelection(window, searchString, searchType,
    	    &selStart, &selEnd, NULL)) {
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

    /* when the i-search bar is active and search is repeated there 
       (Return), the action "find" is called (not: "find_incremental").
       "find" calls this function SearchAndSelect.
       To keep track of the iSearchLastBeginPos correctly in the
       repeated i-search case it is necessary to call the following
       function here, otherwise there are no beeps on the repeated
       incremental search wraps.  */
    iSearchRecordLastBeginPos(window, direction, beginPos);

    /* do the search.  SearchWindow does appropriate dialogs and beeps */
    if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
    	    beginPos, &startPos, &endPos, NULL))
    	return FALSE;
    	
    /* if the search matched an empty string (possible with regular exps)
       beginning at the start of the search, go to the next occurrence,
       otherwise repeated finds will get "stuck" at zero-length matches */
    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
    	if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
    		beginPos+1, &startPos, &endPos, NULL))
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

void SearchForSelected(WindowInfo *window, int direction, int searchType,
    int searchWrap, Time time)
{
   SearchSelectedCallData *callData = XtNew(SearchSelectedCallData);
   callData->direction = direction;
   callData->searchType = searchType;
   callData->searchWrap = searchWrap;
   XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)selectedSearchCB, callData, time);
}

static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
    WindowInfo *window = WidgetToWindow(w);
    SearchSelectedCallData *callDataItems = (SearchSelectedCallData *)callData;
    int searchType;
    char searchString[SEARCHMAX+1];
    
    /* skip if we can't get the selection data or it's too long */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1,
   	    	    "Selection not appropriate for searching", "OK");
    	else
    	    XBell(TheDisplay, 0);
        XtFree(callData);
	return;
    }
    if (*length > SEARCHMAX) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1, "Selection too long", "OK");
    	else
    	    XBell(TheDisplay, 0);
	XtFree(value);
        XtFree(callData);
	return;
    }
    if (*length == 0) {
    	XBell(TheDisplay, 0);
	XtFree(value);
        XtFree(callData);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: can't handle non 8-bit text\n");
    	XBell(TheDisplay, 0);
	XtFree(value);
        XtFree(callData);
	return;
    }
    /* make the selection the current search string */
    strncpy(searchString, value, *length);
    searchString[*length] = '\0';
    XtFree(value);
    
    /* Use the passed method for searching, unless it is regex, since this
       kind of search is by definition a literal search */
    searchType = callDataItems->searchType;
    if (searchType == SEARCH_REGEX )
      searchType = SEARCH_CASE_SENSE;
    else if (searchType == SEARCH_REGEX_NOCASE)
	      searchType = SEARCH_LITERAL;

    /* search for it in the window */
    SearchAndSelect(window, callDataItems->direction, searchString,
        searchType, callDataItems->searchWrap);
    XtFree(callData);
}

/*
** Pop up and clear the incremental search line and prepare to search.
*/
void BeginISearch(WindowInfo *window, int direction)
{
    window->iSearchStartPos = -1;
    XmTextSetString(window->iSearchText, "");
    XmToggleButtonSetState(window->iSearchRevToggle,
	    direction == SEARCH_BACKWARD, FALSE);
    /* Note: in contrast to the replace and find dialogs, the regex and
       case toggles are not reset to their default state when the incremental
       search bar is redisplayed. I'm not sure whether this is the best
       choice. If not, an initToggleButtons() call should be inserted
       here. But in that case, it might be appropriate to have different
       default search modes for i-search and replace/find. */
    TempShowISearch(window, TRUE);
    XmProcessTraversal(window->iSearchText, XmTRAVERSE_CURRENT);
}

/*
** Incremental searching is anchored at the position where the cursor
** was when the user began typing the search string.  Call this routine
** to forget about this original anchor, and if the search bar is not
** permanently up, pop it down.
*/
void EndISearch(WindowInfo *window)
{
    /* Note: Please maintain this such that it can be freely peppered in
       mainline code, without callers having to worry about performance
       or visual glitches.  */
    
    /* Forget the starting position used for the current run of searches */
    window->iSearchStartPos = -1;
    
    /* Mark the end of incremental search history overwriting */
    saveSearchHistory("", NULL, 0, FALSE);
    
    /* Pop down the search line (if it's not pegged up in Preferences) */
    TempShowISearch(window, FALSE);
}

/* 
** Reset window->iSearchLastBeginPos to the resulting initial
** search begin position for incremental searches.
*/
static void iSearchRecordLastBeginPos(WindowInfo *window, int direction, 
	int initPos) 
{
    window->iSearchLastBeginPos = initPos;
    if (direction == SEARCH_BACKWARD) 
      	window->iSearchLastBeginPos--;
}      

/*
** Search for "searchString" in "window", and select the matching text in
** the window when found (or beep or put up a dialog if not found).  If
** "continued" is TRUE and a prior incremental search starting position is
** recorded, search from that original position, otherwise, search from the
** current cursor position.
*/
int SearchAndSelectIncremental(WindowInfo *window, int direction,
	char *searchString, int searchType, int searchWrap, int continued)
{
    int beginPos, startPos, endPos;

    /* If there's a search in progress, start the search from the original
       starting position, otherwise search from the cursor position. */
    if (!continued || window->iSearchStartPos == -1) {
	window->iSearchStartPos = TextGetCursorPos(window->lastFocus);
	iSearchRecordLastBeginPos(window, direction, window->iSearchStartPos);
    }
    beginPos = window->iSearchStartPos;

    /* If the search string is empty, beep eventually if text wrapped
       back to the initial position, re-init iSearchLastBeginPos, 
       clear the selection, set the cursor back to what would be the 
       beginning of the search, and return. */
    if(searchString[0] == 0) {
     	int beepBeginPos = (direction == SEARCH_BACKWARD) ? beginPos-1:beginPos;
      	iSearchTryBeepOnWrap(window, direction, beepBeginPos, beepBeginPos);
	iSearchRecordLastBeginPos(window, direction, window->iSearchStartPos);
	BufUnselect(window->buffer);
	TextSetCursorPos(window->lastFocus, beginPos);
	return TRUE;
    }

    /* Save the string in the search history, unless we're cycling thru
       the search history itself, which can be detected by matching the
       search string with the search string of the current history index. */
    if(!(window->iSearchHistIndex > 1 && !strcmp(searchString, 
	    SearchHistory[historyIndex(window->iSearchHistIndex)]))) {
   	saveSearchHistory(searchString, NULL, searchType, TRUE);
	/* Reset the incremental search history pointer to the beginning */
	window->iSearchHistIndex = 1;
    }
        
    /* begin at insert position - 1 for backward searches */
    if (direction == SEARCH_BACKWARD)
	beginPos--;

    /* do the search.  SearchWindow does appropriate dialogs and beeps */
    if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
	    beginPos, &startPos, &endPos, NULL))
	return FALSE;

    window->iSearchLastBeginPos = startPos;

    /* if the search matched an empty string (possible with regular exps)
       beginning at the start of the search, go to the next occurrence,
       otherwise repeated finds will get "stuck" at zero-length matches */
    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
	if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
	    beginPos+1, &startPos, &endPos, NULL))
	    return FALSE;

    window->iSearchLastBeginPos = startPos;

    /* select the text found string */
    BufSelect(window->buffer, startPos, endPos);
    MakeSelectionVisible(window, window->lastFocus);
    TextSetCursorPos(window->lastFocus, endPos);

    return TRUE;
}

/*
** Attach callbacks to the incremental search bar widgets.  This also fudges
** up the translations on the text widget so Shift+Return will call the
** activate callback (along with Return and Ctrl+Return).  It does this
** because incremental search uses the activate callback from the text
** widget to detect when the user has pressed Return to search for the next
** occurrence of the search string, and Shift+Return, which is the natural
** command for a reverse search does not naturally trigger this callback.
*/
void SetISearchTextCallbacks(WindowInfo *window)
{
    static XtTranslations table = NULL;
    static char *translations = "Shift<KeyPress>Return: activate()\n";
    
    if (table == NULL)
    	table = XtParseTranslationTable(translations);
    XtOverrideTranslations(window->iSearchText, table);
    
    XtAddCallback(window->iSearchText, XmNactivateCallback, 
      (XtCallbackProc)iSearchTextActivateCB, window);
    XtAddCallback(window->iSearchText, XmNvalueChangedCallback, 
      (XtCallbackProc)iSearchTextValueChangedCB, window);
    XtAddEventHandler(window->iSearchText, KeyPressMask, False,
      (XtEventHandler)iSearchTextKeyEH, window);
    
    /* Attach callbacks to deal with the optional sticky case sensitivity
       behaviour. Do this before installing the search callbacks to make 
       sure that the proper search parameters are taken into account. */
    XtAddCallback(window->iSearchCaseToggle, XmNvalueChangedCallback,
	    (XtCallbackProc)iSearchCaseToggleCB, window);
    XtAddCallback(window->iSearchRegexToggle, XmNvalueChangedCallback,
	    (XtCallbackProc)iSearchRegExpToggleCB, window);
    
    /* When search parameters (direction or search type), redo the search */
    XtAddCallback(window->iSearchCaseToggle, XmNvalueChangedCallback,
	    (XtCallbackProc)iSearchTextValueChangedCB, window);
    XtAddCallback(window->iSearchRegexToggle, XmNvalueChangedCallback,
	    (XtCallbackProc)iSearchTextValueChangedCB, window);
    XtAddCallback(window->iSearchRevToggle, XmNvalueChangedCallback,
	    (XtCallbackProc)iSearchTextValueChangedCB, window);
}

/*
** User pressed return in the incremental search bar.  Do a new search with
** the search string displayed.  The direction of the search is toggled if
** the Ctrl key or the Shift key is pressed when the text field is activated.
*/
static void iSearchTextActivateCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData) 
{
    char *params[4];
    char *searchString;
    int searchType, direction;
   
    /* Fetch the string, search type and direction from the incremental
       search bar widgets at the top of the window */
    searchString = XmTextGetString(window->iSearchText);
    if(XmToggleButtonGetState(window->iSearchCaseToggle)) {
      if(XmToggleButtonGetState(window->iSearchRegexToggle)) 
	searchType = SEARCH_REGEX;
      else 
	searchType = SEARCH_CASE_SENSE;
    } else {
      if(XmToggleButtonGetState(window->iSearchRegexToggle)) 
	searchType = SEARCH_REGEX_NOCASE;
      else 
	searchType = SEARCH_LITERAL;
    }
    direction = XmToggleButtonGetState(window->iSearchRevToggle) ?
	    SEARCH_BACKWARD : SEARCH_FORWARD;
    
    /* Reverse the search direction if the Ctrl or Shift key was pressed */
    if (callData->event->xbutton.state & (ShiftMask | ControlMask))
	direction = direction == SEARCH_FORWARD ?
		SEARCH_BACKWARD : SEARCH_FORWARD;
	
    /* find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    params[3] = searchWrapArg(GetPrefSearchWraps());
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 4);
    XtFree(searchString);
}

/*
** Called when user types in the incremental search line.  Redoes the
** search for the new search string.
*/
static void iSearchTextValueChangedCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData) 
{
    char *params[5];
    char *searchString;
    int searchType, direction, nParams;
   
    /* Fetch the string, search type and direction from the incremental
       search bar widgets at the top of the window */
    searchString = XmTextGetString(window->iSearchText);
    if(XmToggleButtonGetState(window->iSearchCaseToggle)) {
      if(XmToggleButtonGetState(window->iSearchRegexToggle)) 
	searchType = SEARCH_REGEX;
      else 
	searchType = SEARCH_CASE_SENSE;
    } else {
      if(XmToggleButtonGetState(window->iSearchRegexToggle)) 
	searchType = SEARCH_REGEX_NOCASE;
      else 
	searchType = SEARCH_LITERAL;
    }
    direction = XmToggleButtonGetState(window->iSearchRevToggle) ?
	    SEARCH_BACKWARD : SEARCH_FORWARD;

    /* If the search type is a regular expression, test compile it.  If it
       fails, silently skip it.  (This allows users to compose the expression
       in peace when they have unfinished syntax, but still get beeps when
       correct syntax doesn't match) */
    if (isRegexType(searchType)) {
	regexp *compiledRE = NULL;
	char *compileMsg;
	compiledRE = CompileRE(searchString, &compileMsg, 
	                       defaultRegexFlags(searchType));
	if (compiledRE == NULL) {
	    XtFree(searchString);
	    return;
	}
	free((char *)compiledRE);
    }
    
    /* Call the incremental search action proc to do the searching and
       selecting (this allows it to be recorded for learn/replay).  If
       there's an incremental search already in progress, mark the operation
       as "continued" so the search routine knows to re-start the search
       from the original starting position */
    nParams = 0;
    params[nParams++] = searchString;
    params[nParams++] = directionArg(direction);
    params[nParams++] = searchTypeArg(searchType);
    params[nParams++] = searchWrapArg(GetPrefSearchWraps());
    if (window->iSearchStartPos != -1)
	params[nParams++] = "continued";
    XtCallActionProc(window->lastFocus, "find_incremental",
	    callData->event, params, nParams);
    XtFree(searchString);
}

/*
** Process arrow keys for history recall, and escape key for leaving
** incremental search bar.
*/
static void iSearchTextKeyEH(Widget w, WindowInfo *window,
	XKeyEvent *event, Boolean *continueDispatch)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->iSearchHistIndex;
    char *searchStr;
    int searchType;

    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down && keysym != XK_Escape) {
	*continueDispatch = TRUE;
	return;
    }
    *continueDispatch = FALSE;

    /* allow escape key to cancel search */
    if (keysym == XK_Escape) {
	XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
	EndISearch(window);
	return;
    }
    
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

    /* Set the info used in the value changed callback before calling
      XmTextSetString(). */
    window->iSearchHistIndex = index;
    initToggleButtons(searchType, window->iSearchRegexToggle,
                      window->iSearchCaseToggle, NULL,
                      &window->iSearchLastLiteralCase,
                      &window->iSearchLastRegexCase);
    
    /* Beware the value changed callback is processed as part of this call */
    XmTextSetString(window->iSearchText, searchStr);
    XmTextSetInsertionPosition(window->iSearchText, 
	    XmTextGetLastPosition(window->iSearchText));
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
    
    /* no flashing required */
    if (window->showMatchingStyle == NO_FLASH) {
	return;
    }

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

    if (window->showMatchingStyle == FLASH_DELIMIT) {
	/* Highlight either the matching character ... */
	BufHighlight(window->buffer, matchPos, matchPos+1);
    } else {
	/* ... or the whole range. */
  	if (MatchingChars[matchIndex].direction == SEARCH_BACKWARD) {
	    BufHighlight(window->buffer, matchPos, pos+1);
	} else {
	    BufHighlight(window->buffer, matchPos+1, pos);
	}
    }
      
    /* Set up a timer to erase the box after 1.5 seconds */
    window->flashTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 1500,
    	    flashTimeoutProc, window);
    window->flashPos = matchPos;
}

void SelectToMatchingCharacter(WindowInfo *window)
{
    int selStart, selEnd;
    int startPos, endPos, matchPos;
    textBuffer *buf = window->buffer;

    /* get the character to match and its position from the selection, or
       the character before the insert point if nothing is selected.
       Give up if too many characters are selected */
    if (!GetSimpleSelection(buf, &selStart, &selEnd)) {
	selEnd = TextGetCursorPos(window->lastFocus);
        if (window->overstrike)
	    selEnd += 1;
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

void GotoMatchingCharacter(WindowInfo *window)
{
    int selStart, selEnd;
    int matchPos;
    textBuffer *buf = window->buffer;

    /* get the character to match and its position from the selection, or
       the character before the insert point if nothing is selected.
       Give up if too many characters are selected */
    if (!GetSimpleSelection(buf, &selStart, &selEnd)) {
	selEnd = TextGetCursorPos(window->lastFocus);
        if (window->overstrike)
	    selEnd += 1;
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
    
    /* temporarily shut off autoShowInsertPos before setting the cursor
       position so MakeSelectionVisible gets a chance to place the cursor
       string at a pleasing position on the screen (otherwise, the cursor would
       be automatically scrolled on screen and MakeSelectionVisible would do
       nothing) */
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, False, NULL);
    TextSetCursorPos(window->lastFocus, matchPos+1);
    MakeSelectionVisible(window, window->lastFocus);
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, True, NULL);
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
int ReplaceSame(WindowInfo *window, int direction, int searchWrap)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 0);
    	return FALSE;
    }

    return SearchAndReplace(window, direction, SearchHistory[historyIndex(1)],
    	    ReplaceHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceFindSame(WindowInfo *window, int direction, int searchWrap)
{
    if (NHist < 1) {
        XBell(TheDisplay, 0);
        return FALSE;
    }

    return ReplaceAndSearch(window, direction, SearchHistory[historyIndex(1)],
                            ReplaceHistory[historyIndex(1)],
                            SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Replace selection with "replaceString" and search for string "searchString" in window "window", 
** using algorithm "searchType" and direction "direction"
*/
int ReplaceAndSearch(WindowInfo *window, int direction, char *searchString,
                     char *replaceString, int searchType, int searchWrap)
{
    int startPos = 0, endPos = 0, replaceLen = 0;
    int searchExtent;
    int replaced;

    /* Save a copy of search and replace strings in the search history */
    saveSearchHistory(searchString, replaceString, searchType, FALSE);
    
    replaced = 0;

    /* Replace the selected text only if it matches the search string */
    if (searchMatchesSelection(window, searchString, searchType,
	                       &startPos, &endPos, &searchExtent)) {
	/* replace the text */
	if (isRegexType(searchType)) {
    	    char replaceResult[SEARCHMAX+1], *foundString;
	    foundString = BufGetRange(window->buffer, startPos, searchExtent+1);
    	    replaceUsingRE(searchString, replaceString, foundString,
		    replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
		    BufGetCharacter(window->buffer, startPos-1),
		    GetWindowDelimiters(window), defaultRegexFlags(searchType));
	    XtFree(foundString);
    	    BufReplace(window->buffer, startPos, endPos, replaceResult);
    	    replaceLen = strlen(replaceResult);
	} else {
            BufReplace(window->buffer, startPos, endPos, replaceString);
            replaceLen = strlen(replaceString);
	}

        /* Position the cursor so the next search will work correctly based */
        /* on the direction of the search */
        TextSetCursorPos(window->lastFocus, startPos +
                         ((direction == SEARCH_FORWARD) ? replaceLen : 0));
        replaced = 1;
    }

    /* do the search; beeps/dialogs are taken care of */
    SearchAndSelect(window, direction, searchString, searchType, searchWrap);

    return replaced;
}           

/*
** Search for string "searchString" in window "window", using algorithm
** "searchType" and direction "direction", and replace it with "replaceString"
** Also adds the search and replace strings to the global search history.
*/
int SearchAndReplace(WindowInfo *window, int direction, char *searchString,
	char *replaceString, int searchType, int searchWrap)
{
    int startPos, endPos, replaceLen, searchExtent;
    int found;
    int beginPos, cursorPos;
    
    /* Save a copy of search and replace strings in the search history */
    saveSearchHistory(searchString, replaceString, searchType, FALSE);
    
    /* If the text selected in the window matches the search string, 	*/
    /* the user is probably using search then replace method, so	*/
    /* replace the selected text regardless of where the cursor is.	*/
    /* Otherwise, search for the string.				*/
    if (!searchMatchesSelection(window, searchString, searchType,
    	    &startPos, &endPos, &searchExtent)) {
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
	found = SearchWindow(window, direction, searchString, searchType, searchWrap,
		beginPos, &startPos, &endPos, &searchExtent);
	if (!found)
	    return FALSE;
    }
    
    /* replace the text */
    if (isRegexType(searchType)) {
    	char replaceResult[SEARCHMAX], *foundString;
	foundString = BufGetRange(window->buffer, startPos, searchExtent+1);
    	replaceUsingRE(searchString, replaceString, foundString,
		replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
		BufGetCharacter(window->buffer, startPos-1),
		GetWindowDelimiters(window), defaultRegexFlags(searchType));
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
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, False, NULL);
    TextSetCursorPos(window->lastFocus, startPos +
    	((direction == SEARCH_FORWARD) ? replaceLen : 0));
    MakeSelectionVisible(window, window->lastFocus);
    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, True, NULL);
    
    return TRUE;
}

/*
** Replace all occurences of "searchString" in "window" with "replaceString"
** within the current primary selection in "window". Also adds the search and
** replace strings to the global search history.
*/
int ReplaceInSelection(WindowInfo *window, char *searchString,
	char *replaceString, int searchType)
{
    int selStart, selEnd, beginPos, startPos, endPos, realOffset, replaceLen;
    int found, anyFound, isRect, rectStart, rectEnd, lineStart, cursorPos;
    int extent;
    char *fileString;
    textBuffer *tempBuf;
    
    /* save a copy of search and replace strings in the search history */
    saveSearchHistory(searchString, replaceString, searchType, FALSE);
    
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
    cursorPos = 0;
    realOffset = 0;
    while (found) {
	found = SearchString(fileString, searchString, SEARCH_FORWARD,
		searchType, FALSE, beginPos, &startPos, &endPos, &extent,
		GetWindowDelimiters(window));
	if (!found)
	    break;
	/* if the selection is rectangular, verify that the found
	   string is in the rectangle */
	if (isRect) {
	    lineStart = BufStartOfLine(window->buffer, selStart+startPos);
	    if (BufCountDispChars(window->buffer, lineStart, selStart+startPos) <
		    rectStart || BufCountDispChars(window->buffer, lineStart,
		    selStart+endPos) > rectEnd) {
		if (fileString[endPos] == '\0')
		    break;
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
	if (isRegexType(searchType)) {
    	    char replaceResult[SEARCHMAX], *foundString;
	    foundString = BufGetRange(tempBuf, startPos+realOffset,
		    extent+realOffset+1);
    	    replaceUsingRE(searchString, replaceString, foundString,
		    replaceResult, SEARCHMAX, startPos+realOffset == 0 ? '\0' :
		    BufGetCharacter(tempBuf, startPos+realOffset-1),
		    GetWindowDelimiters(window), defaultRegexFlags(searchType));
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
	if (fileString[endPos] == '\0')
	    break;
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
    		unmanageReplaceDialogs(window);
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
** Replace all occurences of "searchString" in "window" with "replaceString".
** Also adds the search and replace strings to the global search history.
*/
int ReplaceAll(WindowInfo *window, char *searchString, char *replaceString,
	int searchType)
{
    char *fileString, *newFileString;
    int copyStart, copyEnd, replacementLen;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
    
    /* save a copy of search and replace strings in the search history */
    saveSearchHistory(searchString, replaceString, searchType, FALSE);
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->buffer);
    
    newFileString = ReplaceAllInString(fileString, searchString, replaceString,
	    searchType, &copyStart, &copyEnd, &replacementLen,
	    GetWindowDelimiters(window));
    XtFree(fileString);
    
    if (newFileString == NULL) {
        if (window->multiFileBusy) {
            window->replaceFailed = TRUE; /* only needed during multi-file 
                                             replacements */
        } else if (GetPrefSearchDlogs()) {
    	    if (window->findDlog && XtIsManaged(window->findDlog) &&
    	    	    !XmToggleButtonGetState(window->findKeepBtn))
    		XtUnmanageChild(window->findDlog);
    	    if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	    !XmToggleButtonGetState(window->replaceKeepBtn))
    		unmanageReplaceDialogs(window);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 0);
	return FALSE;
    }
    
    /* replace the contents of the text widget with the substituted text */
    BufReplace(window->buffer, copyStart, copyEnd, newFileString);
    
    /* Move the cursor to the end of the last replacement */
    TextSetCursorPos(window->lastFocus, copyStart + replacementLen);

    XtFree(newFileString);
    return TRUE;	
}    

/*
** Replace all occurences of "searchString" in "inString" with "replaceString"
** and return an allocated string covering the range between the start of the
** first replacement (returned in "copyStart", and the end of the last
** replacement (returned in "copyEnd")
*/
char *ReplaceAllInString(char *inString, char *searchString,
	char *replaceString, int searchType, int *copyStart,
	int *copyEnd, int *replacementLength, char *delimiters)
{
    int beginPos, startPos, endPos, lastEndPos;
    int found, nFound, removeLen, replaceLen, copyLen, addLen;
    char *outString, *fillPtr;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return NULL;
    
    /* rehearse the search first to determine the size of the buffer needed
       to hold the substituted text.  No substitution done here yet */
    replaceLen = strlen(replaceString);
    found = TRUE;
    nFound = 0;
    removeLen = 0;
    addLen = 0;
    beginPos = 0;
    *copyStart = -1;
    while (found) {
    	found = SearchString(inString, searchString, SEARCH_FORWARD, searchType,
		FALSE, beginPos, &startPos, &endPos, NULL, delimiters);
	if (found) {
	    if (*copyStart < 0)
	    	*copyStart = startPos;
    	    *copyEnd = endPos;
    	    /* start next after match unless match was empty, then endPos+1 */
    	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    nFound++;
	    removeLen += endPos - startPos;
	    if (isRegexType(searchType)) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString, &inString[startPos],
    			replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
			inString[startPos-1], delimiters,
                        defaultRegexFlags(searchType));
    		addLen += strlen(replaceResult);
    	    } else
    	    	addLen += replaceLen;
	    if (inString[endPos] == '\0')
		break;
	}
    }
    if (nFound == 0)
	return NULL;
    
    /* Allocate a new buffer to hold all of the new text between the first
       and last substitutions */
    copyLen = *copyEnd - *copyStart;
    outString = XtMalloc(copyLen - removeLen + addLen + 1);
    
    /* Scan through the text buffer again, substituting the replace string
       and copying the part between replaced text to the new buffer  */
    found = TRUE;
    beginPos = 0;
    lastEndPos = 0;
    fillPtr = outString;
    while (found) {
    	found = SearchString(inString, searchString, SEARCH_FORWARD, searchType,
		FALSE, beginPos, &startPos, &endPos, NULL, delimiters);
	if (found) {
	    if (beginPos != 0) {
		memcpy(fillPtr, &inString[lastEndPos], startPos - lastEndPos);
		fillPtr += startPos - lastEndPos;
	    }
	    if (isRegexType(searchType)) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString, &inString[startPos],
    			replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
			inString[startPos-1], delimiters,
	      	      	defaultRegexFlags(searchType));
    		replaceLen = strlen(replaceResult);
    		memcpy(fillPtr, replaceResult, replaceLen);
	    } else {
		memcpy(fillPtr, replaceString, replaceLen);
	    }
	    fillPtr += replaceLen;
	    lastEndPos = endPos;
	    /* start next after match unless match was empty, then endPos+1 */
	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    if (inString[endPos] == '\0')
		break;
	}
    }
    *fillPtr = '\0';
    *replacementLength = fillPtr - outString;
    return outString;
}

/* 
** If this is an incremental search and BeepOnSearchWrap is on:
** Emit a beep if the search wrapped over BOF/EOF compared to
** the last startPos of the current incremental search.
*/
static void iSearchTryBeepOnWrap(WindowInfo *window, int direction, 
	int beginPos, int startPos) 
{
    if(GetPrefBeepOnSearchWrap())  {
	if(direction == SEARCH_FORWARD) {
	    if(  (startPos >= beginPos && window->iSearchLastBeginPos < beginPos)
	       ||(startPos < beginPos && window->iSearchLastBeginPos >= beginPos)) 
	    XBell(TheDisplay, 0);
	} else {
	    if(  (startPos <= beginPos && window->iSearchLastBeginPos > beginPos)
	       ||(startPos > beginPos && window->iSearchLastBeginPos <= beginPos))
	    XBell(TheDisplay, 0);
	}
    }
}

/*
** Search the text in "window", attempting to match "searchString"
*/
int SearchWindow(WindowInfo *window, int direction, char *searchString,
	int searchType, int searchWrap, int beginPos, int *startPos, int *endPos, int *extent)
{
    char *fileString;
    int found, resp, fileEnd;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->buffer);
    
    /* search the string copied from the text area widget, and present
       dialogs, or just beep.  iSearchStartPos is not a perfect indicator that
       an incremental search is in progress.  A parameter would be better. */
    if (GetPrefSearchDlogs() && window->iSearchStartPos == -1) {
    	found = SearchString(fileString, searchString, direction, searchType,
    	    	FALSE, beginPos, startPos, endPos, extent,
		GetWindowDelimiters(window));
    	/* Avoid Motif 1.1 bug by putting away search dialog before DialogF */
    	if (window->findDlog && XtIsManaged(window->findDlog) &&
    	    	!XmToggleButtonGetState(window->findKeepBtn))
    	    XtUnmanageChild(window->findDlog);
    	if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	!XmToggleButtonGetState(window->replaceKeepBtn))
    	    unmanageReplaceDialogs(window);
        if (!found) {
            if (searchWrap) {
		fileEnd = window->buffer->length - 1;
		if (direction == SEARCH_FORWARD && beginPos != 0) {
		    if(GetPrefBeepOnSearchWrap()) {
			XBell(TheDisplay, 0);
		    } else {
			resp = DialogF(DF_QUES, window->shell, 2,
				"Continue search from\nbeginning of file?", 
                                "Continue", "Cancel");
			if (resp == 2) {
			    XtFree(fileString);
			    return False;
			}
		    }
		    found = SearchString(fileString, searchString, direction,
			searchType, FALSE, 0, startPos, endPos, extent,
			GetWindowDelimiters(window));
		} else if (direction == SEARCH_BACKWARD && beginPos != fileEnd) {
		    if(GetPrefBeepOnSearchWrap()) {
			XBell(TheDisplay, 0);
		    } else {
			resp = DialogF(DF_QUES, window->shell, 2,
				"Continue search\nfrom end of file?", "Continue",
				"Cancel");
			if (resp == 2) {
			    XtFree(fileString);
			    return False;
			}
		    }
                    found = SearchString(fileString, searchString, direction,
			searchType, FALSE, fileEnd, startPos, endPos, extent,
			GetWindowDelimiters(window));
        	}
	    }
            if (!found)
                DialogF(DF_INF, window->shell,1,"String was not found","OK");
        }
    } else { /* no dialogs */
	if(GetPrefBeepOnSearchWrap() && searchWrap && window->iSearchStartPos == -1) {
	    found = SearchString(fileString, searchString, direction, searchType,
		    FALSE, beginPos, startPos, endPos, extent,
		    GetWindowDelimiters(window));
	    if (!found) {
		fileEnd = window->buffer->length - 1;
		if (direction == SEARCH_FORWARD && beginPos != 0) {
		    XBell(TheDisplay, 0);
		    found = SearchString(fileString, searchString, direction,
			    searchType, FALSE, 0, startPos, endPos, extent,
			    GetWindowDelimiters(window));
		} else if (direction == SEARCH_BACKWARD && beginPos != fileEnd) {
		    XBell(TheDisplay, 0);
		    found = SearchString(fileString, searchString, direction,
			    searchType, FALSE, fileEnd, startPos, endPos, extent,
			    GetWindowDelimiters(window));
		}
		if (!found)
		  XBell(TheDisplay, 0);
	    }
      	} else {
	    found = SearchString(fileString, searchString, direction,
		    searchType, searchWrap, beginPos, startPos, endPos, extent,
		    GetWindowDelimiters(window));
	    if (found) {
	      	if(window->iSearchStartPos != -1)
	      	    iSearchTryBeepOnWrap(window, direction, beginPos, *startPos);
	    } else
		XBell(TheDisplay, 0);
	}
    }
    
    /* Free the text buffer copy returned from BufGetAll */
    XtFree(fileString);

    return found;
}

/*
** Search the null terminated string "string" for "searchString", beginning at
** "beginPos".  Returns the boundaries of the match in "startPos" and "endPos".
** searchExtent returns the forwardmost position used to make the match, which
** is usually endPos, but may extend further if positive lookahead was used in
** a regular expression match.  "delimiters" may be used to provide an
** alternative set of word delimiters for regular expression "<" and ">"
** characters, or simply passed as null for the default delimiter set.
*/
int SearchString(const char *string, const char *searchString, int direction,
       int searchType, int wrap, int beginPos, int *startPos, int *endPos,
       int *searchExtent, const char *delimiters)
{
    switch (searchType) {
      case SEARCH_CASE_SENSE_WORD:
      	 return searchLiteralWord(string, searchString, TRUE,  direction, wrap,
	 		       beginPos, startPos, endPos, delimiters);
      case SEARCH_LITERAL_WORD:
      	 return  searchLiteralWord(string, searchString, FALSE, direction, wrap,
	 		       beginPos, startPos, endPos, delimiters);
      case SEARCH_CASE_SENSE:
      	 return searchLiteral(string, searchString, TRUE, direction, wrap,
	 		       beginPos, startPos, endPos, searchExtent);
      case SEARCH_LITERAL:
      	 return  searchLiteral(string, searchString, FALSE, direction, wrap,
	 	beginPos, startPos, endPos, searchExtent);
      case SEARCH_REGEX:
      	 return  searchRegex(string, searchString, direction, wrap,
      	 	beginPos, startPos, endPos, searchExtent, delimiters, 
                REDFLT_STANDARD);
      case SEARCH_REGEX_NOCASE:
      	 return  searchRegex(string, searchString, direction, wrap,
      	 	beginPos, startPos, endPos, searchExtent, delimiters, 
                REDFLT_CASE_INSENSITIVE);
    }
    return FALSE; /* never reached, just makes compilers happy */
}

/* 
** Parses a search type description string. If the string contains a valid 
** search type description, returns TRUE and writes the corresponding 
** SearchType in searchType. Returns FALSE and leaves searchType untouched 
** otherwise. (Originally written by Markus Schwarzenberg; slightly adapted).
*/
int StringToSearchType(const char * string, int *searchType) 
{
    if (!strcmp(string, SEARCH_LITERAL_STRING))
    	*searchType = SEARCH_LITERAL;
    else if (!strcmp(string, SEARCH_CASE_SENSE_STRING))
    	*searchType =  SEARCH_CASE_SENSE;
    else if (!strcmp(string, SEARCH_REGEX_STRING))
    	*searchType =  SEARCH_REGEX;
    else if (!strcmp(string, SEARCH_LITERAL_WORD_STRING))
    	*searchType =  SEARCH_LITERAL_WORD;
    else if (!strcmp(string, SEARCH_CASE_SENSE_WORD_STRING))
    	*searchType =  SEARCH_CASE_SENSE_WORD;
    else if (!strcmp(string, SEARCH_REGEX_NOCASE_STRING))
    	*searchType =  SEARCH_REGEX_NOCASE;
    else 
      	return FALSE; /* string not recognized  */
    return TRUE; /* string ok */
} 

/*
**  Searches for whole words (Markus Schwarzenberg).
**
**  If the first/last character of `searchString' is a "normal
**  word character" (not contained in `delimiters', not a whitespace)
**  then limit search to strings, who's next left/next right character
**  is contained in `delimiters' or is a whitespace or text begin or end.
**
**  If the first/last character of `searchString' itself is contained
**  in delimiters or is a white space, then the neighbour character of the
**  first/last character will not be checked, just a simple match 
**  will suffice in that case.
**  
*/
static int searchLiteralWord(const char *string, const char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos, 
        const char * delimiters)
{
/* This is critical code for the speed of searches.			    */
/* For efficiency, we define the macro DOSEARCH with the guts of the search */
/* routine and repeat it, changing the parameters of the outer loop for the */
/* searching, forwards, backwards, and before and after the begin point	    */
#define DOSEARCHWORD() \
    if (*filePtr == *ucString || *filePtr == *lcString) { \
	/* matched first character */ \
	ucPtr = ucString; \
	lcPtr = lcString; \
	tempPtr = filePtr; \
	while (*tempPtr == *ucPtr || *tempPtr == *lcPtr) { \
	    tempPtr++; ucPtr++; lcPtr++; \
	    if (   *ucPtr == 0 /* matched whole string */ \
		&& (cignore_R ||\
		    isspace(*tempPtr) ||\
		    strchr(delimiters, *tempPtr) ) \
		    /* next char right delimits word ? */ \
		&& (cignore_L ||\
                    filePtr==string || /* border case */ \
                    isspace(filePtr[-1]) ||\
                    strchr(delimiters,filePtr[-1]) ))\
                    /* next char left delimits word ? */ { \
		*startPos = filePtr - string; \
		*endPos = tempPtr - string; \
		return TRUE; \
	    } \
	} \
    }

    register const char *filePtr, *tempPtr, *ucPtr, *lcPtr;
    char lcString[SEARCHMAX], ucString[SEARCHMAX];
						
    int cignore_L=0, cignore_R=0;
		
    /* SEARCHMAX was fine in the original NEdit, but it should be done away 
       with now that searching can be done from macros without limits. 
       Returning search failure here is cheating users.  This limit is not 
       documented. */
    if (strlen(searchString) >= SEARCHMAX)
	return FALSE;
    
    /* If there is no language mode, we use the default list of delimiters */
    if (delimiters==NULL) delimiters = GetPrefDelimiters();
		
    if (   isspace(*searchString) 
	|| strchr(delimiters, *searchString))
	cignore_L=1;

    if (   isspace(searchString[strlen(searchString)-1])
	|| strchr(delimiters, searchString[strlen(searchString)-1]) )
	cignore_R=1;
   
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
      	    DOSEARCHWORD() 
	}
	if (!wrap)
	    return FALSE;

	/* search from start of file to beginPos */
	for (filePtr=string; filePtr<=string+beginPos; filePtr++) {
      	    DOSEARCHWORD() 
	}
	return FALSE;
    } else {
	/* SEARCH_BACKWARD */
	/* search from beginPos to start of file. A negative begin pos */
	/* says begin searching from the far end of the file */
	if (beginPos >= 0) {
	    for (filePtr=string+beginPos; filePtr>=string; filePtr--) {
	    	DOSEARCHWORD() 
	    }
	}
	if (!wrap)
	    return FALSE;
	/* search from end of file to beginPos */
	/*... this strlen call is extreme inefficiency, but it's not obvious */
	/* how to get the text string length from the text widget (under 1.1)*/
	for (filePtr=string+strlen(string); filePtr>=string+beginPos; filePtr--) {
      	    DOSEARCHWORD() 
	}
	return FALSE;
    }
}


static int searchLiteral(const char *string, const char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos,
	int *searchExtent)
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
		if (searchExtent != NULL) \
		    *searchExtent = *endPos; \
		return TRUE; \
	    } \
	} \
    } \

    register const char *filePtr, *tempPtr, *ucPtr, *lcPtr;
    char lcString[SEARCHMAX], ucString[SEARCHMAX];

    /* SEARCHMAX was fine in the original NEdit, but it should be done away with
       now that searching can be done from macros without limits.  Returning
       search failure here is cheating users.  This limit is not documented. */
    if (strlen(searchString) >= SEARCHMAX)
	return FALSE;
    
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

static int searchRegex(const char *string, const char *searchString, int direction,
	int wrap, int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags)
{
    if (direction == SEARCH_FORWARD)
	return forwardRegexSearch(string, searchString, wrap, 
            beginPos, startPos, endPos, searchExtent, delimiters, defaultFlags);
    else
    	return backwardRegexSearch(string, searchString, wrap, 
	    beginPos, startPos, endPos, searchExtent, delimiters, defaultFlags);
}

static int forwardRegexSearch(const char *string, const char *searchString, int wrap,
	int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    
    /* compile the search string for searching with ExecRE.  Note that
       this does not process errors from compiling the expression.  It
       assumes that the expression was checked earlier. */
    compiledRE = CompileRE(searchString, &compileMsg, defaultFlags);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to end of string */
    if (ExecRE(compiledRE, NULL, string + beginPos, NULL, FALSE,
    	    beginPos==0 ? '\0' : string[beginPos-1], '\0', delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	if (searchExtent != NULL)
	    *searchExtent = compiledRE->extentp - string;
	free((char *)compiledRE);
	return TRUE;
    }
    
    /* if wrap turned off, we're done */
    if (!wrap) {
    	free((char *)compiledRE);
	return FALSE;
    }
    
    /* search from the beginning of the string to beginPos */
    if (ExecRE(compiledRE, NULL, string, string + beginPos, FALSE, '\0',
	    string[beginPos], delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	if (searchExtent != NULL)
	    *searchExtent = compiledRE->extentp - string;
	free((char *)compiledRE);
	return TRUE;
    }

    free((char *)compiledRE);
    return FALSE;
}

static int backwardRegexSearch(const char *string, const char *searchString, int wrap,
	int beginPos, int *startPos, int *endPos, int *searchExtent,
	const char *delimiters, int defaultFlags)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    int length;

    /* compile the search string for searching with ExecRE */
    compiledRE = CompileRE(searchString, &compileMsg, defaultFlags);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to start of file.  A negative begin pos	*/
    /* says begin searching from the far end of the file.		*/
    if (beginPos >= 0) {
	if (ExecRE(compiledRE, NULL, string, string + beginPos, TRUE, '\0',
		'\0', delimiters)) {
	    *startPos = compiledRE->startp[0] - string;
	    *endPos = compiledRE->endp[0] - string;
	    if (searchExtent != NULL)
		*searchExtent = compiledRE->extentp - string;
	    free((char *)compiledRE);
	    return TRUE;
	}
    }
    
    /* if wrap turned off, we're done */
    if (!wrap) {
    	free((char *)compiledRE);
    	return FALSE;
    }
    
    /* search from the end of the string to beginPos */
    if (beginPos < 0)
    	beginPos = 0;
    length = strlen(string); /* sadly, this means scanning entire string */
    if (ExecRE(compiledRE, NULL, string + beginPos, string + length, TRUE,
    	    beginPos==0 ? '\0' : string[beginPos-1], '\0', delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	if (searchExtent != NULL)
	    *searchExtent = compiledRE->extentp - string;
	free((char *)compiledRE);
	return TRUE;
    }
    free((char *)compiledRE);
    return FALSE;
}

static void upCaseString(char *outString, const char *inString)
{
    char *outPtr;
    const char *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = toupper((unsigned char)*inPtr);
    }
    *outPtr = 0;
}

static void downCaseString(char *outString, const char *inString)
{
    char *outPtr;
    const char *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = tolower((unsigned char)*inPtr);
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
    XmProcessTraversal(window->findText, XmTRAVERSE_CURRENT);
}
static void resetReplaceTabGroup(WindowInfo *window)
{
    XmProcessTraversal(window->replaceText, XmTRAVERSE_CURRENT);
}

/*
** Return TRUE if "searchString" exactly matches the text in the window's
** current primary selection using search algorithm "searchType".  If true,
** also return the position of the selection in "left" and "right".
*/
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right, int *searchExtent)
{
    int selLen, selStart, selEnd, startPos, endPos, extent;
    int regexLookaheadContext = isRegexType(searchType) ? 1000 : 0;
    char *string;
    int found, isRect, rectStart, rectEnd, lineStart = 0;
    
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
    
    /* get the selected text plus some additional context for regular
       expression lookahead */
    if (isRect) {
    	string = BufGetRange(window->buffer, lineStart + rectStart,
		lineStart + rectEnd + regexLookaheadContext);
    	selLen = rectEnd - rectStart;
    } else {
	string = BufGetRange(window->buffer, selStart,
		selEnd + regexLookaheadContext);
    	selLen = selEnd - selStart;
    }
    if (*string == '\0') {
    	XtFree(string);
    	return FALSE;
    }
    
    /* search for the string in the selection (we are only interested 	*/
    /* in an exact match, but the procedure SearchString does important */
    /* stuff like applying the correct matching algorithm)		*/
    found = SearchString(string, searchString, SEARCH_FORWARD, searchType,
    	    FALSE, 0, &startPos, &endPos, &extent, GetWindowDelimiters(window));
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
    if (searchExtent != NULL)
	*searchExtent = *right + extent - endPos;
    return TRUE;
}

/*
** Substitutes a replace string for a string that was matched using a
** regular expression.  This was added later and is rather inneficient
** because instead of using the compiled regular expression that was used
** to make the match in the first place, it re-compiles the expression
** and redoes the search on the already-matched string.  This allows the
** code to continue using strings to represent the search and replace
** items.
*/  
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen, int prevChar, char *delimiters, 
        int defaultFlags)
{
    regexp *compiledRE;
    char *compileMsg;
    
    compiledRE = CompileRE(searchStr, &compileMsg, defaultFlags);
    ExecRE(compiledRE, NULL, sourceStr, NULL, False, prevChar, '\0',delimiters);
    SubstituteRE(compiledRE, replaceStr, destStr, maxDestLen);
    free((char *)compiledRE);
}

/*
** Store the search and replace strings, and search type for later recall.
** If replaceString is NULL, duplicate the last replaceString used.
** Contiguous incremental searches share the same history entry (each new
** search modifies the current search string, until a non-incremental search
** is made.  To mark the end of an incremental search, call saveSearchHistory
** again with an empty search string and isIncremental==False.
*/
static void saveSearchHistory(char *searchString, char *replaceString,
	int searchType, int isIncremental)
{
    char *sStr, *rStr;
    static int currentItemIsIncremental = FALSE;
    
    /* Cancel accumulation of contiguous incremental searches (even if the
       information is not worthy of saving) if search is not incremental */
    if (!isIncremental)
	currentItemIsIncremental = FALSE;
    
    /* Don't save empty search strings */
    if (searchString[0] == '\0')
	return;
    
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
    
    /* If the current history item came from an incremental search, and the
       new one is also incremental, just update the entry */
    if (currentItemIsIncremental && isIncremental) {
    	XtFree(SearchHistory[historyIndex(1)]);
    	SearchHistory[historyIndex(1)] = XtNewString(searchString);
	SearchTypeHistory[historyIndex(1)] = searchType;
	return;
    }
    currentItemIsIncremental = isIncremental;
    
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
    	return SEARCH_LITERAL_STRING;
    else if (searchType == SEARCH_CASE_SENSE)
    	return SEARCH_CASE_SENSE_STRING;
    else if (searchType == SEARCH_REGEX)
    	return SEARCH_REGEX_STRING;
    else if (searchType == SEARCH_LITERAL_WORD)
    	return SEARCH_LITERAL_WORD_STRING;
    else if (searchType == SEARCH_CASE_SENSE_WORD)
    	return SEARCH_CASE_SENSE_WORD_STRING;
    else if (searchType == SEARCH_REGEX_NOCASE)
    	return SEARCH_REGEX_NOCASE_STRING;
    else /* Fallback: */
      	return SEARCH_LITERAL;
}

/*
** Return a pointer to the string describing search wrap for search action
** routine parameters (see menu.c for processing of action routines)
*/
static char *searchWrapArg(int searchWrap)
{
    if (searchWrap) {
    	return "wrap";
    }
    return "nowrap";
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

/*
** Checks whether a search mode in one of the regular expression modes.
*/
static int isRegexType(int searchType)
{
    return searchType == SEARCH_REGEX || searchType == SEARCH_REGEX_NOCASE;
}

/*
** Returns the default flags for regular expression matching, given a 
** regular expression search mode.
*/
static int defaultRegexFlags(int searchType)
{
    switch (searchType) {
	case SEARCH_REGEX:
	    return REDFLT_STANDARD;
	case SEARCH_REGEX_NOCASE:
	    return REDFLT_CASE_INSENSITIVE;
	default:
	    /* We should never get here, but just in case ... */
	    return REDFLT_STANDARD;
    }
}   

/* 
** The next 4 callbacks handle the states of find/replace toggle 
** buttons, which depend on the state of the "Regex" button, and the
** sensitivity of the Whole Word buttons.
** Callbacks are necessary for both "Regex" and "Case Sensitive"
** buttons to make sure the states are saved even after a cancel operation.
**
** If sticky case sensitivity is requested, the behaviour is as follows:
**   The first time "Regular expression" is checked, "Match case" gets
**   checked too. Thereafter, checking or unchecking "Regular expression"
**   restores the "Match case" button to the setting it had the last
**   time when literals or REs where used. 
** Without sticky behaviour, the state of the Regex button doesn't influence
** the state of the Case Sensitive button.
** 
** Independently, the state of the buttons is always restored to the 
** default state when a dialog is popped up, and when the user returns
** from stepping through the search history. 
**
** NOTE: similar call-backs exist for the incremental search bar; see window.c.
*/
static void findRegExpToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchRegex = XmToggleButtonGetState(w);
    int searchCaseSense = XmToggleButtonGetState(window->findCaseToggle);
    
    /* In sticky mode, restore the state of the Case Sensitive button */
    if(GetPrefStickyCaseSenseBtn()) {
	if(searchRegex) {
	    window->findLastLiteralCase = searchCaseSense;
	    XmToggleButtonSetState(window->findCaseToggle, 
		window->findLastRegexCase, False);
	} else {
	    window->findLastRegexCase = searchCaseSense;
	    XmToggleButtonSetState(window->findCaseToggle, 
		window->findLastLiteralCase, False);
	}
    }
    /* make the Whole Word button insensitive for regex searches */
    XtSetSensitive(window->findWordToggle, !searchRegex);
}

static void replaceRegExpToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchRegex = XmToggleButtonGetState(w);
    int searchCaseSense = XmToggleButtonGetState(window->replaceCaseToggle);
    
    /* In sticky mode, restore the state of the Case Sensitive button */
    if(GetPrefStickyCaseSenseBtn()) {
	if(searchRegex) {
      	    window->replaceLastLiteralCase = searchCaseSense;
	    XmToggleButtonSetState(window->replaceCaseToggle, 
		window->replaceLastRegexCase, False);
	} else {
      	    window->replaceLastRegexCase = searchCaseSense;
	    XmToggleButtonSetState(window->replaceCaseToggle, 
		window->replaceLastLiteralCase, False);
	}
    }
    /* make the Whole Word button insensitive for regex searches */
    XtSetSensitive(window->replaceWordToggle, !searchRegex);
}

static void iSearchRegExpToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchRegex = XmToggleButtonGetState(w);
    int searchCaseSense = XmToggleButtonGetState(window->iSearchCaseToggle);
    
    /* In sticky mode, restore the state of the Case Sensitive button */
    if(GetPrefStickyCaseSenseBtn()) {
	if(searchRegex) {
      	    window->iSearchLastLiteralCase = searchCaseSense;
	    XmToggleButtonSetState(window->iSearchCaseToggle, 
		window->iSearchLastRegexCase, False);
	} else {
      	    window->iSearchLastRegexCase = searchCaseSense;
	    XmToggleButtonSetState(window->iSearchCaseToggle, 
		window->iSearchLastLiteralCase, False);
	}
    }
    /* The iSearch bar has no Whole Word button to enable/disable. */
}
static void findCaseToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchCaseSense = XmToggleButtonGetState(w);
    
    /* Save the state of the Case Sensitive button 
       depending on the state of the Regex button*/
    if(XmToggleButtonGetState(window->findRegexToggle))
    	window->findLastRegexCase = searchCaseSense;
    else
	window->findLastLiteralCase = searchCaseSense;
}

static void replaceCaseToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchCaseSense = XmToggleButtonGetState(w);
    
    /* Save the state of the Case Sensitive button 
       depending on the state of the Regex button*/
    if(XmToggleButtonGetState(window->replaceRegexToggle))
    	window->replaceLastRegexCase = searchCaseSense;
    else
	window->replaceLastLiteralCase = searchCaseSense;
}

static void iSearchCaseToggleCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo * window = (WindowInfo *) clientData;
    int searchCaseSense = XmToggleButtonGetState(w);
    
    /* Save the state of the Case Sensitive button 
       depending on the state of the Regex button*/
    if(XmToggleButtonGetState(window->iSearchRegexToggle))
    	window->iSearchLastRegexCase = searchCaseSense;
    else
	window->iSearchLastLiteralCase = searchCaseSense;
}



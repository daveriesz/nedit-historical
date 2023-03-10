/*******************************************************************************
*									       *
* selection.c - (some) Nirvana Editor commands operating the primary selection *
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
static char SCCSID[] = "@(#)selection.c	1.9     9/27/94";
#include <stdio.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <X11/Xatom.h>
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "selection.h"
#include "file.h"
#include "window.h"
#include "menu.h"

static void gotoCB(Widget widget, WindowInfo *window, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static void fileCB(Widget widget, WindowInfo *window, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static void processMarkEvent(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch, char *action);
static void markTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void markKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch);
static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch);
static void maintainSelection(selection *sel, int pos, int nInserted,
    	int nDeleted);
static void maintainPosition(int *position, int modPos, int nInserted,
    	int nDeleted);

void GotoLineNumber(WindowInfo *window)
{
    char lineNumText[DF_MAX_PROMPT_LENGTH], *params[1];
    int lineNum, nRead, response;
    
    response = DialogF(DF_PROMPT, window->shell, 2, "Goto Line Number:",
    		       lineNumText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(lineNumText, "%d", &lineNum);
    if (nRead != 1) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = lineNumText;
    XtCallActionProc(window->lastFocus, "goto-line-number", NULL, params, 1);
}
    
void GotoSelectedLineNumber(WindowInfo *window, Time time)
{
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)gotoCB, window, time);
}

void OpenSelectedFile(WindowInfo *window, Time time)
{
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)fileCB, window, time);
}

static void gotoCB(Widget widget, WindowInfo *window, Atom *selection,
    	Atom *type, char *value, int *length, int *format)
{
    char lineText[21];
    int nRead, lineNum;
    
    /* skip if we can't get the selection data, or it's obviously not a number */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	XBell(TheDisplay, 0);
	return;
    }
    if (*length > 20) {
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    strncpy(lineText, value, *length);
    lineText[*length] = '\0';
    
    nRead = sscanf(lineText, "%d", &lineNum);
    XtFree(value);
    if (nRead != 1) {
    	XBell(TheDisplay, 0);
	return;
    }

    SelectNumberedLine(window, lineNum);
}

static void fileCB(Widget widget, WindowInfo *window, Atom *selection,
    	Atom *type, char *value, int *length, int *format)
{
    char nameText[MAXPATHLEN], includeName[MAXPATHLEN];
    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
#ifdef VMS
    static char badFilenameChars[] = "\n \t*?(){}";
#ifndef __DECC
    static char includeDir[] = "sys$library:";
#else
    static char includeDir[] = "decc$library_include:";
#endif
#else
    static char badFilenameChars[] = "\n \t*?()[]{}";
    static char includeDir[] = "/usr/include/";
#endif
    
    /* get the string, or skip if we can't get the selection data, or it's
       obviously not a file name */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	XBell(TheDisplay, 0);
	return;
    }
    if (*length > MAXPATHLEN || *length == 0) {
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
    	XBell(TheDisplay, 0);
	XtFree(value);
	return;
    }
    strncpy(nameText, value, *length);
    XtFree(value);
    nameText[*length] = '\0';
    
    /* extract name from #include syntax */
    if (sscanf(nameText, "#include \"%[^\"]\"", includeName) == 1)
    	strcpy(nameText, includeName);
    else if (sscanf(nameText, "#include <%[^<>]>", includeName) == 1)
    	sprintf(nameText, "%s%s", includeDir, includeName);
    
    /* Check and reject a few obviously non-filename & wildcard characters */
    if (strcspn(nameText, badFilenameChars) != strlen(nameText)) {
    	XBell(TheDisplay, 0);
	return;
    }
    
    /* Open the file */
    ParseFilename(nameText, filename, pathname);
    EditExistingFile(WindowList, filename, pathname, False);
    CheckCloseDim();
}

void SelectNumberedLine(WindowInfo *window, int lineNum)
{
    int i, lineStart, lineEnd;

    /* count lines to find the start and end positions for the selection */
    if (lineNum < 1)
    	lineNum = 1;
    lineEnd = -1;
    for (i=1; i<=lineNum && lineEnd<window->buffer->length; i++) {
    	lineStart = lineEnd + 1;
    	lineEnd = BufEndOfLine(window->buffer, lineStart);
    }
    
    /* highlight the line */
    BufSelect(window->buffer, lineStart, lineEnd+1);
    MakeSelectionVisible(window, window->lastFocus);
    TextSetCursorPos(window->lastFocus, lineStart);
}

void MarkDialog(WindowInfo *window)
{
    char letterText[DF_MAX_PROMPT_LENGTH], *params[1];
    int response;
    
    response = DialogF(DF_PROMPT, window->shell, 2,
"Enter a single letter label to use for recalling\n\
the current selection and cursor position.\n\
\n\
(To skip this dialog, use the accelerator key,\n\
followed immediately by a letter key (a-z))",
    		       letterText, "OK", "Cancel");
    if (response == 2)
    	return;
    if (strlen(letterText) != 1 || !isalpha(letterText[0])) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = letterText;
    XtCallActionProc(window->lastFocus, "mark", NULL, params, 1);
}

void GotoMarkDialog(WindowInfo *window)
{
    char letterText[DF_MAX_PROMPT_LENGTH], *params[1];
    int response;
    
    response = DialogF(DF_PROMPT, window->shell, 2,
"Enter the single letter label used to mark\n\
the selection and/or cursor position.\n\
\n\
(To skip this dialog, use the accelerator\n\
key, followed immediately by the letter)",
    		       letterText, "OK", "Cancel");
    if (response == 2)
    	return;
    if (strlen(letterText) != 1 || !isalpha(letterText[0])) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = letterText;
    XtCallActionProc(window->lastFocus, "goto-mark", NULL, params, 1);
}

/*
** Process a command to mark a selection.  Expects the user to continue
** the command by typing a label character.  Handles both correct user
** behavior (type a character a-z) or bad behavior (do nothing or type
** something else).
*/
void BeginMarkCommand(WindowInfo *window)
{
    XtInsertEventHandler(window->lastFocus, KeyPressMask, False,
    	    markKeyCB, window, XtListHead);
    window->markTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 4000,
    	    markTimeoutProc, window->lastFocus);
}

/*
** Process a command to go to a marked selection.  Expects the user to
** continue the command by typing a label character.  Handles both correct
** user behavior (type a character a-z) or bad behavior (do nothing or type
** something else).
*/
void BeginGotoMarkCommand(WindowInfo *window)
{
    XtInsertEventHandler(window->lastFocus, KeyPressMask, False,
    	    gotoMarkKeyCB, window, XtListHead);
    window->markTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 4000,
    	    markTimeoutProc, window->lastFocus);
}

/*
** Xt timer procedure for removing event handler if user failed to type a
** mark character withing the allowed time
*/
static void markTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    Widget w = (Widget)clientData;
    WindowInfo *window = WidgetToWindow(w);
    
    XtRemoveEventHandler(w, KeyPressMask, False, markKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkKeyCB, window);
    window->markTimeoutID = 0;
}

/*
** Temporary event handlers for keys pressed after the mark or goto-mark
** commands, If the key is valid, grab the key event and call the action
** procedure to mark (or go to) the selection, otherwise, remove the handler
** and give up.
*/
static void processMarkEvent(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch, char *action)
{
    XKeyEvent *e = (XKeyEvent *)event;
    WindowInfo *window = WidgetToWindow(w);
    Modifiers modifiers;
    KeySym keysym;
    char *params[1], string[2];

    XtTranslateKeycode(TheDisplay, e->keycode, e->state, &modifiers,
    	    &keysym);
    if (keysym >= 'A' && keysym <= 'Z' || keysym >= 'a' && keysym <= 'z' ) {
    	string[0] = toupper(keysym);
    	string[1] = '\0';
    	params[0] = string;
    	XtCallActionProc(window->lastFocus, action, event, params, 1);
    	*continueDispatch = False;
    }
    XtRemoveEventHandler(w, KeyPressMask, False, markKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkKeyCB, window);
    XtRemoveTimeOut(window->markTimeoutID);
}
static void markKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch)
{
    processMarkEvent(w, clientData, event, continueDispatch, "mark");
}
static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch)
{
    processMarkEvent(w, clientData, event, continueDispatch, "goto-mark");
}

void AddMark(WindowInfo *window, Widget widget, char label)
{
    int index;
    
    /* look for a matching mark to re-use, or advance
       nMarks to create a new one */
    label = toupper(label);
    for (index=0; index<window->nMarks; index++) {
    	if (window->markTable[index].label == label)
   	    break;
    }
    if (index >= MAX_MARKS) {
    	fprintf(stderr, "no more marks allowed\n"); /* shouldn't happen */
    	return;
    }
    if (index == window->nMarks)
    	window->nMarks++;
    
    /* store the cursor location and selection position in the table */
    window->markTable[index].label = label;
    memcpy(&window->markTable[index].sel, &window->buffer->primary,
    	    sizeof(selection));
    window->markTable[index].cursorPos = TextGetCursorPos(widget);
}

void GotoMark(WindowInfo *window, Widget w, char label)
{
    int index;
    selection *sel;
    
    /* look up the mark in the mark table */
    label = toupper(label);
    for (index=0; index<window->nMarks; index++) {
    	if (window->markTable[index].label == label)
   	    break;
    }
    if (index == window->nMarks) {
    	XBell(TheDisplay, 0);
    	return;
    }
    
    /* reselect marked the selection, and move the cursor to the marked pos */
    sel = &window->markTable[index].sel;
    if (sel->selected) {
    	if (sel->rectangular)
    	    BufRectSelect(window->buffer, sel->start, sel->end, sel->rectStart,
            	    sel->rectEnd);
    	else
    	    BufSelect(window->buffer, sel->start, sel->end);
    }
    /*... Note that MakeSelectionVisible is not great with multi-line
          selections, and here we will sometimes give it one */
    MakeSelectionVisible(window, window->lastFocus);
    TextSetCursorPos(w, window->markTable[index].cursorPos);
}

/*
** Keep the marks in the windows book-mark table up to date across
** changes to the underlying buffer
*/
void UpdateMarkTable(WindowInfo *window, int pos, int nInserted,
    	int nDeleted)
{
    int i;
    
    for (i=0; i<window->nMarks; i++) {
    	maintainSelection(&window->markTable[i].sel, pos, nInserted,
    	    	nDeleted);
    	maintainPosition(&window->markTable[i].cursorPos, pos, nInserted,
    	    	nDeleted);
    }
}

/*
** Update a selection across buffer modifications specified by
** "pos", "nDeleted", and "nInserted".
*/
static void maintainSelection(selection *sel, int pos, int nInserted,
	int nDeleted)
{
    if (!sel->selected || pos > sel->end)
    	return;
    maintainPosition(&sel->start, pos, nInserted, nDeleted);
    maintainPosition(&sel->end, pos, nInserted, nDeleted);
    if (sel->end <= sel->start)
	sel->selected = False;
}

/*
** Update a position across buffer modifications specified by
** "modPos", "nDeleted", and "nInserted".
*/
static void maintainPosition(int *position, int modPos, int nInserted,
    	int nDeleted)
{
    if (modPos > *position)
    	return;
    if (modPos+nDeleted <= *position)
    	*position += nInserted - nDeleted;
    else
    	*position = modPos;
}

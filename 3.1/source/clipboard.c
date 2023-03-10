/*******************************************************************************
*									       *
* clipboard.c -- Nirvana Editor clipboard interface			       *
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
static char SCCSID[] = "@(#)clipboard.c	1.9     9/27/94";
#include <stdio.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/CutPaste.h>
#include <X11/Xatom.h>		/* for getting selection */
#ifdef MOTIF10
#include <X11/Selection.h>	/* " " */
#endif
#include <X11/X.h>		/* " " */
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "nedit.h"
#include "clipboard.h"
#include "file.h"
#include "window.h"

static int checkLock(WindowInfo *window, int clipStatus);
static void gotoCB(Widget widget, WindowInfo *window, Atom *selection,
		   Atom *type, char *value, int *length, int *format);
static void fileCB(Widget widget, WindowInfo *window, Atom *selection,
		   Atom *type, char *value, int *length, int *format);

void CutToClipboard(WindowInfo *window, Time time)
{
#ifdef MOTIF10
    /* Copy selection to clipboard */
    CopyToClipboard(window, time);
 
    /* Delete primary selection */
    DeletePrimarySelection(window->textArea);
#else
    XmTextCut(window->textArea, time);
#endif
}

void CopyToClipboard(WindowInfo *window, Time time)
{
#ifndef MOTIF10
    XmTextCopy(window->textArea, time);
#else
    char *selectionString;
    unsigned long itemID = 0;
    int dataId = 0;
    int status;
    XmString clipLabel;
    Window textWindow = XtWindow(window->textArea);
    
 
    /* Get the selected text, if none selected, return */
    selectionString = XmTextGetSelection (window->textArea);
    if (selectionString == NULL)
    	return;

    /* Initiate the copy, if locked, ask user and try again */
    clipLabel = XmStringCreateLtoR ("nedit", CHARSET);
    status = 0;
    while (status != ClipboardSuccess) {
	status = XmClipboardStartCopy (TheDisplay, textWindow, clipLabel, time,
				       window->textArea, NULL, &itemID);
	if (checkLock(window, status))
	    return;
    }
    XmStringFree(clipLabel);
    
    /* move the data to the clipboard, , if locked, ask user and try again */
    status = 0;
    while (status != ClipboardSuccess) {
	status = XmClipboardCopy (TheDisplay, textWindow, itemID, "STRING",
				  selectionString,
				  (long)strlen(selectionString)+1, 0, &dataId);
	if (checkLock(window, status))
	    return;
    }
    /* End the copy to the clipboard, if locked, ask user and try again */
    status = 0;
    while (status != ClipboardSuccess) {
	status = XmClipboardEndCopy (TheDisplay, textWindow, itemID);
	if (checkLock(window, status))
	    return;
    }
#endif /* MOTIF10 */
}

/*
** retrieve the current data from the clipboard
**    and paste it at the current cursor position
*/
void PasteFromClipboard(WindowInfo *window, Time time)
{
#ifndef MOTIF10
    XmTextPaste(window->lastFocus);
#else
    char * selectionString = XmTextGetSelection (window->textArea);
					       /* containts of selection  */
    int status = 0;			      /* clipboard status	 */
    char *buffer;			      /* temporary text buffer 	 */
    int length;				      /* length of buffer     	 */
    int outlength = 0;			      /* length of bytes copied	 */
    int private_id = 0;			      /* id of item on clipboard */
    XmTextPosition cursorPos;		      /* text cursor position 	 */
    int ac;				      /* arg count  	      	 */
    Widget textArea = window->textArea;
    int pendingDelete;
    int left, right;
 
    /* find the length of the paste item, continue till the length is found */
    status = 0;
    while (status != ClipboardSuccess) {
	status = XmClipboardInquireLength(TheDisplay, XtWindow(textArea),
					 "STRING", &length);
	if (status == ClipboardNoData)
	    return;
	if (checkLock(window, status))
	    return;
	if (length == 0)
	    return;
    }
 
    /* malloc to necessary space */
    buffer = XtMalloc(length);
 
    status = 0;
    while (status != ClipboardSuccess) {
	status = XmClipboardRetrieve(TheDisplay, XtWindow(textArea),
			    "STRING", buffer, length, &outlength, &private_id);
	if (checkLock(window, status)) {
	    XtFree(buffer);
	    return;
	}
	if (status == ClipboardNoData) {
	    XtFree(buffer);
	    return;
	} else if (status == ClipboardTruncate) {
	    DialogF(DF_WARN, textArea, 1 ,"Some clipboard data has been lost",
	    	    "OK");
	    XtFree(buffer);
	    return;
	}
    }
 
    GET_ONE_RSRC(textArea, XmNpendingDelete, &pendingDelete)
    if (GetSelection(window->textArea, &left, &right) && pendingDelete) {
	XmTextReplace(textArea, left, right, buffer);
	XmTextClearSelection(window->textArea, time);
    } else {
	cursorPos = GetInsertPosition(window);
	XmTextReplace(textArea, cursorPos, cursorPos, buffer);
    }
    XtFree(buffer);

#endif /* MOTIF10 */
}

#ifdef MOTIF10	/* this routine works, but is not needed */
static int checkLock(WindowInfo *window, int clipStatus)
{
    int response;

    if (clipStatus == ClipboardLocked) {
	response = DialogF(DF_QUES, window->textArea, 2,
			   "Clipboard is locked\nby another application",
			   "Give Up", "Try Again");
	if (response == 1)
	    return TRUE;
    }
    return FALSE;
}
#endif /* MOTIF10 */

void GotoLineNumber(WindowInfo *window, Time time)
{
    char lineNumText[DF_MAX_PROMPT_LENGTH];
    int lineNum, nRead, response;
    
    response = DialogF(DF_PROMPT, window->shell, 2, "Goto Line Number:",
    		       lineNumText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(lineNumText, "%d", &lineNum);
    if (nRead != 1) {
    	XBell(TheDisplay, 100);
	return;
    }
    SelectNumberedLine(window, lineNum, time);
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
    	XBell(TheDisplay, 100);
	return;
    }
    if (*length > 20) {
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    strncpy(lineText, value, *length);
    lineText[*length] = '\0';
    
    nRead = sscanf(lineText, "%d", &lineNum);
    XtFree(value);
    if (nRead != 1) {
    	XBell(TheDisplay, 100);
	return;
    }

    SelectNumberedLine(window, lineNum, CurrentTime);
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
    	XBell(TheDisplay, 100);
	return;
    }
    if (*length > MAXPATHLEN || *length == 0) {
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
    	XBell(TheDisplay, 100);
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
    	XBell(TheDisplay, 100);
	return;
    }
    
    /* Open the file */
    ParseFilename(nameText, filename, pathname);
    EditExistingFile(WindowList, filename, pathname, False);
}

void SelectNumberedLine(WindowInfo *window, int lineNum, Time time)
{
    char *fileString, *filePtr;
    int lineCount = 0, lastLine = 0, startPos = 0, endPos = 0, i, topChar;

    if (lineNum < 1)
    	lineNum = 1;
	
    /* get the entire (sigh) text buffer from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1,
		"Out of memory!\nTry closing some windows.\nSave your files!",
		"OK");
	XtFree(fileString);
	return;
    }
    
    /* count lines to find the start and end positions for the selection */
    for (filePtr=fileString, i=0; *filePtr!=0; filePtr++, i++) {
    	if (*filePtr == '\n') {
	    lineCount++;
	    if (lineCount == lineNum) {
		startPos = lastLine;
		endPos = i + 1;
		break;
	    }
	    lastLine = i + 1;
	}
    }
    XtFree(fileString);
    if (endPos == 0) {
    	/* went through whole file and didn't find lineNum, select last line */
	startPos = lastLine;
	endPos = i;
    }
    
    /* highlight the line */
    XmTextSetSelection(window->lastFocus, startPos, endPos, time);

    /* the text widget autoShowCursorPosition only ensures that the insert
       point will be shown.  Make sure the whole selection is visible */
    topChar = XmTextGetTopCharacter(window->lastFocus);
    if (topChar > startPos)
    	XmTextScroll(window->lastFocus, -1);
}

/*******************************************************************************
*									       *
* window.c -- Nirvana Editor window creation/deletion			       *
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
static char SCCSID[] = "@(#)window.c	1.23     3/15/94";
#include <stdio.h>
#include <stdlib.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <limits.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#ifdef IBM_PROTO_BUG
#define _NO_PROTO	/* Problem with function prototypes in */
#include <Xm/TextP.h>	/*   TextP.h under IBM AIX 3.2 */
#undef _NO_PROTO
#else
#include <Xm/TextP.h>
#endif
#ifndef MOTIF12
#include <Xm/TextSrcP.h>
#endif
#include <Xm/TextOutP.h>	/* only for NOLINE constant */
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "nedit.h"
#include "window.h"
#include "menu.h"
#include "search.h"
#include "undo.h"
#include "preferences.h"
#include "nedit.bm"
#include "file.h"

#define PANE_MIN_HEIGHT 45

/* from motif-2.3.8/lib/Xm/TextStrOoI.h */
extern char  * _XmStringSourceGetString(XmTextWidget tw, XmTextPosition from, XmTextPosition to, int want_wchar);
/* from motif-2.3.8/lib/Xm/TextI.h */
extern LineNum _XmTextPosToLine(XmTextWidget widget, XmTextPosition position);

/* Original text source procedure for scanning word, line, and paragraph
   boundaries.  Gets replaced with but also used by replacementScanProc */
static ScanProc OriginalScanProc;

/* Original text source procedure for counting newlines in a range of text */
static CountLinesProc OriginalCountProc;

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols);
static void addToWindowList(WindowInfo *window);
static void removeFromWindowList(WindowInfo *window);
static void focusCB(Widget w, WindowInfo *window, XmTextVerifyPtr call_data);
static void modifiedCB(Widget w, WindowInfo *window, XmTextVerifyPtr call_data);
static void movedCB(Widget w, WindowInfo *window, XmTextVerifyPtr call_data);
static void closeCB(Widget w, WindowInfo *window, caddr_t call_data);
static void updateStatsLine(WindowInfo *window, int pos);
static void changeWordDelimiters(Widget text);
static void patchScrollBarBug(Widget text);
static XmTextPosition replacementScanProc(XmTextSource source,
	XmTextPosition pos, XmTextScanType sType, XmTextScanDirection dir,
	int count, Boolean include);
static int replacementCountProc(XmTextSource source, XmTextPosition start,
	int length);
void recreateWindow(WindowInfo *window, char *font, int wrap);
static int isDelimeter(char c);
static void changeAutoIndent(Widget text, int state);
static void changeTabDist(Widget text, int tabWidth);
static void addRightBtnActions(Widget text);
static int textPosToLineNum(Widget text, int pos);
static void setPaneDesiredHeight(Widget w, int height);
static int columnNumber(Widget textW, XmTextPosition pos, int tabDist);

/*
** Create a new editor window
*/
WindowInfo *CreateWindow(char *title, int rows, int cols, int autoIndent, 
	int autoSave, char *fontName, int wrap, int tabDist)
{
    Widget appShell, main, menuBar, pane, text, stats;
    WindowInfo *window;
    Arg al[20];
    int ac;
    static Pixmap iconPixmap = 0, maskPixmap = 0;

    /* Allocate some memory for the new window data structure */
    window = (WindowInfo *)XtMalloc(sizeof(WindowInfo));
    
    /* initialize window structure */
    window->replaceDlog = NULL;
    window->replaceText = NULL;
    window->replaceWithText = NULL;
    window->replaceLiteralBtn = NULL;
    window->replaceCaseBtn = NULL;
    window->replaceRegExpBtn = NULL;
    window->findDlog = NULL;
    window->findText = NULL;
    window->findLiteralBtn = NULL;
    window->findCaseBtn = NULL;
    window->findRegExpBtn = NULL;
    window->fileChanged = FALSE;
    window->fileMode = 0;
    window->filenameSet = FALSE;
    window->undo = NULL;
    window->redo = NULL;
    window->nPanes = 0;
    window->autoSaveCharCount = 0;
    window->autoSaveOpCount = 0;
    window->undoOpCount = 0;
    window->undoMemUsed = 0;
    window->readOnly = FALSE;
    window->autoIndent = autoIndent;
    window->autoSave = autoSave;
    window->wrap = wrap;
    window->tabDist = tabDist;
    window->ignoreModify = FALSE;
    window->windowMenuValid = FALSE;

    /* Get a fontlist for the text widget from the specified font name */
    strcpy(window->fontName, fontName);
    window->font = XLoadQueryFont(TheDisplay, window->fontName);
    if (window->font != NULL)
    	window->fontList = XmFontListCreate(window->font,
    		XmSTRING_DEFAULT_CHARSET);
    else
    	window->fontList = NULL;
    
    /* Create a new toplevel shell to hold the window */
    ac = 0;
    XtSetArg(al[ac], XmNtitle, title); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(al[ac], XmNiconName, title); ac++;
    appShell = XtAppCreateShell("nedit", "NEdit",
		applicationShellWidgetClass, TheDisplay, al, ac);
    window->shell = appShell;

    /* Add the icon.  There is something weird about the  XmNiconPixmap
       resource that prevents it from being set from the defaults in
       the application resource database.  It's just hardwired here. */
    if (iconPixmap == 0) {
    	iconPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen (appShell)), (char *)iconBits,
    		iconBitmapWidth, iconBitmapHeight);
    	maskPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen (appShell)), (char *)maskBits,
    		iconBitmapWidth, iconBitmapHeight);
    }
    XtVaSetValues(appShell, XmNiconPixmap, iconPixmap,
    	    XmNiconMask, maskPixmap, NULL);

    /* Create a MainWindow to manage the menubar and text area */
    main = XmCreateMainWindow(appShell, "main", NULL, 0);
    XtManageChild(main);
    
    /* Create file statistics display area */
    stats = XtVaCreateWidget("stats", xmLabelWidgetClass,  main,
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNfontList, window->fontList, NULL);
    window->statsLine = stats;
    if (GetPrefStatsLine())
    	XtManageChild(window->statsLine);

    /* Create the menu bar */
    menuBar = CreateMenuBar(main, window);
    XtManageChild(menuBar);
    
    /* Create paned window to manage split window behavior */
    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass,  main,
    	    XmNmarginWidth, 0, XmNmarginHeight, 0, XmNseparatorOn, False,
    	    XmNspacing, 3, XmNsashIndent, -2, XmNsashHeight, 11,
            XmNsashWidth, 11, NULL);
    window->splitPane = pane;
    XmMainWindowSetAreas(main, menuBar, stats, NULL, NULL, pane);

    /* Create the first, and most permanent text area (other panes may
       be added & removed, but this one will never be removed */
    text = createTextArea(pane, window, rows, cols);
    XtAddCallback(text, XmNmodifyVerifyCallback, (XtCallbackProc)modifiedCB,
    	    window);
    XtManageChild(text);
    window->textArea = text;
    window->lastFocus = text;
    
    /* add the window to the global window list, update the Windows menus */
    addToWindowList(window);
    InvalidateWindowMenus();
    
    /* realize all of the widgets in the new window */
    XtRealizeWidget(appShell);

    /* Make close command in window menu gracefully prompt for close */
    AddMotifCloseCallback(appShell, (XtCallbackProc)closeCB, window);

    return window;
}

/*
** Close an editor window
*/
void CloseWindow(WindowInfo *window)
{
    /* if this is the last window, don't close it, make it Untitled */
    if (WindowList == window && window->next == NULL) {
	window->readOnly = FALSE;
	strcpy(window->filename, "Untitled");
	strcpy(window->path, "");
	window->ignoreModify = TRUE;
	XmTextSetString(window->textArea, "");
	window->ignoreModify = FALSE;
	window->filenameSet = FALSE;
	window->fileChanged = FALSE;
	UpdateWindowTitle(window);
	XtSetSensitive(window->closeItem, FALSE);
    	ClearUndoList(window);
    	ClearRedoList(window);
	XmTextSetEditable(window->textArea, TRUE);
	return;
    }
   
    /* remove and deallocate all of the widgets associated with window */
    XtDestroyWidget(window->shell);
    
    /* remove the window from the global window list, update window menus */
    removeFromWindowList(window);
    InvalidateWindowMenus();
    
    /* free the font and fontList */
    if (window->fontList != NULL) {
	XmFontListFree(window->fontList);
	XFreeFont(TheDisplay, window->font);
    }
    
    /* free the undo and redo lists */
    ClearUndoList(window);
    ClearRedoList(window);
    
    /* deallocate the window data structure */
    XtFree((char *)window);
}

/*
** Check if there is already a window open for a given file
*/
WindowInfo *FindWindowWithFile(char *name, char *path)
{
    WindowInfo *w;

    for (w=WindowList; w!=NULL; w=w->next) {
    	if (!strcmp(w->filename, name) && !strcmp(w->path, path)) {
	    return w;
	}
    }
    return NULL;
}

/*
** Add another independently scrollable window pane to the current window,
** splitting the pane which currently has keyboard focus.
*/
void SplitWindow(WindowInfo *window)
{
    short paneHeights[MAX_PANES+1];
    XmTextPosition selStart, selEnd;
    XmTextPosition insertPositions[MAX_PANES+1], topChars[MAX_PANES+1];
    int i, focusPane, hasSelection, totalHeight=0;
    Widget text;
    char *contents;
    
    /* Don't create new panes if we're already at the limit */
    if (window->nPanes >= MAX_PANES)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, keyboard focus, and selection status */
    hasSelection = XmTextGetSelectionPosition(window->textArea,
    	    &selStart, &selEnd);
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = XmTextGetInsertionPosition(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], NULL);
    	totalHeight += paneHeights[i];
    	topChars[i] = XmTextGetTopCharacter(text);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Create a text widget to add to the pane */
    text = createTextArea(window->splitPane, window, 1, 1); 
#ifdef MOTIF12
    XtAddCallback(text, XmNmodifyVerifyCallback, (XtCallbackProc)modifiedCB,
    	    window);
#endif
    XtManageChild(text);
    XmTextSetSource(text, XmTextGetSource(window->textArea), 0, 0);
    window->textPanes[window->nPanes++] = text;
    
    /* New text widgets with shared sources don't properly adapt to existing
       text contents.  Contents must be reset for scroll bars to work right */
    contents = XmTextGetString(window->textArea);
    window->ignoreModify = True;
    XmTextSetString(window->textArea, "");
    XmTextSetString(window->textArea, contents);
    window->ignoreModify = False;
    XtFree(contents);
    
    /* adjust the heights, scroll positions, etc., to split the focus pane */
    for (i=window->nPanes; i>focusPane; i--) {
    	insertPositions[i] = insertPositions[i-1];
    	paneHeights[i] = paneHeights[i-1];
    	topChars[i] = topChars[i-1];
    }
    paneHeights[focusPane] = paneHeights[focusPane]/2;
    paneHeights[focusPane+1] = paneHeights[focusPane];
    
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	setPaneDesiredHeight(XtParent(text), paneHeights[i]);
    }

    /* Re-manage panedWindow to recalculate pane heights & reset selection */
    XtManageChild(window->splitPane);
    
    /* Reset all of the heights, scroll positions, etc. */
    if (hasSelection )
	XmTextSetSelection(window->textArea, selStart, selEnd, CurrentTime);
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	XmTextSetInsertionPosition(text, insertPositions[i]);
	XmTextSetTopCharacter(text, topChars[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
}

/*
** Close the window pane that last had the keyboard focus.  (Actually, close
** the bottom pane and make it look like pane which had focus was closed)
*/
void ClosePane(WindowInfo *window)
{
    short paneHeights[MAX_PANES+1];
    XmTextPosition selStart, selEnd;
    XmTextPosition insertPositions[MAX_PANES+1], topChars[MAX_PANES+1];
    int i, focusPane, hasSelection, totalHeight=0;
    Widget text;
    
    /* Don't delete the last pane */
    if (window->nPanes <= 0)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, keyboard focus, and selection status */
    hasSelection = XmTextGetSelectionPosition(window->textArea,
    	    &selStart, &selEnd);
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = XmTextGetInsertionPosition(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], NULL);
    	totalHeight += paneHeights[i];
    	topChars[i] = XmTextGetTopCharacter(text);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Destroy last pane, and make sure lastFocus points to an existing pane */
    XtDestroyWidget(window->textPanes[--window->nPanes]);
    if (window->nPanes == 0)
	window->lastFocus = window->textArea;
    else if (focusPane > window->nPanes)
	window->lastFocus = window->textPanes[window->nPanes-1];
    
    /* adjust the heights, scroll positions, etc., to make it look
       like the pane with the input focus was closed */
    for (i=window->nPanes; i>=focusPane; i--) {
    	insertPositions[i] = insertPositions[i+1];
    	paneHeights[i] = paneHeights[i+1];
    	topChars[i] = topChars[i+1];
    }
    
    /* set the desired heights and re-manage the paned window so it will
       recalculate pane heights */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	setPaneDesiredHeight(XtParent(text), paneHeights[i]);
    }
    XtManageChild(window->splitPane);
    
    /* Reset all of the selection, scroll positions, insert positions, etc. */
    if (hasSelection )
	XmTextSetSelection(window->textArea, selStart, selEnd, CurrentTime);
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	XmTextSetInsertionPosition(text, insertPositions[i]);
	XmTextSetTopCharacter(text, topChars[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
}

/*
** Turn on and off the display of the statistics line
*/
void ShowStatsLine(WindowInfo *window, int state)
{
    /* The very silly use of XmNcommandWindowLocation below is just to kick
       the main window widget to remove the status line when it is unmanaged */
    if (state) {
    	XtVaSetValues(XtParent(window->statsLine),
   		XmNcommandWindowLocation, XmCOMMAND_ABOVE_WORKSPACE, NULL);
    	XtManageChild(window->statsLine);
   	updateStatsLine(window, -1);
    } else {
     	XtUnmanageChild(window->statsLine);
   	XtVaSetValues(XtParent(window->statsLine),
   		XmNcommandWindowLocation,  XmCOMMAND_BELOW_WORKSPACE, NULL);
    }
}

/*
** Count the windows
*/
int NWindows()
{
    WindowInfo *win;
    int n;
    
    for (win=WindowList, n=0; win!=NULL; win=win->next, n++);
    return n;
}

/*
** Turn on and off autoindent
*/
void SetAutoIndent(WindowInfo *window, int state)
{
    int i;
    
    window->autoIndent = state;
    changeAutoIndent(window->textArea, state);
    for (i=0; i<window->nPanes; i++)
    	changeAutoIndent(window->textPanes[i], state);
}

/*
** Set the font for a window
*/
void SetFont(WindowInfo *window, char *fontName)
{
    recreateWindow(window, fontName, window->wrap);
}

/*
** Turn text wrapping on and off
*/
void SetWrap(WindowInfo *window, int wrap)
{
    recreateWindow(window, window->fontName, wrap);
}

/*
** Set the number of characters to a tab stop
*/
void SetTabDistance(WindowInfo *window, int tabWidth)
{
    int i;
    
    /* set the tab distance for all panes */
    changeTabDist(window->textArea, tabWidth);
    for (i=0; i<window->nPanes; i++)
    	changeTabDist(window->textPanes[i], tabWidth);
    window->tabDist = tabWidth;
    
    /* temporarily add and remove a newline at the beginning of the text
       area to force a redisplay */
    window->ignoreModify = True;
    XmTextInsert(window->textArea, 0, "\n");
    XmTextReplace(window->textArea, 0, 1, "");
    window->ignoreModify = False;
}

/*
** To reset font or text wrapping, it is necessary to close
** and re-create the whole window.  (the paned window won't allow width resize,
** and text won't allow change to wrap).
*/
void recreateWindow(WindowInfo *window, char *font, int wrap)
{
    WindowInfo *newWindow;
    char *s1;
    short rows, cols;
    
    /* create a new window and transfer values from the old one */
    XtVaGetValues(window->textArea, XmNrows, &rows, XmNcolumns, &cols, NULL);
    newWindow = CreateWindow("", rows, cols, window->autoIndent,
    	    window->autoSave, font, wrap, window->tabDist);
    if (XtIsManaged(window->statsLine))
    	XtManageChild(newWindow->statsLine);
    XtSetSensitive(newWindow->undoItem, XtIsSensitive(window->undoItem));
    newWindow->ignoreModify = TRUE;
    XmTextSetString(newWindow->textArea, s1=XmTextGetString(window->textArea));
    XtFree(s1);
    newWindow->ignoreModify = FALSE;
    XmTextSetInsertionPosition(newWindow->textArea,
    	    XmTextGetInsertionPosition(window->textArea));
    XmTextSetTopCharacter(newWindow->textArea,
   	    XmTextGetTopCharacter(window->textArea));
    strcpy(newWindow->filename, window->filename);
    strcpy(newWindow->path, window->path);
    newWindow->fileMode = window->fileMode;
    newWindow->undo = window->undo;
    newWindow->redo = window->redo;
    newWindow->autoSaveCharCount = window->autoSaveCharCount;
    newWindow->autoSaveOpCount = window->autoSaveOpCount;
    newWindow->undoOpCount = window->undoOpCount;
    newWindow->undoMemUsed = window->undoMemUsed;
    newWindow->filenameSet = window->filenameSet;
    newWindow->fileChanged = window->fileChanged;
    newWindow->readOnly = window->readOnly;
    UpdateWindowTitle(newWindow);
    
    /* close the old window */
    XtDestroyWidget(window->shell);
    removeFromWindowList(window);
    if (window->fontList != NULL) {
	XmFontListFree(window->fontList);
	XFreeFont(TheDisplay, window->font);
    }
    XtFree((char *)window);
}

/*
** Change the window appearance and the window data structure to show
** that the file it contains has been modified
*/
void SetWindowModified(WindowInfo *window, int modified)
{
    if (window->fileChanged == FALSE && modified == TRUE) {
    	XtSetSensitive(window->closeItem, TRUE);
    	window->fileChanged = TRUE;
    	UpdateWindowTitle(window);
    } else if (window->fileChanged == TRUE && modified == FALSE) {
    	window->fileChanged = FALSE;
    	UpdateWindowTitle(window);
    }
}

/*
** Update the window title to reflect the filename, read-only, and modified
** status of the window data structure
*/
void UpdateWindowTitle(WindowInfo *window)
{
    char title[MAXPATHLEN + 11]; 	/* 11: strlen(" (read only)") */

    strcpy(title, window->filename);
    if (window->readOnly)
    	strcat(title, " (read only)");
    else if (window->fileChanged)
    	strcat(title, " (modified)");
    SetWindowTitle(window, title);
}

void SetWindowTitle(WindowInfo *window, char *title)
{
    /* Set both the window title and the icon title */
    XtVaSetValues(window->shell, XmNtitle, title, XmNiconName, title, NULL);

    /* Update the Windows menus with the new name */
    InvalidateWindowMenus();
}

/*
** Get the start and end of the current selection.  This routine can now
** be phased out because Motif 1.1 now provides the functionality in
** XmTextGetSelectionPosition.
*/
int GetSelection(Widget widget, int *left, int *right)
{
    return XmTextGetSelectionPosition(widget, (XmTextPosition *)left,
    	    (XmTextPosition *)right) && !(*left == *right);    
}

/*
** Returns a range of text from a text widget.  Memory is allocated with
** XtMalloc and caller should free it.  Depends on private widget data,
** so it will probably break later.
*/
char *GetTextRange(Widget widget, int left, int right)
{
#ifndef MOTIF10
#ifdef MOTIF12
    return _XmStringSourceGetString((XmTextWidget)widget, left, right, False);
#else
    return _XmStringSourceGetString((XmTextWidget)widget, left, right);
#endif   
#else
    int length;
    char *fileString;
    XmTextWidget tw = (XmTextWidget)widget;
    XmTextSource source = tw->text.source;
    
    length = right - left;
    fileString = (char *)XtMalloc(length+1);
    strncpy(fileString, (char *)&(source->data->ptr[left]), length);
    fileString[length] = 0;
    return fileString;
#endif
}

/*
** Return the length of the text in the text widget.  Note: depends on
** private widget data and may break in a future release of Motif.  (I
** think this could be replaced with XmTextGetLastPosition).
*/
int TextLength(Widget widget)
{
    return ((XmTextWidget)widget)->text.last_position;
}

/*
** Make sure that the selection is shown in the window.  (Well actually,
** just nudge the text up or down one line to compensate for the text
** widget's inability to do XmNautoShowCursorPosition correctly under 1.2)
*/
void MakeSelectionVisible(Widget textW)
{
    XmTextPosition left, right;
    XmTextPosition topChar = XmTextGetTopCharacter(textW);
    Position x, y;
    
    /* find out where the selection is */
    if (!XmTextGetSelectionPosition(textW, &left, &right))
    	return;
    	
    /* if the end of the selection is below bottom, scroll up one line */
    if (right > topChar && !XmTextPosToXY(textW, right, &x, &y)) {
    	XmTextScroll(textW, 1);
    	return;
    }
    
    /* if the beginning of the selection is before the top, scroll down */
    if (topChar > left)
    	XmTextScroll(textW, -1);
}

int GetInsertPosition(WindowInfo *window)
{
    int position;
    GET_ONE_RSRC(window->textArea, XmNcursorPosition, &position);
    return position;
}

void DeletePrimarySelection(Widget w)
{
#ifdef MOTIF10
   XClientMessageEvent cm;

   /* send a client message to the text widget with action as atom */
   cm.type = ClientMessage;
   cm.display = TheDisplay;
   cm.message_type = XmInternAtom(TheDisplay, "KILL_SELECTION", FALSE);
   cm.window = XtWindow(w);
   cm.format = 32;
   cm.data.l[0] = XA_PRIMARY;
   XSendEvent(TheDisplay, cm.window, TRUE, NoEventMask, &cm);
#else
    int selStart, selEnd;
    
    if (GetSelection(w, &selStart, &selEnd))
    	XmTextReplace(w, selStart, selEnd, "");
#endif
}

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols)
{
    Arg al[20];
    int ac;
    Widget text;
    Widget vsb = NULL;
    Pixel troughColor;
        
    /* create text and scrolled window widget.  Even though the main
       window is a subclass of a scrolled window, we still need to create
       an additional scrolled window below it.  The text widget does not
       recognize a main window as a scrolled window & refuses to create
       the necessary scroll bar widgets. */
    ac = 0;
    XtSetArg(al[ac], XmNresizeWidth, False);  ac++;
    XtSetArg(al[ac], XmNresizeHeight, False);  ac++;
    XtSetArg(al[ac], XmNscrollVertical, True);  ac++;
    XtSetArg(al[ac], XmNwordWrap, window->wrap);  ac++;
    XtSetArg(al[ac], XmNscrollHorizontal, !window->wrap);  ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);  ac++;
    XtSetArg(al[ac], XmNautoShowCursorPosition, True);  ac++;
    XtSetArg(al[ac], XmNtraversalOn, True);  ac++;
    XtSetArg(al[ac], XmNspacing, 0);  ac++;
    XtSetArg(al[ac], XmNfontList, window->fontList);  ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNrows, rows);  ac++;
    XtSetArg(al[ac], XmNcolumns, cols);  ac++;
    XtSetArg(al[ac], XmNpaneMaximum, SHRT_MAX);  ac++;
    XtSetArg(al[ac], XmNpaneMinimum, PANE_MIN_HEIGHT);  ac++;
    text = XmCreateScrolledText(parent, "text", al, ac);
    
    /* add focus tracking callback */
    XtAddCallback(text, XmNfocusCallback, (XtCallbackProc)focusCB, window);
    XtAddCallback(text, XmNmotionVerifyCallback, (XtCallbackProc)movedCB,
    	    window);

    /* this makes sure the text area initially has a the insert point shown */
    XmAddTabGroup(XtParent(text));
    
    /* get the scroll bar widgets to speed them up */
    ac = 0;
    XtSetArg(al[ac], XmNverticalScrollBar, &vsb); ac++;
    XtGetValues(XtParent(text), al, ac);
    ac = 0;
    XtSetArg(al[ac], XmNrepeatDelay, 10); ac++;
    XtSetValues(vsb, al, ac);

    /* Set the little square in the corner between the scroll
       bars to be the same color as the scroll bar interiors  */
    ac = 0;
    XtSetArg(al[ac], XmNtroughColor, &troughColor); ac++;
    XtGetValues(vsb, al, ac);
    ac = 0;
    XtSetArg(al[ac], XmNbackground, troughColor); ac++;
    XtSetValues(XtParent(text), al, ac);

    /* kludge over delete/backspace Motif deficiency */
    RemapDeleteKey(text);

    /* add right button equivalents of quick-copy actions */
    addRightBtnActions(text);
    
    /* set rational word delimiters that include punctuation */
    changeWordDelimiters(text);
    
    /* make auto indent and tab distance settings agree with rest of window */
    changeAutoIndent(text, window->autoIndent);
    changeTabDist(text, window->tabDist);

    /* patch a scroll bar synchronization problem */
    patchScrollBarBug(text);
    
    return text;
}

static void movedCB(Widget w, WindowInfo *window, XmTextVerifyPtr callData) 
{
    updateStatsLine(window, callData->newInsert);
}

static void modifiedCB(Widget w, WindowInfo *window, XmTextVerifyPtr callData) 
{
    /* When the program needs to make a change to a text area without without
       recording it for undo or marking file as changed it sets ignoreModify */
    if (window->ignoreModify)
    	return;
    
    /* Prevent read-only windows from being modified */
    if (window->readOnly) {
	callData->doit = FALSE;
	return;
    }
    
    /* Save information for undoing this operation (this call also counts
       characters and editing operations for triggering autosave */
    SaveUndoInformation(window, callData);
    
    /* Trigger automatic backup if operation or character limits reached */
    if (window->autoSave &&
    	    (window->autoSaveCharCount > AUTOSAVE_CHAR_LIMIT ||
    	     window->autoSaveOpCount > AUTOSAVE_OP_LIMIT)) {
    	WriteBackupFile(window);
    	window->autoSaveCharCount = 0;
    	window->autoSaveOpCount = 0;
    }
    
    /* Indicate that the window has now been modified */ 
    SetWindowModified(window, TRUE);
    
    /* Update # of bytes, and line and col statistics */
    updateStatsLine(window, callData->newInsert);
}

static void focusCB(Widget w, WindowInfo *window, XmTextVerifyPtr call_data) 
{
    /* record which window pane last had the keyboard focus */
    window->lastFocus = w;
    
    /* update line number statistic to reflect current focus */
    updateStatsLine(window, -1);
}

static void closeCB(Widget w, WindowInfo *window, caddr_t call_data) 
{
    int resp;
    
    if (WindowList->next == NULL && !WindowList->filenameSet) {
     	resp = DialogF(DF_QUES, window->shell, 2, "Exit NEdit?", "OK","Cancel");
    	if (resp == 1)
    	    exit(0);
    } else
    	CloseFileAndWindow(window);
}

/*
** turn on and off autoIndent by overriding the translation for carriage
** return
*/
static void changeAutoIndent(Widget text, int state)
{
    static XtTranslations onTable = NULL, offTable = NULL;
    static char *onTranslations =
	"Ctrl<Key>Return: newline()\n<Key>Return: newline-and-indent()\n";
    static char *offTranslations =
	"Ctrl<Key>Return: newline-and-indent()\n<Key>Return: newline()\n";

    if (state) {
    	if (onTable == NULL)
    	    onTable = XtParseTranslationTable(onTranslations);
    	XtOverrideTranslations(text, onTable);
    } else {
    	if (offTable == NULL)
    	    offTable = XtParseTranslationTable(offTranslations);
    	XtOverrideTranslations(text, offTable);
    }
}

/*
** Change the distance in characters between tab stops for a text widget
** Note: depends on private widget data and may break in future Motif releases
*/
static void changeTabDist(Widget text, int tabWidth)
{
    OutputData data = ((XmTextWidget)text)->text.output->data;
    
    data->tabwidth = tabWidth * data->averagecharwidth;
}

/*
** Under Motif 1.2, OSF moved the secondary actions to Alt+ middle mouse
** button.  This was not too bright, since most Motif window managers
** are already using this button assignment.  This routine adds these
** translations to the right mouse button using XtAgmentTranslations
** (so that user assignments will take precedence).  Note that this
** also means that if OSF ever adds right button actions to the widget,
** this method will cease to work.
*/
static void addRightBtnActions(Widget text)
{
#ifdef MOTIF12
    static XtTranslations table = NULL;
    static char *translations = "\
	Ctrl<Btn3Down>:	process-bdrag()\n\
	<Btn3Down>:	process-bdrag()\n\
	Ctrl<Btn3Motion>:secondary-adjust()\n\
	<Btn3Motion>:	secondary-adjust()\n\
	Ctrl<Btn3Up>:	move-to()\n\
	<Btn3Up>:	copy-to()\n";

    if (table == NULL)
    	table = XtParseTranslationTable(translations);
    XtAugmentTranslations(text, table);
#endif
}

/*
** Find the line number given the cursor position.  This can only be
** calculated easily when the insert position is within the displayed
** part of the text.  This routine will return NOLINE, if the line
** is not displayed.
**
** This depends heavily on private widget routines and data, so try not
** to use it anyplace critical since it may break in the future.
*/
static int textPosToLineNum(Widget text, int pos)
{
    int displayedLineNum;
    
    displayedLineNum = _XmTextPosToLine((XmTextWidget)text, pos);
    if (displayedLineNum == NOLINE)
    	return NOLINE;
    return ((XmTextWidget)text)->text.top_line + displayedLineNum + 1;
}

/*
** Change the text widget to recognize punctuation in determing word
** boundaries.  This is important for programmers to be able to select
** variable names by double clicking.
*/
static void changeWordDelimiters(Widget text)
{
    XmTextSource source = ((XmTextWidget)text)->text.source;
    
    /* replace the text source scan procedure with our own.  Note that this
       is dependent on private widget data and may well break in the future */
    if (source->Scan != (ScanProc)replacementScanProc) {
    	OriginalScanProc = source->Scan;
    	source->Scan = (ScanProc)replacementScanProc;
    }
}

/*
** All versions of Motif prior to Motif 1.2 have a bug that causes
** the scroll bars of scrolling text widgets to get out of sync with the
** displayed text.  The problem is caused by the CountLines procedure.
** Through extreme good fortune, CountLines is user-replaceable, and we
** do so here.  This routine should be periodically checked to make sure
** it is still necessary.  To test for the bug, open a large file, cut about
** 20 or so lines near the top of the file, then autoscroll downwards through
** the file.  If the scroll bar accelerates towards the bottom and reaches
** the bottom before the text is fully scrolled, the bug is still there.
*/
static void patchScrollBarBug(Widget text)
{
#ifndef MOTIF12
    XmTextSource source = ((XmTextWidget)text)->text.source;

    if (source->CountLines != replacementCountProc) {
    	OriginalCountProc = source->CountLines;
    	source->CountLines = replacementCountProc;
    }
#endif
}

/*
** This procedure augments the text scanning procedure used by the text
** widget to change the word selection criteria to account for punctuation.
** It depends on private widget data, so it could break in a future release
** of Motif.
*/
static XmTextPosition replacementScanProc(XmTextSource source,
	XmTextPosition pos, XmTextScanType sType, XmTextScanDirection dir,
	int count, Boolean include)
{
    XmTextPosition scanPos, newScanPos, i;
    char *text;
    
    /* Run the original scan procedure to get its result */
    scanPos = (*OriginalScanProc)(source, pos, sType, dir, count, include);

    /* Use the original result for everything other than words */
    if (sType != XmSELECT_WORD)
    	return scanPos;
    
    /* If the original result selects no characters, no
       further restriction is possible */
    if (scanPos == pos)
    	return scanPos;
    	
    /* Restrict the original scan procedure's results if punctuation is
       found within the range that it suggests */
    newScanPos = scanPos;
    if (dir == XmsdRight) {
    	text = GetTextRange((Widget)source->data->widgets[0], pos, scanPos);
    	for (i=0; i<scanPos-pos; i++) {
    	    if (isDelimeter(text[i])) {
    	        newScanPos = pos + i;
    	        break;
    	    }
    	}
    } else {
    	text = GetTextRange((Widget)source->data->widgets[0], scanPos, pos);
    	for (i=(pos-scanPos)-1; i>=0; i--) {
    	    if (isDelimeter(text[i])) {
    	        newScanPos = scanPos + i + 1;
    	        break;
    	    }
    	}
    }
    XtFree(text);
    return newScanPos;
}

/*
** This procedure replaces the CountLines procedure in the widget text source
** data structure.  The existing one through at least Motif 1.1.5 is flawed
** and counts lines wrong under some circumstances. (see patchScrollBarBug
** above).
*/
static int replacementCountProc(XmTextSource source, XmTextPosition start,
	int length)
{
    int nLines=0;
    char *text, *c;
    
    text = GetTextRange((Widget)source->data->widgets[0], start, start+length);
    for (c=text; *c!='\0'; c++)
    	if (*c=='\n') nLines++;
    XtFree(text);
    return nLines;
}

/*
** Determine if a character is a word delimeter.  This really should not be
** hard coded, since some users would like correct word selection for their
** particular computer language, or typing preference.
*/
static int isDelimeter(char c)
{
    int i;
    static char delimeters[] = {'.',  ',', '/', '`', '\'', '!', '@', '#',
	'%', '^', '&', '*', '(', ')', '-', '=', '+', '{', '}', '[', ']',
	'"', ':', ';', '<', '>', '?'};

    for (i=0; i<XtNumber(delimeters); i++) {
    	if (c == delimeters[i])
    	    return True;
    }
    return False;
}

/*
** Add a window to the the window list.
*/
static void addToWindowList(WindowInfo *window) 
{
    WindowInfo *temp;

    temp = WindowList;
    WindowList = window;
    window->next = temp;
}

/*
** Remove a window from the list of windows
*/
static void removeFromWindowList(WindowInfo *window)
{
    WindowInfo *temp;

    if (WindowList == window)
	WindowList = window->next;
    else {
	for (temp = WindowList; temp != NULL; temp = temp->next) {
	    if (temp->next == window) {
		temp->next = window->next;
		break;
	    }
	}
    }
}

/*
** Update the optional statistics line.  pos is the current insert position
** so that the routine can have the updated position when called from the
** motionVerify callback, from anywhere else, just pass -1.  
*/
static void updateStatsLine(WindowInfo *window, int pos)
{
    int line, length;
    char *string;
    XmString mString;
    
    if (!XtIsManaged(window->statsLine))
    	return;
    if (pos == -1)
    	pos = XmTextGetInsertionPosition(window->lastFocus);
    line = textPosToLineNum(window->lastFocus, pos);
    length = TextLength(window->textArea);
    
    string = XtMalloc(strlen(window->filename) + strlen(window->path) + 45);
    if (line == NOLINE)
    	sprintf(string, "%s%s %d bytes", window->path, window->filename,
    		length);
    else
    	sprintf(string, "%s%s line %d, col %d, %d bytes",
    		window->path, window->filename, line,
    		columnNumber(window->lastFocus, pos, window->tabDist), length);
    mString = XmStringCreateSimple(string);
    XtFree(string);
    XtVaSetValues(window->statsLine, XmNlabelString, mString, NULL);
    XmStringFree(mString);
}

/*
** Find the column number corresponding to a given character position
** in a text widget
*/
static int columnNumber(Widget textW, XmTextPosition pos, int tabDist)
{
    XmTextPosition start, p, lineStart, length, colStart, col;
    char *text, *c, *charStart;
    
    /* Get 80 characters of text before the cursor */
    start = pos-80 >= 0 ? pos-80 : 0;
    length = pos - start;
    text = GetTextRange(textW, start, pos);
    
    /* Scan backwards to find the beginning of the line */
    lineStart = start;
    charStart = text;
    for (c = &text[length-1], p=pos-1; p>=start; c--, p--) {
    	if (*c == '\n') {
    	    lineStart = p+1;
    	    charStart = ++c;
    	    break;
    	}
    }
    
    /* If the start of the line is beyond the 80 characters retrieved, just
       recursively call this function to find the column number of the starting
       position.  This means memory is allocated in 80 character chunks as
       needed for the backwards search for the beginning of the line */
    if (p < start && start != 0)
    	colStart = columnNumber(textW, lineStart, tabDist);
    else
    	colStart = 0;
    	
    /* Calculate the column number by scanning forward from the line break
       and counting characters, accounting for tabs */
    col = colStart;
    for (c=charStart; *c!='\0'; c++) {
    	if (*c == '\t')
    	    col += tabDist - (col % tabDist);
    	else
    	    col++;
    }
    XtFree(text);
    return col;
}

/*
** Paned windows are impossible to adjust after they are created, which makes
** them nearly useless for NEdit (or any application which needs to dynamically
** adjust the panes) unless you tweek some private data to overwrite the
** desired pane heights which were set at creation time.  This, as usual, may
** break in a future release of Motif because of dependence on private data.
*/
static void setPaneDesiredHeight(Widget w, int height)
{
    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.dheight = height;
}

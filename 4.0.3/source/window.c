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
static char SCCSID[] = "@(#)window.c	1.34     9/22/94";
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
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/RowColumnP.h>
/* #include <Xm/DrawingA.h> */
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#ifdef EDITRES
#include <X11/Xmu/Editres.h>
extern void _XEditResCheckMessages();
#endif /* EDITRES */
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "textSel.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "menu.h"
#include "file.h"
#include "search.h"
#include "undo.h"
#include "preferences.h"
#include "selection.h"
#include "nedit.bm"

/* Initial minimum height of a pane.  Just a fallback in case setPaneMinHeight
   (which may break in a future release) is not available */
#define PANE_MIN_HEIGHT 39

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols);
static void addToWindowList(WindowInfo *window);
static void removeFromWindowList(WindowInfo *window);
static void focusCB(Widget w, WindowInfo *window, XtPointer callData);
static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
	char *deletedText, void *cbArg);
static void movedCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragStartCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragEndCB(Widget w, WindowInfo *window, dragEndCBStruct *callData);
static void closeCB(Widget w, WindowInfo *window, XtPointer callData);
static void recreateWindow(WindowInfo *window, char *fontName, XmFontList fontList);
static void setPaneDesiredHeight(Widget w, int height);
static void setPaneMinHeight(Widget w, int min);
static void updateWMSizeHints(WindowInfo *window);
static void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id);
#ifdef ROWCOLPATCH
static void patchRowCol(Widget w);
static void patchedRemoveChild(Widget child);
#endif

/*
** Create a new editor window
*/
WindowInfo *CreateWindow(char *name, int rows, int cols, int autoIndent, 
	int autoSave, int saveOldVersion, char *fontName, XmFontList fontList,
	int wrap, int overstrike, int tabDist, int showMatching)
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
    strcpy(window->filename, name);
    window->undo = NULL;
    window->redo = NULL;
    window->nPanes = 0;
    window->autoSaveCharCount = 0;
    window->autoSaveOpCount = 0;
    window->undoOpCount = 0;
    window->undoMemUsed = 0;
    window->readOnly = FALSE;
    window->lockWrite = FALSE;
    window->autoIndent = autoIndent;
    window->autoSave = autoSave;
    window->saveOldVersion = saveOldVersion;
    window->wrap = wrap;
    window->overstrike = overstrike;
    window->showMatching = showMatching;
    window->showStats = GetPrefStatsLine();
    window->modeMessageDisplayed = FALSE;
    window->ignoreModify = FALSE;
    window->windowMenuValid = FALSE;
    window->prevOpenMenuValid = FALSE;
    window->flashTimeoutID = 0;
    window->wasSelected = FALSE;
    strcpy(window->fontName, fontName);
    window->fontList = fontList;
    window->shellCmdDialog = NULL;
    window->macroCmdDialog = NULL;
    window->nMarks = 0;
    window->markTimeoutID = 0;
    
    /* Create a new toplevel shell to hold the window */
    ac = 0;
    XtSetArg(al[ac], XmNtitle, name); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(al[ac], XmNiconName, name); ac++;
    appShell = XtAppCreateShell("nedit", "NEdit",
		applicationShellWidgetClass, TheDisplay, al, ac);
    window->shell = appShell;

#ifdef EDITRES
    XtAddEventHandler (appShell, (EventMask)0, True,
	    _XEditResCheckMessages, NULL);
#endif /* EDITRES */

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
    	    XmNiconMask, maskPixmap, 0);

    /* Create a MainWindow to manage the menubar and text area, set the
       userData resource to be used by WidgetToWindow to recover the
       window pointer from the widget id of any of the window's widgets */
    XtSetArg(al[ac], XmNuserData, window); ac++;
    main = XmCreateMainWindow(appShell, "main", al, ac);
    XtManageChild(main);
    
    /* Create file statistics display area.  Using a text widget rather than
       a label solves a layout problem with the main window, which messes up
       if the label is too long (we would need a resize callback to control
       the length when the window changed size).*/
    stats = XtVaCreateWidget("statsLine", xmTextWidgetClass,  main,
    	    XmNshadowThickness, 0,
    	    XmNmarginHeight, 0,
    	    XmNscrollHorizontal, False,
    	    XmNeditMode, XmSINGLE_LINE_EDIT,
    	    XmNeditable, False,
    	    XmNtraversalOn, False,
    	    XmNcursorPositionVisible, False,
    	    XmNfontList, window->fontList, 0);
    window->statsLine = stats;
    if (GetPrefStatsLine())
    	XtManageChild(stats);
    	
    /* If the fontList was NULL, use the magical default provided by Motif,
       since it must have worked if we've gotten this far */
    if (window->fontList == NULL)
    	XtVaGetValues(stats, XmNfontList, &window->fontList, 0);

    /* Create the menu bar */
    menuBar = CreateMenuBar(main, window);
    window->menuBar = menuBar;
    XtManageChild(menuBar);
        
    /* Create paned window to manage split window behavior */
    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass,  main,
    	    XmNmarginWidth, 0, XmNmarginHeight, 0, XmNseparatorOn, False,
    	    XmNspacing, 3, XmNsashIndent, -2, 0);
    window->splitPane = pane;
    XmMainWindowSetAreas(main, menuBar, stats, NULL, NULL, pane);

    /* Create the first, and most permanent text area (other panes may
       be added & removed, but this one will never be removed */
    text = createTextArea(pane, window, rows, cols);
    XtManageChild(text);
    window->textArea = text;
    window->lastFocus = text;

#ifdef RIGHT_POPUP
    /* Create the right button popup menu (note: order is important here,
       since the event handler for popping up this menu was probably already
       added in createTextArea, but CreatePopupMenu requires window->textArea
       to be set so it can attach the menu to it (because menu shells are
       finicky about the kinds of widgets they are attached to)) */
    window->popMenu = CreatePopupMenu(window);
#endif    
    
    /* Use this permanent text widget's buffer as the buffer to hold all text
       in the editor and the widget as the owner for buffer selections */
    window->buffer = TextGetBuffer(text);
    BufAddModifyCB(window->buffer, modifiedCB, window);
    HandleXSelections(text);
    
    /* Set the requested hardware tab distance and useTabs in the text buffer */
    BufSetTabDistance(window->buffer, tabDist);
    window->buffer->useTabs = GetPrefInsertTabs();

    /* add the window to the global window list, update the Windows menus */
    addToWindowList(window);
    InvalidateWindowMenus();
    
    /* realize all of the widgets in the new window */
    XtRealizeWidget(appShell);

    /* Make close command in window menu gracefully prompt for close */
    AddMotifCloseCallback(appShell, (XtCallbackProc)closeCB, window);
    
    /* Make window resizing work in nice character heights */
    updateWMSizeHints(window);
    
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
	window->lockWrite = FALSE;
	strcpy(window->filename, "Untitled");
	strcpy(window->path, "");
	window->ignoreModify = TRUE;
	BufSetAll(window->buffer, "");
	window->ignoreModify = FALSE;
	window->filenameSet = FALSE;
	window->fileChanged = FALSE;
	UpdateWindowTitle(window);
	UpdateWindowReadOnly(window);
	XtSetSensitive(window->closeItem, FALSE);
	XtSetSensitive(window->readOnlyItem, TRUE);
	XmToggleButtonSetState(window->readOnlyItem, FALSE, FALSE);
    	ClearUndoList(window);
    	ClearRedoList(window);
    	XmTextSetString(window->statsLine, ""); /* resets scroll pos of stats
    	    	    	    	    	           line from long file names */
    	UpdateStatsLine(window);
	return;
    }
    
    /* remove the buffer modification callback so the buffer will be
       deallocated when the last text widget is destroyed */
    BufRemoveModifyCB(window->buffer, modifiedCB, window);
    
#ifdef ROWCOLPATCH
    patchRowCol(window->menuBar);
#endif
    
    /* remove and deallocate all of the widgets associated with window */
    XtDestroyWidget(window->shell);
    
    /* remove the window from the global window list, update window menus */
    removeFromWindowList(window);
    InvalidateWindowMenus();
    
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
    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
    int horizOffsets[MAX_PANES+1];
    int i, focusPane, totalHeight=0;
    Widget text;
    
    /* Don't create new panes if we're already at the limit */
    if (window->nPanes >= MAX_PANES)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, keyboard focus */
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], 0);
    	totalHeight += paneHeights[i];
    	TextGetScroll(text, &topLines[i], &horizOffsets[i]);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Create a text widget to add to the pane and set its buffer to be
       the same as the other panes in the window */
    text = createTextArea(window->splitPane, window, 1, 1);
    TextSetBuffer(text, window->buffer);
    XtManageChild(text);
    window->textPanes[window->nPanes++] = text;
    
    /* adjust the heights, scroll positions, etc., to split the focus pane */
    for (i=window->nPanes; i>focusPane; i--) {
    	insertPositions[i] = insertPositions[i-1];
    	paneHeights[i] = paneHeights[i-1];
    	topLines[i] = topLines[i-1];
    	horizOffsets[i] = horizOffsets[i-1];
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
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], horizOffsets[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
    
    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
    	    wmSizeUpdateProc, window);
}

/*
** Close the window pane that last had the keyboard focus.  (Actually, close
** the bottom pane and make it look like pane which had focus was closed)
*/
void ClosePane(WindowInfo *window)
{
    short paneHeights[MAX_PANES+1];
    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
    int horizOffsets[MAX_PANES+1];
    int i, focusPane,totalHeight=0;
    Widget text;
    
    /* Don't delete the last pane */
    if (window->nPanes <= 0)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, and the keyboard focus */
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], 0);
    	totalHeight += paneHeights[i];
    	TextGetScroll(text, &topLines[i], &horizOffsets[i]);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Destroy last pane, and make sure lastFocus points to an existing pane */
    XtDestroyWidget(XtParent(window->textPanes[--window->nPanes]));
    if (window->nPanes == 0)
	window->lastFocus = window->textArea;
    else if (focusPane > window->nPanes)
	window->lastFocus = window->textPanes[window->nPanes-1];
    
    /* adjust the heights, scroll positions, etc., to make it look
       like the pane with the input focus was closed */
    for (i=window->nPanes; i>=focusPane; i--) {
    	insertPositions[i] = insertPositions[i+1];
    	paneHeights[i] = paneHeights[i+1];
    	topLines[i] = topLines[i+1];
    	horizOffsets[i] = horizOffsets[i+1];
    }
    
    /* set the desired heights and re-manage the paned window so it will
       recalculate pane heights */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	setPaneDesiredHeight(XtParent(text), paneHeights[i]);
    }
    XtManageChild(window->splitPane);
    
    /* Reset all of the scroll positions, insert positions, etc. */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], horizOffsets[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);

    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
    	    wmSizeUpdateProc, window);
}

/*
** Turn on and off the display of the statistics line
*/
void ShowStatsLine(WindowInfo *window, int state)
{
    Widget mainW = XtParent(window->statsLine);
    
    /* The very silly use of XmNcommandWindowLocation and XmNshowSeparator
       below are to kick the main window widget to position and remove the
       status line when it is managed and unmanaged */
    if (state) {
    	XtVaSetValues(mainW, XmNcommandWindowLocation,
    		XmCOMMAND_ABOVE_WORKSPACE, 0);
    	XtVaSetValues(mainW, XmNshowSeparator, True, 0);
    	XtManageChild(window->statsLine);
    	XtVaSetValues(mainW, XmNshowSeparator, False, 0);
   	UpdateStatsLine(window);
    } else {
     	XtUnmanageChild(window->statsLine);
   	XtVaSetValues(mainW, XmNcommandWindowLocation,
   		XmCOMMAND_BELOW_WORKSPACE, 0);
    }
    
    /* Tell WM that the non-expandable part of the window has changed size */
    updateWMSizeHints(window);
}

/*
** Display a special message in the stats line (show the stats line if it
** is not currently shown).
*/
void SetModeMessage(WindowInfo *window, char *message)
{
    window->modeMessageDisplayed = True;
    XmTextSetString(window->statsLine, message);
    ShowStatsLine(window, True);
}

/*
** Clear special statistics line message set in SetModeMessage, returns
** the statistics line to its original state as set in window->showStats
*/
void ClearModeMessage(WindowInfo *window)
{
    window->modeMessageDisplayed = False;
    ShowStatsLine(window, window->showStats);
    UpdateStatsLine(window);
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
    XtVaSetValues(window->textArea, textNautoIndent, state, 0);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNautoIndent, state, 0);
}

/*
** Set the font for a window from a font name (generates the fontList).
**
** Note that this leaks memory and server resources.  In previous NEdit
** versions, fontLists were carefully tracked and freed, but X and Motif
** have some kind of timing problem when widgets are distroyed, such that
** fonts may not be freed immediately after widget destruction with 100%
** safety.  Rather than kludge around this with timerProcs, I have chosen
** to create new fontLists only when the user explicitly changes the font
** (which shouldn't happen much in normal NEdit operation), and skip the
** futile effort of freeing them.
*/
void SetFontByName(WindowInfo *window, char *fontName)
{
    XFontStruct *font;
    
    font = XLoadQueryFont(TheDisplay, fontName);
    recreateWindow(window, fontName, font==NULL ? NULL :
    	    XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET));
}

/*
** Set insert/overstrike mode
*/
void SetOverstrike(WindowInfo *window, int overstrike)
{
    int i;

    XtVaSetValues(window->textArea, textNoverstrike, overstrike, 0);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNoverstrike, overstrike, 0);
    window->overstrike = overstrike;
}

/*
** Turn on/off auto-wrap mode
*/
void SetAutoWrap(WindowInfo *window, int state)
{
    int i;

    XtVaSetValues(window->textArea, textNautoWrap, state, 0);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNautoWrap, state, 0);
    window->wrap = state;
}

/*
** Set the wrap margin (0 == wrap at right edge of window)
*/
void SetWrapMargin(WindowInfo *window, int margin)
{
    int i;
    
    XtVaSetValues(window->textArea, textNwrapMargin, margin, 0);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNwrapMargin, margin, 0);
}

/*
** Recover the window pointer from any widget in the window, by searching
** up the widget hierarcy for the top level container widget where the window
** pointer is stored in the userData field.
*/
WindowInfo *WidgetToWindow(Widget w)
{
    WindowInfo *window;
    Widget parent;
    
    while (True) {
    	parent = XtParent(w);
    	if (parent == NULL)
    	    return NULL;
    	if (XtClass(parent) == applicationShellWidgetClass)
    	    break;
    	w = parent;
    }
    XtVaGetValues(w, XmNuserData, &window, 0);
    return window;
}

/*
** To reset font, it is necessary to close and re-create the whole window.
** (the paned window won't allow width resize required for font change).
*/
static void recreateWindow(WindowInfo *window, char *fontName,
	XmFontList fontList)
{
    WindowInfo *newWindow;
    int i, rows, cols, topLine, horizOffset;
    int emTabDist, wrapMargin;
    char *s1;
    
    /* create a new window and transfer values from the old one */
    XtVaGetValues(window->textArea, textNrows, &rows, textNcolumns, &cols,
    	    textNemulateTabs, &emTabDist, textNwrapMargin, &wrapMargin, 0);
    newWindow = CreateWindow("", rows, cols, window->autoIndent,
    	    window->autoSave, window->saveOldVersion, fontName, fontList,
    	    window->wrap, window->overstrike, window->buffer->tabDist,
    	    window->showMatching);
    XtVaSetValues(newWindow->textArea, textNemulateTabs, emTabDist,
    	    textNwrapMargin, wrapMargin, 0);
    XtSetSensitive(newWindow->undoItem, XtIsSensitive(window->undoItem));
    newWindow->ignoreModify = TRUE;
    BufSetAll(newWindow->buffer, s1=BufGetAll(window->buffer));
    XtFree(s1);
    newWindow->buffer->useTabs = window->buffer->useTabs;
    newWindow->ignoreModify = FALSE;
    TextSetCursorPos(newWindow->textArea, TextGetCursorPos(window->textArea));
    TextGetScroll(window->textArea, &topLine, &horizOffset);
    TextSetScroll(newWindow->textArea, topLine, horizOffset);
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
    newWindow->lockWrite = window->lockWrite;
    for (i=0; i<window->nMarks; i++)
    	newWindow->markTable[i] = window->markTable[i];
    newWindow->nMarks = window->nMarks;
    UpdateWindowTitle(newWindow);
    UpdateWindowReadOnly(newWindow);
    
    /* close the old window */
#ifdef ROWCOLPATCH
    patchRowCol(window->menuBar);
#endif
    BufRemoveModifyCB(window->buffer, modifiedCB, window);
    XtDestroyWidget(window->shell);
    removeFromWindowList(window);
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
    char title[MAXPATHLEN + 14]; 	/* 11: strlen("Replace (in )") */

    /* Set the window title, adding annotations for "modified" or "read-only" */
    strcpy(title, window->filename);
    if (window->readOnly)
    	strcat(title, " (read only)");
    else if (window->lockWrite)
    	strcat(title, " (locked)");
    else if (window->fileChanged)
    	strcat(title, " (modified)");
    SetWindowTitle(window, title);
    
    /* If there's a find or replace dialog up in "Keep Up" mode, with a
       file name in the title, update it too */
    if (window->findDlog && XmToggleButtonGetState(window->findKeepBtn)) {
    	sprintf(title, "Find (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->findDlog), XmNtitle, title, 0);
    }
    if (window->replaceDlog && XmToggleButtonGetState(window->replaceKeepBtn)) {
    	sprintf(title, "Replace (in %s)", window->filename);
    	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, 0);
    }
}

/*
** Update the read-only state of the text area(s) in the window, and
** the ReadOnly toggle button in the File menu to agree the readOnly
** and lockWrite state in the window data structure.
*/
void UpdateWindowReadOnly(WindowInfo *window)
{
    int i, state;
    
    state = window->readOnly || window->lockWrite;
    XtVaSetValues(window->textArea, textNreadOnly, state, 0);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNreadOnly, state, 0);
    XmToggleButtonSetState(window->readOnlyItem, state, FALSE);
    XtSetSensitive(window->readOnlyItem, !window->readOnly);
}

void SetWindowTitle(WindowInfo *window, char *title)
{
    /* Set both the window title and the icon title */
    XtVaSetValues(window->shell, XmNtitle, title, XmNiconName, title, 0);

    /* Update the Windows menus with the new name */
    InvalidateWindowMenus();
}

/*
** Get the start and end of the current selection.  This routine is obsolete
** because it ignores rectangular selections, and reads from the widget
** instead of the buffer.  Use BufGetSelectionPos.
*/
int GetSelection(Widget widget, int *left, int *right)
{
    return GetSimpleSelection(TextGetBuffer(widget), left, right);
}

/*
** Find the start and end of a single line selection.  Hides rectangular
** selection issues for older routines which use selections that won't
** span lines.
*/
int GetSimpleSelection(textBuffer *buf, int *left, int *right)
{
    int selStart, selEnd, isRect, rectStart, rectEnd, lineStart;

    /* get the character to match and its position from the selection, or
       the character before the insert point if nothing is selected.
       Give up if too many characters are selected */
    if (!BufGetSelectionPos(buf, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
        return False;
    if (isRect) {
    	lineStart = BufStartOfLine(buf, selStart);
    	selStart = BufCountForwardDispChars(buf, lineStart, rectStart);
    	selEnd = BufCountForwardDispChars(buf, lineStart, rectEnd);
    }
    *left = selStart;
    *right = selEnd;
    return True;
}

/*
** Returns a range of text from a text widget (this routine is obsolete,
** get text from the buffer instead).  Memory is allocated with
** XtMalloc and caller should free it.
*/
char *GetTextRange(Widget widget, int left, int right)
{
    return BufGetRange(TextGetBuffer(widget), left, right);
}

/*
** If the selection (or cursor position if there's no selection) is not
** fully shown, scroll to bring it in to view.  Note that as written,
** this won't work well with multi-line selections.  Modest re-write
** of the horizontal scrolling part would be quite easy to make it work
** well with rectangular selections.
*/
void MakeSelectionVisible(WindowInfo *window, Widget textPane)
{
    int left, right, isRect, rectStart, rectEnd, horizOffset;
    int scrollOffset, leftX, rightX, y, rows, margin;
    int topLineNum, lastLineNum, rightLineNum, leftLineNum, linesToScroll;
    textBuffer *buf = window->buffer;
    int topChar = TextFirstVisiblePos(textPane);
    int lastChar = TextLastVisiblePos(textPane);
    Dimension width;
    
    /* find out where the selection is */
    if (!BufGetSelectionPos(window->buffer, &left, &right, &isRect,
    	    &rectStart, &rectEnd)) {
    	left = right = TextGetCursorPos(textPane);
    	isRect = False;
    }
    	
    /* Check vertical positioning unless the selection is already shown or
       already covers the display.  If the end of the selection is below
       bottom, scroll it in to view until the end selection is scrollOffset
       lines from the bottom of the display or the start of the selection
       scrollOffset lines from the top.  Calculate a pleasing distance from the
       top or bottom of the window, to scroll the selection to (if scrolling is
       necessary), around 1/3 of the height of the window */
    if (!((left >= topChar && right <= lastChar) ||
    	    (left < topChar && right > lastChar))) {
	XtVaGetValues(textPane, textNrows, &rows, 0);
	scrollOffset = rows/3;
	TextGetScroll(textPane, &topLineNum, &horizOffset);
	lastLineNum = topLineNum + rows;
	if (right > lastChar) {
            if (left <= topChar)
        	return;
            leftLineNum = topLineNum + BufCountLines(buf, topChar, left);
            if (leftLineNum < topLineNum + scrollOffset)
        	return;
            linesToScroll = BufCountLines(buf, lastChar, right) + scrollOffset;
            if (leftLineNum - linesToScroll < topLineNum + scrollOffset)
        	linesToScroll = leftLineNum - (topLineNum + scrollOffset);
    	    TextSetScroll(textPane, topLineNum+linesToScroll, horizOffset);
	} else if (left < topChar) {
            if (right >= lastChar)
        	return;
            rightLineNum = lastLineNum - BufCountLines(buf, right, lastChar);
            if (rightLineNum > lastLineNum - scrollOffset)
        	return;
            linesToScroll = BufCountLines(buf, left, topChar) + scrollOffset;
            if (rightLineNum + linesToScroll > lastLineNum - scrollOffset)
        	linesToScroll = (lastLineNum - scrollOffset) - rightLineNum;
    	    TextSetScroll(textPane, topLineNum-linesToScroll, horizOffset);
	}
    }
    
    /* If either end of the selection off screen horizontally, try to bring it
       in view, by making sure both end-points are visible.  Using only end
       points of a multi-line selection is not a great idea, and disaster for
       rectangular selections, so this part of the routine should be re-written
       if it is to be used much with either.  Note also that this is a second
       scrolling operation, causing the display to jump twice.  It's done after
       vertical scrolling to take advantage of TextPosToXY which requires it's
       reqested position to be vertically on screen) */
    if (    TextPosToXY(textPane, left, &leftX, &y) &&
    	    TextPosToXY(textPane, right, &rightX, &y) && leftX <= rightX) {
    	TextGetScroll(textPane, &topLineNum, &horizOffset);
    	XtVaGetValues(textPane, XmNwidth, &width, textNmarginWidth, &margin, 0);
    	if (leftX < margin)
    	    horizOffset -= margin - leftX;
    	else if (rightX > width - margin)
    	    horizOffset += rightX - (width - margin);
    	TextSetScroll(textPane, topLineNum, horizOffset);
    }
     
    /* make sure that the statistics line is up to date */
    UpdateStatsLine(window);
}

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols)
{
    Widget text, sw, hScrollBar, vScrollBar;
    Pixel troughColor;
    XFontStruct *fs;
    int marginHeight;
    Dimension hsbHeight, swMarginHeight;
        
    /* Create a text widget inside of a scrolled window widget */
    sw = XtVaCreateManagedWidget("scrolledW", xmScrolledWindowWidgetClass,
    	    parent, XmNspacing, 0, XmNpaneMaximum, SHRT_MAX,
    	    XmNpaneMinimum, PANE_MIN_HEIGHT, XmNhighlightThickness, 0, 0); 
    hScrollBar = XtVaCreateManagedWidget("textHorScrollBar",
    	    xmScrollBarWidgetClass, sw, XmNorientation, XmHORIZONTAL, 
    	    XmNrepeatDelay, 10, 0);
    vScrollBar = XtVaCreateManagedWidget("textVertScrollBar",
    	    xmScrollBarWidgetClass, sw, XmNorientation, XmVERTICAL,
    	    XmNrepeatDelay, 10, 0);
    text = XtVaCreateManagedWidget("text", textWidgetClass, sw,
    	    textNrows, rows, textNcolumns, cols,
    	    textNemulateTabs, GetPrefEmTabDist(),
    	    textNfont, GetDefaultFontStruct(window->fontList),
    	    textNhScrollBar, hScrollBar, textNvScrollBar, vScrollBar,
    	    textNwordDelimiters, GetPrefDelimiters(),
    	    textNwrapMargin, GetPrefWrapMargin(),
    	    textNautoIndent, window->autoIndent, textNautoWrap, window->wrap,
    	    textNoverstrike, window->overstrike, 0);
    XtVaSetValues(sw, XmNworkWindow, text, XmNhorizontalScrollBar, hScrollBar,
    	    XmNverticalScrollBar, vScrollBar, 0);
    
    /* add focus, drag, and cursor tracking callback */
    XtAddCallback(text, textNfocusCallback, (XtCallbackProc)focusCB, window);
    XtAddCallback(text, textNcursorMovementCallback, (XtCallbackProc)movedCB,
    	    window);
    XtAddCallback(text, textNdragStartCallback, (XtCallbackProc)dragStartCB,
    	    window);
    XtAddCallback(text, textNdragEndCallback, (XtCallbackProc)dragEndCB,
    	    window);
    
    /* This makes sure the text area initially has a the insert point shown
       ... (check if still true with the nedit text widget, probably not) */
    XmAddTabGroup(XtParent(text));

    /* Set the little square in the corner between the scroll
       bars to be the same color as the scroll bar interiors  */
    XtVaGetValues(vScrollBar, XmNtroughColor, &troughColor, 0);
    XtVaSetValues(sw, XmNbackground, troughColor, 0);

    /* set the minimum size of a pane */
    XtVaGetValues(sw, XmNscrolledWindowMarginHeight, &swMarginHeight, 0);
    XtVaGetValues(text, textNmarginHeight, &marginHeight, textNfont, &fs, 0);
    XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, 0);
    setPaneMinHeight(sw, fs->ascent + fs->descent + marginHeight*2 +
    	    swMarginHeight*2 + hsbHeight);
    
    /* compensate for Motif delete/backspace problem */
    RemapDeleteKey(text);

#ifdef RIGHT_POPUP
    /* Attach event handler for right button popup menu */
    AddPopupHandler(window, text);
#endif    
    
    return text;
}

static void movedCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    if (window->ignoreModify)
    	return;

    /* update line and column nubers in statistics line */
    UpdateStatsLine(window);
    
    /* Check the character before the cursor for matchable characters */
    FlashMatching(window, w);
}

static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
	char *deletedText, void *cbArg) 
{
    WindowInfo *window = (WindowInfo *)cbArg;
    int selected = window->buffer->primary.selected;
    
    /* update the table of bookmarks */
    UpdateMarkTable(window, pos, nInserted, nDeleted);
    
    /* Check and dim/undim selection related menu items */
    if (window->wasSelected && !selected || !window->wasSelected && selected) {
    	window->wasSelected = selected;
    	XtSetSensitive(window->printSelItem, selected);
    	XtSetSensitive(window->cutItem, selected);
    	XtSetSensitive(window->copyItem, selected);
#ifndef VMS
    	XtSetSensitive(window->filterItem, selected);
#endif
#ifdef RIGHT_POPUP
    	XtSetSensitive(window->popCutItem, selected);
    	XtSetSensitive(window->popCopyItem, selected);
#endif
    	if (window->replaceDlog != NULL)
    	    XtSetSensitive(window->replaceInSelBtn, selected);
    }
    
    /* When the program needs to make a change to a text area without without
       recording it for undo or marking file as changed it sets ignoreModify */
    if (window->ignoreModify || (nDeleted == 0 && nInserted == 0))
    	return;
    
    /* Save information for undoing this operation (this call also counts
       characters and editing operations for triggering autosave */
    SaveUndoInformation(window, pos, nInserted, nDeleted, deletedText);
    
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
    UpdateStatsLine(window);
}

static void focusCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    /* record which window pane last had the keyboard focus */
    window->lastFocus = w;
    
    /* update line number statistic to reflect current focus pane */
    UpdateStatsLine(window);
}

static void dragStartCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    /* don't record all of the intermediate drag steps for undo */
    window->ignoreModify = True;
}

static void dragEndCB(Widget w, WindowInfo *window, dragEndCBStruct *callData) 
{
    /* restore recording of undo information */
    window->ignoreModify = False;
    
    /* Do nothing if drag operation was canceled */
    if (callData->nCharsInserted == 0)
    	return;
    	
    /* Save information for undoing this operation not saved while
       undo recording was off */
    modifiedCB(callData->startPos, callData->nCharsInserted,
    	    callData->nCharsDeleted, 0, callData->deletedText, window);
}

static void closeCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    if (WindowList->next == NULL) {
    	if (!WindowList->fileChanged)
     	    exit(0);
     	if (CloseFileAndWindow(window))
     	    exit(0);
    } else
    	CloseFileAndWindow(window);
    	
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
** Update the optional statistics line.  
*/
void UpdateStatsLine(WindowInfo *window)
{
    int line, length, pos, colNum, topLineNum, horizOffset;
    char *string;
    Widget statW = window->statsLine;
    
    /* This routine is called for each character typed, so its performance
       affects overall editor perfomance.  Only update if the line is on
       and not displaying a special mode message */ 
    if (!window->showStats || window->modeMessageDisplayed)
    	return;
    
    /* Compose the string to display. If line # isn't available, leave it off */
    pos = TextGetCursorPos(window->lastFocus);
    length = window->buffer->length;
    string = XtMalloc(strlen(window->filename) + strlen(window->path) + 45);
    if (!TextPosToLineNum(window->lastFocus, pos, &line))
    	sprintf(string, "%s%s %d bytes", window->path, window->filename,length);
    else {
    	TextGetScroll(window->lastFocus, &topLineNum, &horizOffset);
    	colNum = BufCountDispChars(window->buffer,
    		BufStartOfLine(window->buffer, pos), pos);
    	sprintf(string, "%s%s line %d, col %d, %d bytes",
    		window->path, window->filename, line, colNum, length);
    }
    
    /* Change the text in the stats line */
    XmTextReplace(statW, 0, XmTextGetLastPosition(statW), string);
    XtFree(string);
}

/*
** Paned windows are impossible to adjust after they are created, which makes
** them nearly useless for NEdit (or any application which needs to dynamically
** adjust the panes) unless you tweek some private data to overwrite the
** desired and minimum pane heights which were set at creation time.  These
** will probably break in a future release of Motif because of dependence on
** private data.
*/
static void setPaneDesiredHeight(Widget w, int height)
{
    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.dheight = height;
}
static void setPaneMinHeight(Widget w, int min)
{
    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.min = min;
}

/*
** Update the window manager's size hints.  These tell it the increments in
** which it is allowed to resize the window.  While this isn't particularly
** important for NEdit (since it can tolerate any window size), setting these
** hints also makes the resize indicator show the window size in characters
** rather than pixels, which is very helpful to users.
*/
static void updateWMSizeHints(WindowInfo *window)
{
    Dimension shellWidth, shellHeight, textHeight, textWidth;
    int marginHeight, marginWidth, totalHeight;
    XFontStruct *fs;
    int i, baseWidth, baseHeight, fontHeight, fontWidth;

    /* Find the base (non-expandable) width and height of the editor window */
    XtVaGetValues(window->textArea, XmNheight, &textHeight, XmNwidth,
    	    &textWidth, textNmarginHeight, &marginHeight, textNmarginWidth,
    	    &marginWidth, 0);
    totalHeight = textHeight - 2*marginHeight;
    for (i=0; i<window->nPanes; i++) {
    	XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, 0);
    	totalHeight += textHeight - 2*marginHeight;
    }
    XtVaGetValues(window->shell, XmNwidth, &shellWidth,
    	    XmNheight, &shellHeight, 0);
    baseWidth = shellWidth - (textWidth - 2*marginWidth);
    baseHeight = shellHeight - totalHeight;
    
    /* Find the dimensions of a single character of the text font */
    XtVaGetValues(window->textArea, textNfont, &fs, 0);
    fontHeight = fs->ascent + fs->descent;
    fontWidth = fs->max_bounds.width;
    
    /* Set the size hints in the shell widget */
    XtVaSetValues(window->shell, XmNwidthInc, fs->max_bounds.width,
    	    XmNheightInc, fontHeight,
    	    XmNbaseWidth, baseWidth, XmNbaseHeight, baseHeight,
    	    XmNminWidth, baseWidth + fontWidth,
    	    XmNminHeight, baseHeight + (1+window->nPanes) * fontHeight, 0);
}

/*
** Xt timer procedure for updating size hints.  The new sizes of objects in
** the window are not ready immediately after adding or removing panes.  This
** is a timer routine to be invoked with a timeout of 0 to give the event
** loop a chance to finish processing the size changes before reading them
** out for setting the window manager size hints.
*/
static void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id)
{
    updateWMSizeHints((WindowInfo *)clientData);
}

#ifdef ROWCOLPATCH
/*
** There is a bad memory reference in the delete_child method of the
** RowColumn widget in some Motif versions (so far, just Solaris with Motif
** 1.2.3) which appears durring the phase 2 destroy of the widget. This
** patch replaces the method with a call to the Composite widget's
** delete_child method.  The composite delete_child method handles part,
** but not all of what would have been done by the original method, meaning
** that this is dangerous and should be used sparingly.  Note that
** patchRowCol is called only in CloseWindow, before the widget is about to
** be destroyed, and only on systems where the bug has been observed
*/
static void patchRowCol(Widget w)
{
    ((XmRowColumnClassRec *)XtClass(w))->composite_class.delete_child =
    	    patchedRemoveChild;
}
static void patchedRemoveChild(Widget child)
{
    /* Call composite class method instead of broken row col delete_child
       method */
    (*((CompositeWidgetClass)compositeWidgetClass)->composite_class.
                delete_child) (child);
}
#endif /* ROWCOLPATCH */

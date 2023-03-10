static const char CVSID[] = "$Id: window.c,v 1.84.2.1 2003/08/01 01:26:42 n8gray Exp $";
/*******************************************************************************
*                                                                              *
* window.c -- Nirvana Editor window creation/deletion                          *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version.                                                                     *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA                                 *
*                                                                              *
* Nirvana Text Editor                                                          *
* May 10, 1991                                                                 *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "window.h"
#include "textBuf.h"
#include "textSel.h"
#include "text.h"
#include "textDisp.h"
#include "textP.h"
#include "nedit.h"
#include "menu.h"
#include "file.h"
#include "search.h"
#include "undo.h"
#include "preferences.h"
#include "selection.h"
#include "server.h"
#include "shell.h"
#include "macro.h"
#include "highlight.h"
#include "smartIndent.h"
#include "userCmds.h"
#include "nedit.bm"
#include "n.bm"
#include "windowTitle.h"
#include "../util/clearcase.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifndef __MVS__
#include <sys/param.h>
#endif
#include "../util/clearcase.h"
#endif /*VMS*/
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#ifdef __unix__
#include <sys/time.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/RowColumnP.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Protocols.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/PrimitiveP.h>
#include <Xm/Frame.h>
#ifdef EDITRES
#include <X11/Xmu/Editres.h>
/* extern void _XEditResCheckMessages(); */
#endif /* EDITRES */


#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif


/* Initial minimum height of a pane.  Just a fallback in case setPaneMinHeight
   (which may break in a future release) is not available */
#define PANE_MIN_HEIGHT 39

/* Thickness of 3D border around statistics and/or incremental search areas
   below the main menu bar */
#define STAT_SHADOW_THICKNESS 1

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
        int cols, int emTabDist, char *delimiters, int wrapMargin,
        int lineNumCols);
static void showStats(WindowInfo *window, int state);
static void showISearch(WindowInfo *window, int state);
static void showStatsForm(WindowInfo *window, int state);
static void addToWindowList(WindowInfo *window);
static void removeFromWindowList(WindowInfo *window);
static void focusCB(Widget w, WindowInfo *window, XtPointer callData);
static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
        char *deletedText, void *cbArg);
static void movedCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragStartCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragEndCB(Widget w, WindowInfo *window, dragEndCBStruct *callData);
static void closeCB(Widget w, WindowInfo *window, XtPointer callData);
static void saveYourselfCB(Widget w, WindowInfo *window, XtPointer callData);
static int isIconic(WindowInfo *window);
static void setPaneDesiredHeight(Widget w, int height);
static void setPaneMinHeight(Widget w, int min);
static void addWindowIcon(Widget shell);
static void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id);
static void getGeometryString(WindowInfo *window, char *geomString);
#ifdef ROWCOLPATCH
static void patchRowCol(Widget w);
static void patchedRemoveChild(Widget child);
#endif
static unsigned char* sanitizeVirtualKeyBindings();
static int sortAlphabetical(const void* k1, const void* k2);
static int virtKeyBindingsAreInvalid(const unsigned char* bindings);
static void restoreInsaneVirtualKeyBindings(unsigned char* bindings);
static Widget containingPane(Widget w);

/*
** Create a new editor window
*/
WindowInfo *CreateWindow(const char *name, char *geometry, int iconic)
{
    Widget appShell, mainWin, menuBar, pane, text, stats, statsAreaForm;
    Widget iSearchLabel;
    WindowInfo *window;
    Pixel bgpix, fgpix;
    Arg al[20];
    int ac;
    XmString s1;
#ifdef SGI_CUSTOM
    char sgi_title[MAXPATHLEN + 14 + SGI_WINDOW_TITLE_LEN] = SGI_WINDOW_TITLE; 
#endif
    char newGeometry[MAX_GEOM_STRING_LEN];
    unsigned int rows, cols;
    int x, y, bitmask;
    static Atom wmpAtom, syAtom = 0;
    static int firstTime = True;
    unsigned char* invalidBindings = NULL;

    if (firstTime) 
    {
        invalidBindings = sanitizeVirtualKeyBindings();
        firstTime = False;
    }
    
    /* Allocate some memory for the new window data structure */
    window = (WindowInfo *)XtMalloc(sizeof(WindowInfo));
    
    /* initialize window structure */
    /* + Schwarzenberg: should a 
      memset(window, 0, sizeof(WindowInfo));
         be added here ?
    */
    window->replaceDlog = NULL;
    window->replaceText = NULL;
    window->replaceWithText = NULL;
    window->replaceWordToggle = NULL;
    window->replaceCaseToggle = NULL;
    window->replaceRegexToggle = NULL;
    window->findDlog = NULL;
    window->findText = NULL;
    window->findWordToggle = NULL;
    window->findCaseToggle = NULL;
    window->findRegexToggle = NULL;
    window->replaceMultiFileDlog = NULL;
    window->replaceMultiFilePathBtn = NULL;
    window->replaceMultiFileList = NULL;
    window->multiFileReplSelected = FALSE;
    window->multiFileBusy = FALSE;
    window->writableWindows = NULL;
    window->nWritableWindows = 0;
    window->fileChanged = FALSE;
    window->fileMode = 0;
    window->filenameSet = FALSE;
    window->fileFormat = UNIX_FILE_FORMAT;
    window->lastModTime = 0;
    window->fileMissing = True;
    strcpy(window->filename, name);
    window->undo = NULL;
    window->redo = NULL;
    window->nPanes = 0;
    window->autoSaveCharCount = 0;
    window->autoSaveOpCount = 0;
    window->undoOpCount = 0;
    window->undoMemUsed = 0;
    CLEAR_ALL_LOCKS(window->lockReasons);
    window->indentStyle = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
    window->autoSave = GetPrefAutoSave();
    window->saveOldVersion = GetPrefSaveOldVersion();
    window->wrapMode = GetPrefWrap(PLAIN_LANGUAGE_MODE);
    window->overstrike = False;
    window->showMatchingStyle = GetPrefShowMatching();
    window->matchSyntaxBased = GetPrefMatchSyntaxBased();
    window->showStats = GetPrefStatsLine();
    window->showISearchLine = GetPrefISearchLine();
    window->showLineNumbers = GetPrefLineNums();
    window->highlightSyntax = GetPrefHighlightSyntax();
    window->backlightCharTypes = NULL;
    window->backlightChars = GetPrefBacklightChars();
    if (window->backlightChars) {
        char *cTypes = GetPrefBacklightCharTypes();
        if (cTypes && window->backlightChars) {
            if ((window->backlightCharTypes = XtMalloc(strlen(cTypes) + 1)))
                strcpy(window->backlightCharTypes, cTypes);
        }
    }
    window->modeMessageDisplayed = FALSE;
    window->ignoreModify = FALSE;
    window->windowMenuValid = FALSE;
    window->prevOpenMenuValid = FALSE;
    window->flashTimeoutID = 0;
    window->fileClosedAtom = None;
    window->wasSelected = FALSE;

    strcpy(window->fontName, GetPrefFontName());
    strcpy(window->italicFontName, GetPrefItalicFontName());
    strcpy(window->boldFontName, GetPrefBoldFontName());
    strcpy(window->boldItalicFontName, GetPrefBoldItalicFontName());
    window->colorDialog = NULL;
    window->fontList = GetPrefFontList();
    window->italicFontStruct = GetPrefItalicFont();
    window->boldFontStruct = GetPrefBoldFont();
    window->boldItalicFontStruct = GetPrefBoldItalicFont();
    window->fontDialog = NULL;
    window->nMarks = 0;
    window->markTimeoutID = 0;
    window->highlightData = NULL;
    window->shellCmdData = NULL;
    window->macroCmdData = NULL;
    window->smartIndentData = NULL;
    window->languageMode = PLAIN_LANGUAGE_MODE;
    window->iSearchHistIndex = 0;
    window->iSearchStartPos = -1;
    window->replaceLastRegexCase   = TRUE;
    window->replaceLastLiteralCase = FALSE;
    window->iSearchLastRegexCase   = TRUE;
    window->iSearchLastLiteralCase = FALSE;
    window->findLastRegexCase      = TRUE;
    window->findLastLiteralCase    = FALSE;
    
    /* If window geometry was specified, split it apart into a window position
       component and a window size component.  Create a new geometry string
       containing the position component only.  Rows and cols are stripped off
       because we can't easily calculate the size in pixels from them until the
       whole window is put together.  Note that the preference resource is only
       for clueless users who decide to specify the standard X geometry
       application resource, which is pretty useless because width and height
       are the same as the rows and cols preferences, and specifying a window
       location will force all the windows to pile on top of one another */
    if (geometry == NULL || geometry[0] == '\0')
        geometry = GetPrefGeometry();
    if (geometry == NULL || geometry[0] == '\0') {
        rows = GetPrefRows();
        cols = GetPrefCols();
        newGeometry[0] = '\0';
    } else {
        bitmask = XParseGeometry(geometry, &x, &y, &cols, &rows);
        if (bitmask == 0)
            fprintf(stderr, "Bad window geometry specified: %s\n", geometry);
        else {
            if (!(bitmask & WidthValue))
                cols = GetPrefCols();
            if (!(bitmask & HeightValue))
                rows = GetPrefRows();
        }
        CreateGeometryString(newGeometry, x, y, 0, 0,
                bitmask & ~(WidthValue | HeightValue));
    }
    
    /* Create a new toplevel shell to hold the window */
    ac = 0;
#ifdef SGI_CUSTOM
    strcat(sgi_title, name);
    XtSetArg(al[ac], XmNtitle, sgi_title); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    if (strncmp(name, "Untitled", 8) == 0) { 
        XtSetArg(al[ac], XmNiconName, APP_NAME); ac++;
    } else {
        XtSetArg(al[ac], XmNiconName, name); ac++;
    }
#else
    XtSetArg(al[ac], XmNtitle, name); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(al[ac], XmNiconName, name); ac++;
#endif
    XtSetArg(al[ac], XmNgeometry, newGeometry[0]=='\0'?NULL:newGeometry); ac++;
    XtSetArg(al[ac], XmNinitialState,
            iconic ? IconicState : NormalState); ac++;

    appShell = CreateShellWithBestVis(APP_NAME, APP_CLASS,
                applicationShellWidgetClass, TheDisplay, al, ac);
    window->shell = appShell;

#ifdef EDITRES
    XtAddEventHandler (appShell, (EventMask)0, True,
                (XtEventHandler)_XEditResCheckMessages, NULL);
#endif /* EDITRES */

#ifndef SGI_CUSTOM
    addWindowIcon(appShell);
#endif

    /* Create a MainWindow to manage the menubar and text area, set the
       userData resource to be used by WidgetToWindow to recover the
       window pointer from the widget id of any of the window's widgets */
    XtSetArg(al[ac], XmNuserData, window); ac++;
    mainWin = XmCreateMainWindow(appShell, "main", al, ac);
    XtManageChild(mainWin);
    
    /* The statsAreaForm holds the stats line and the I-Search line. */
    statsAreaForm = XtVaCreateWidget("statsAreaForm", 
            xmFormWidgetClass, mainWin,
            XmNmarginWidth, STAT_SHADOW_THICKNESS,
            XmNmarginHeight, STAT_SHADOW_THICKNESS,
            /* XmNautoUnmanage, False, */
            NULL);
    if(window->showISearchLine || window->showStats)
        XtManageChild(statsAreaForm);
            
    /* NOTE: due to a bug in openmotif 2.1.30, NEdit used to crash when
       the i-search bar was active, and the i-search text widget was focussed,
       and the window's width was resized to nearly zero. 
       In theory, it is possible to avoid this by imposing a minimum
       width constraint on the nedit windows, but that width would have to
       be at least 30 characters, which is probably unacceptable.
       Amazingly, adding a top offset of 1 pixel to the toggle buttons of 
       the i-search bar, while keeping the the top offset of the text widget 
       to 0 seems to avoid avoid the crash. */
       
    window->iSearchForm = XtVaCreateWidget("iSearchForm", 
            xmFormWidgetClass, statsAreaForm,
            XmNleftAttachment, XmATTACH_FORM,
            XmNtopAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNbottomAttachment, window->showStats ?
                XmATTACH_NONE : XmATTACH_FORM,
            NULL);
    if(window->showISearchLine)
        XtManageChild(window->iSearchForm);
    iSearchLabel = XtVaCreateManagedWidget("iSearchLabel",
            xmLabelWidgetClass, window->iSearchForm,
            XmNlabelString, s1=XmStringCreateSimple("Find:"),
            XmNmarginHeight, 0,
            XmNleftAttachment, XmATTACH_FORM,
            XmNleftOffset, 5,
            XmNtopAttachment, XmATTACH_FORM,
            XmNtopOffset, 1, /* see openmotif note above, for aligment 
                                with toggle buttons below */
            XmNbottomAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);

    /* Disable keyboard traversal of the toggle buttons.  We were doing
       this previously by forcing the keyboard focus back to the text
       widget whenever a toggle changed.  That causes an ugly focus flash
       on screen.  It's better just not to go there in the first place. 
       Plus, if the user really wants traversal, it's an X resource so it
       can be enabled without too much pain and suffering. */
    
    window->iSearchCaseToggle = XtVaCreateManagedWidget("iSearchCaseToggle",
            xmToggleButtonWidgetClass, window->iSearchForm,
            XmNlabelString, s1=XmStringCreateSimple("Case"),
            XmNset, GetPrefSearch() == SEARCH_CASE_SENSE 
            || GetPrefSearch() == SEARCH_REGEX
            || GetPrefSearch() == SEARCH_CASE_SENSE_WORD,
            XmNtopAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopOffset, 1, /* see openmotif note above */
            XmNrightAttachment, XmATTACH_FORM,
            XmNmarginHeight, 0, 
            XmNtraversalOn, False,
            NULL);
    XmStringFree(s1);
    
    window->iSearchRegexToggle = XtVaCreateManagedWidget("iSearchREToggle",
            xmToggleButtonWidgetClass, window->iSearchForm,
            XmNlabelString, s1=XmStringCreateSimple("RegExp"),
            XmNset, GetPrefSearch() == SEARCH_REGEX_NOCASE 
            || GetPrefSearch() == SEARCH_REGEX,
            XmNtopAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopOffset, 1, /* see openmotif note above */
            XmNrightAttachment, XmATTACH_WIDGET,
            XmNrightWidget, window->iSearchCaseToggle,
            XmNmarginHeight, 0,
            XmNtraversalOn, False,
            NULL);
    XmStringFree(s1);
    
    window->iSearchRevToggle = XtVaCreateManagedWidget("iSearchRevToggle",
            xmToggleButtonWidgetClass, window->iSearchForm,
            XmNlabelString, s1=XmStringCreateSimple("Rev"),
            XmNset, False,
            XmNtopAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNtopOffset, 1, /* see openmotif note above */
            XmNrightAttachment, XmATTACH_WIDGET,
            XmNrightWidget, window->iSearchRegexToggle,
            XmNmarginHeight, 0,
            XmNtraversalOn, False,
            NULL);
    XmStringFree(s1);
    
    window->iSearchText = XtVaCreateManagedWidget("iSearchText",
            xmTextWidgetClass, window->iSearchForm,
            XmNmarginHeight, 1,
            XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
            XmNleftAttachment, XmATTACH_WIDGET,
            XmNleftWidget, iSearchLabel,
            XmNrightAttachment, XmATTACH_WIDGET,
            XmNrightWidget, window->iSearchRevToggle,
            XmNrightOffset, 5,
            XmNtopAttachment, XmATTACH_FORM,
            XmNtopOffset, 0, /* see openmotif note above */
            XmNbottomAttachment, XmATTACH_FORM,
            XmNbottomOffset, 0, NULL);
    RemapDeleteKey(window->iSearchText);

    SetISearchTextCallbacks(window);
    
    /* A form to hold the stats line text and line/col widgets */
    window->statsLineForm = XtVaCreateWidget("statsLineForm",
            xmFormWidgetClass, statsAreaForm,
            XmNshadowThickness, 0,
            XmNtopAttachment, window->showISearchLine ?
                XmATTACH_WIDGET : XmATTACH_FORM,
            XmNtopWidget, window->iSearchForm,
            XmNrightAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNresizable, False,    /*  */
            NULL);
    
    /* A separate display of the line/column number */
    window->statsLineColNo = XtVaCreateManagedWidget("statsLineColNo",
            xmLabelWidgetClass, window->statsLineForm,
            XmNlabelString, s1=XmStringCreateSimple("L: ---  C: ---"),
            XmNshadowThickness, 0,
            XmNmarginHeight, 0,
            XmNmarginTop, 1,    /* Help align with statsLine */
            XmNtraversalOn, False,
            XmNtopAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM, /*  */
            NULL);
    XmStringFree(s1);
    
    /* Create file statistics display area.  Using a text widget rather than
       a label solves a layout problem with the main window, which messes up
       if the label is too long (we would need a resize callback to control
       the length when the window changed size), and allows users to select
       file names and line numbers.  Colors are copied from parent
       widget, because many users and some system defaults color text
       backgrounds differently from other widgets. */

    XtVaGetValues(statsAreaForm, XmNbackground, &bgpix, NULL);
    XtVaGetValues(statsAreaForm, XmNforeground, &fgpix, NULL);
    stats = XtVaCreateManagedWidget("statsLine", 
            xmTextWidgetClass,  window->statsLineForm,
            XmNbackground, bgpix,
            XmNforeground, fgpix,
            XmNshadowThickness, 0,
            XmNhighlightColor, bgpix,
            XmNhighlightThickness, 1,  /* Setting this to zero causes mysterious
                                          problems with lesstif! */
            XmNmarginHeight, 0,
            XmNscrollHorizontal, False,
            XmNeditMode, XmSINGLE_LINE_EDIT,
            XmNeditable, False,
            XmNtraversalOn, False,
            XmNcursorPositionVisible, False,
            XmNtopAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_WIDGET,
            XmNrightWidget, window->statsLineColNo,
            XmNrightOffset, 3,
            NULL);
    window->statsLine = stats;
    
    /* Manage the statsLineForm */
    if(window->showStats)
        XtManageChild(window->statsLineForm);
    
    /* If the fontList was NULL, use the magical default provided by Motif,
       since it must have worked if we've gotten this far */
    if (window->fontList == NULL)
        XtVaGetValues(stats, XmNfontList, &window->fontList, NULL);

    /* Create the menu bar */
    menuBar = CreateMenuBar(mainWin, window);
    window->menuBar = menuBar;
    XtManageChild(menuBar);
    
    /* Create paned window to manage split window behavior */
    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass,  mainWin,
            XmNseparatorOn, False,
            XmNspacing, 3, XmNsashIndent, -2, NULL);
    window->splitPane = pane;
    XmMainWindowSetAreas(mainWin, menuBar, statsAreaForm, NULL, NULL, pane);

    /* Patch around Motif's most idiotic "feature", that its menu accelerators
       recognize Caps Lock and Num Lock as modifiers, and don't trigger if
       they are engaged */ 
    AccelLockBugPatch(pane, menuBar);

    /* Create the first, and most permanent text area (other panes may
       be added & removed, but this one will never be removed */
    text = createTextArea(pane, window, rows,cols,
            GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters(),
            GetPrefWrapMargin(), window->showLineNumbers?MIN_LINE_NUM_COLS:0);
    XtManageChild(text);
    window->textArea = text;
    window->lastFocus = text;

    /* Set the initial colors from the globals. */
    SetColors(window,
              GetPrefColorName(TEXT_FG_COLOR  ),
              GetPrefColorName(TEXT_BG_COLOR  ),
              GetPrefColorName(SELECT_FG_COLOR),
              GetPrefColorName(SELECT_BG_COLOR),
              GetPrefColorName(HILITE_FG_COLOR),
              GetPrefColorName(HILITE_BG_COLOR),
              GetPrefColorName(LINENO_FG_COLOR),
              GetPrefColorName(CURSOR_FG_COLOR));

    /* Create the right button popup menu (note: order is important here,
       since the translation for popping up this menu was probably already
       added in createTextArea, but CreateBGMenu requires window->textArea
       to be set so it can attach the menu to it (because menu shells are
       finicky about the kinds of widgets they are attached to)) */
    window->bgMenuPane = CreateBGMenu(window);
    
    /* Create the text buffer rather than using the one created automatically
       with the text area widget.  This is done so the syntax highlighting
       modify callback can be called to synchronize the style buffer BEFORE
       the text display's callback is called upon to display a modification */
    window->buffer = BufCreate();
    BufAddModifyCB(window->buffer, SyntaxHighlightModifyCB, window);
    
    /* Attach the buffer to the text widget, and add callbacks for modify */
    TextSetBuffer(text, window->buffer);
    BufAddModifyCB(window->buffer, modifiedCB, window);
    
    /* Designate the permanent text area as the owner for selections */
    HandleXSelections(text);
    
    /* Set the requested hardware tab distance and useTabs in the text buffer */
    BufSetTabDistance(window->buffer, GetPrefTabDist(PLAIN_LANGUAGE_MODE));
    window->buffer->useTabs = GetPrefInsertTabs();

    /* add the window to the global window list, update the Windows menus */
    addToWindowList(window);
    InvalidateWindowMenus();
    
    /* realize all of the widgets in the new window */
    RealizeWithoutForcingPosition(appShell);
    XmProcessTraversal(text, XmTRAVERSE_CURRENT);

    /* Make close command in window menu gracefully prompt for close */
    AddMotifCloseCallback(appShell, (XtCallbackProc)closeCB, window);
    
#ifndef NO_SESSION_RESTART
    /* Add wm protocol callback for making nedit restartable by session
       managers.  Doesn't yet handle multiple-desktops or iconifying right. */
    if (syAtom == 0) {
        wmpAtom = XmInternAtom(TheDisplay, "WM_PROTOCOLS", FALSE);
        syAtom = XmInternAtom(TheDisplay, "WM_SAVE_YOURSELF", FALSE);
    }
    XmAddProtocolCallback(appShell, wmpAtom, syAtom,
            (XtCallbackProc)saveYourselfCB, (XtPointer)window);
#endif
        
    /* Make window resizing work in nice character heights */
    UpdateWMSizeHints(window);
    
    /* Set the minimum pane height for the initial text pane */
    UpdateMinPaneHeights(window);
    
    restoreInsaneVirtualKeyBindings(invalidBindings);

    return window;
}

/*
** Close an editor window
*/
void CloseWindow(WindowInfo *window)
{
    int keepWindow;
    char name[MAXPATHLEN];
    
    /* Free smart indent macro programs */
    EndSmartIndent(window);
    
    /* Clean up macro references to the doomed window.  If a macro is
       executing, stop it.  If macro is calling this (closing its own
       window), leave the window alive until the macro completes */
    keepWindow = !MacroWindowCloseActions(window);
    
#ifndef VMS
    /* Kill shell sub-process and free related memory */
    AbortShellCommand(window);
#endif /*VMS*/
    
    /* Unload the default tips files for this language mode if necessary */
    UnloadLanguageModeTipsFile(window);

    /* If a window is closed while it is on the multi-file replace dialog
       list of any other window (or even the same one), we must update those
       lists or we end up with dangling references. Normally, there can 
       be only one of those dialogs at the same time (application modal),
       but LessTif doesn't even (always) honor application modalness, so
       there can be more than one dialog. */
    RemoveFromMultiReplaceDialog(window);
    
    /* Destroy the file closed property for this file */
    DeleteFileClosedProperty(window);

    /* if this is the last window, or must be kept alive temporarily because
       it's running the macro calling us, don't close it, make it Untitled */
    if (keepWindow || (WindowList == window && window->next == NULL)) {
        window->filename[0] = '\0';
        UniqueUntitledName(name);
        CLEAR_ALL_LOCKS(window->lockReasons);
        window->fileMode = 0;
        strcpy(window->filename, name);
        strcpy(window->path, "");
        window->ignoreModify = TRUE;
        BufSetAll(window->buffer, "");
        window->ignoreModify = FALSE;
        window->filenameSet = FALSE;
        window->fileMissing = TRUE;
        window->fileChanged = FALSE;
        window->fileFormat = UNIX_FILE_FORMAT;
        window->lastModTime = 0;
        StopHighlighting(window);
        EndSmartIndent(window);
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
        DetermineLanguageMode(window, True);
        return;
    }
    
    /* Free syntax highlighting patterns, if any. w/o redisplaying */
    FreeHighlightingData(window);
        
    /* remove the buffer modification callbacks so the buffer will be
       deallocated when the last text widget is destroyed */
    BufRemoveModifyCB(window->buffer, modifiedCB, window);
    BufRemoveModifyCB(window->buffer, SyntaxHighlightModifyCB, window);

#ifdef ROWCOLPATCH
    patchRowCol(window->menuBar);
#endif
    
    /* free the undo and redo lists */
    ClearUndoList(window);
    ClearRedoList(window);
    
    /* remove and deallocate all of the widgets associated with window */
    XtFree(window->backlightCharTypes); /* we made a copy earlier on */
    XtDestroyWidget(window->shell);
    
    /* remove the window from the global window list, update window menus */
    removeFromWindowList(window);
    InvalidateWindowMenus();
    
    /* deallocate the window data structure */
    XtFree((char *)window);
}

/*
** Check if there is already a window open for a given file
*/
WindowInfo *FindWindowWithFile(const char *name, const char *path)
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
    int i, focusPane, emTabDist, wrapMargin, lineNumCols, totalHeight=0;
    char *delimiters;
    Widget text;
    textDisp *textD, *newTextD;
    
    /* Don't create new panes if we're already at the limit */
    if (window->nPanes >= MAX_PANES)
        return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, keyboard focus */
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
        text = i==0 ? window->textArea : window->textPanes[i-1];
        insertPositions[i] = TextGetCursorPos(text);
        XtVaGetValues(containingPane(text),XmNheight,&paneHeights[i],NULL);
        totalHeight += paneHeights[i];
        TextGetScroll(text, &topLines[i], &horizOffsets[i]);
        if (text == window->lastFocus)
            focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Create a text widget to add to the pane and set its buffer and
       highlight data to be the same as the other panes in the window */
    XtVaGetValues(window->textArea, textNemulateTabs, &emTabDist,
            textNwordDelimiters, &delimiters, textNwrapMargin, &wrapMargin,
            textNlineNumCols, &lineNumCols, NULL);
    text = createTextArea(window->splitPane, window, 1, 1, emTabDist,
            delimiters, wrapMargin, lineNumCols);
    TextSetBuffer(text, window->buffer);
    if (window->highlightData != NULL)
    	AttachHighlightToWidget(text, window);
    if (window->backlightChars)
    {
        XtVaSetValues(text, textNbacklightCharTypes,
                window->backlightCharTypes, 0);
    }
    XtManageChild(text);
    window->textPanes[window->nPanes++] = text;
    
    /* Fix up the colors */
    textD = ((TextWidget)window->textArea)->text.textD;
    newTextD = ((TextWidget)text)->text.textD;
    XtVaSetValues(text,
                XmNforeground, textD->fgPixel,
                XmNbackground, textD->bgPixel,
                NULL);
    TextDSetColors( newTextD, textD->fgPixel, textD->bgPixel, 
            textD->selectFGPixel, textD->selectBGPixel, textD->highlightFGPixel,
            textD->highlightBGPixel, textD->lineNumFGPixel, 
            textD->cursorFGPixel );
    
    /* Set the minimum pane height in the new pane */
    UpdateMinPaneHeights(window);

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
        setPaneDesiredHeight(containingPane(text), paneHeights[i]);
    }

    /* Re-manage panedWindow to recalculate pane heights & reset selection */
    XtManageChild(window->splitPane);
    
    /* Reset all of the heights, scroll positions, etc. */
    for (i=0; i<=window->nPanes; i++) {
        text = i==0 ? window->textArea : window->textPanes[i-1];
        TextSetCursorPos(text, insertPositions[i]);
        TextSetScroll(text, topLines[i], horizOffsets[i]);
        setPaneDesiredHeight(containingPane(text),
                            totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
    
    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
            wmSizeUpdateProc, window);
}

Widget GetPaneByIndex(WindowInfo *window, int paneIndex)
{
    Widget text = NULL;
    if (paneIndex >= 0 && paneIndex <= window->nPanes) {
        text = (paneIndex == 0) ? window->textArea : window->textPanes[paneIndex - 1];
    }
    return(text);
}

int WidgetToPaneIndex(WindowInfo *window, Widget w)
{
    int i;
    Widget text;
    int paneIndex = 0;
    
    for (i = 0; i <= window->nPanes; ++i) {
        text = (i == 0) ? window->textArea : window->textPanes[i - 1];
        if (text == w) {
            paneIndex = i;
        }
    }
    return(paneIndex);
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
        XtVaGetValues(containingPane(text),
                            XmNheight, &paneHeights[i], NULL);
        totalHeight += paneHeights[i];
        TextGetScroll(text, &topLines[i], &horizOffsets[i]);
        if (text == window->lastFocus)
            focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Destroy last pane, and make sure lastFocus points to an existing pane.
       Workaround for OM 2.1.30: text widget must be unmanaged for 
       xmPanedWindowWidget to calculate the correct pane heights for
       the remaining panes, simply detroying it didn't seem enough */
    window->nPanes--;
    XtUnmanageChild(containingPane(window->textPanes[window->nPanes]));
    XtDestroyWidget(containingPane(window->textPanes[window->nPanes]));

    if (window->nPanes == 0)
        window->lastFocus = window->textArea;
    else if (focusPane > window->nPanes)
        window->lastFocus = window->textPanes[window->nPanes-1];
    
    /* adjust the heights, scroll positions, etc., to make it look
       like the pane with the input focus was closed */
    for (i=focusPane; i<=window->nPanes; i++) {
        insertPositions[i] = insertPositions[i+1];
        paneHeights[i] = paneHeights[i+1];
        topLines[i] = topLines[i+1];
        horizOffsets[i] = horizOffsets[i+1];
    }
    
    /* set the desired heights and re-manage the paned window so it will
       recalculate pane heights */
    for (i=0; i<=window->nPanes; i++) {
        text = i==0 ? window->textArea : window->textPanes[i-1];
        setPaneDesiredHeight(containingPane(text), paneHeights[i]);
    }
    XtManageChild(window->splitPane);
    
    /* Reset all of the scroll positions, insert positions, etc. */
    for (i=0; i<=window->nPanes; i++) {
        text = i==0 ? window->textArea : window->textPanes[i-1];
        TextSetCursorPos(text, insertPositions[i]);
        TextSetScroll(text, topLines[i], horizOffsets[i]);
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);

    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
            wmSizeUpdateProc, window);
}

/*
** Turn on and off the display of line numbers
*/
void ShowLineNumbers(WindowInfo *window, int state)
{
    Widget text;
    int i, marginWidth;
    Dimension windowWidth;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    
    if (window->showLineNumbers == state)
        return;
    window->showLineNumbers = state;
    
    /* Just setting window->showLineNumbers is sufficient to tell
       UpdateLineNumDisp to expand the line number areas and the window
       size for the number of lines required.  To hide the line number
       display, set the width to zero, and contract the window width. */
    if (state) {
        UpdateLineNumDisp(window);
    } else {
        XtVaGetValues(window->shell, XmNwidth, &windowWidth, NULL);
        XtVaGetValues(window->textArea, textNmarginWidth, &marginWidth, NULL);
        XtVaSetValues(window->shell, XmNwidth,
                windowWidth - textD->left + marginWidth, NULL);
        for (i=0; i<=window->nPanes; i++) {
            text = i==0 ? window->textArea : window->textPanes[i-1];
            XtVaSetValues(text, textNlineNumCols, 0, NULL);
        }
    }
            
    /* Tell WM that the non-expandable part of the window has changed size */
    UpdateWMSizeHints(window);
}

void SetTabDist(WindowInfo *window, int tabDist)
{
    if (window->buffer->tabDist != tabDist) {
        int saveCursorPositions[MAX_PANES + 1];
        int saveVScrollPositions[MAX_PANES + 1];
        int saveHScrollPositions[MAX_PANES + 1];
        int paneIndex;
        
        window->ignoreModify = True;
        
        for (paneIndex = 0; paneIndex <= window->nPanes; ++paneIndex) {
            Widget w = GetPaneByIndex(window, paneIndex);
            textDisp *textD = ((TextWidget)w)->text.textD;

            TextGetScroll(w, &saveVScrollPositions[paneIndex], &saveHScrollPositions[paneIndex]);
            saveCursorPositions[paneIndex] = TextGetCursorPos(w);
            textD->modifyingTabDist = 1;
        }
        
        BufSetTabDistance(window->buffer, tabDist);

        for (paneIndex = 0; paneIndex <= window->nPanes; ++paneIndex) {
            Widget w = GetPaneByIndex(window, paneIndex);
            textDisp *textD = ((TextWidget)w)->text.textD;
            
            textD->modifyingTabDist = 0;
            TextSetCursorPos(w, saveCursorPositions[paneIndex]);
            TextSetScroll(w, saveVScrollPositions[paneIndex], saveHScrollPositions[paneIndex]);
        }
        
        window->ignoreModify = False;
    }
}

void SetEmTabDist(WindowInfo *window, int emTabDist)
{
    int i;

    XtVaSetValues(window->textArea, textNemulateTabs, emTabDist, NULL);
    for (i = 0; i < window->nPanes; ++i) {
        XtVaSetValues(window->textPanes[i], textNemulateTabs, emTabDist, NULL);
    }
}

/*
** Turn on and off the display of the statistics line
*/
void ShowStatsLine(WindowInfo *window, int state)
{
    Widget text;
    int i;
    
    /* In continuous wrap mode, text widgets must be told to keep track of
       the top line number in absolute (non-wrapped) lines, because it can
       be a costly calculation, and is only needed for displaying line
       numbers, either in the stats line, or along the left margin */
    for (i=0; i<=window->nPanes; i++) {
        text = i==0 ? window->textArea : window->textPanes[i-1];
        TextDMaintainAbsLineNum(((TextWidget)text)->text.textD, state);
    }
    window->showStats = state;
    showStats(window, state);
}

/*
** Displays and undisplays the statistics line (regardless of settings of
** window->showStats or window->modeMessageDisplayed)
*/
static void showStats(WindowInfo *window, int state)
{
    if (state) {
        Dimension ht;

        XtVaSetValues(window->iSearchForm,
                XmNbottomAttachment, XmATTACH_NONE, NULL);
        XtManageChild(window->statsLineForm);
        showStatsForm(window, True);
        
        /* workaround for Lesstif (0.93.0, maybe others) to reveal the 
            statsline */
        XtVaGetValues(window->statsLine, XmNheight, &ht, NULL);
        XtVaSetValues(window->statsLineColNo, XmNheight, ht, NULL);
    } else {
        XtUnmanageChild(window->statsLineForm);
        XtVaSetValues(window->iSearchForm,
                XmNbottomAttachment, XmATTACH_FORM, NULL);
        showStatsForm(window, window->showISearchLine);
    }
      
    /* Tell WM that the non-expandable part of the window has changed size */
    /* Already done in showStatsForm */
    /* UpdateWMSizeHints(window); */
}

/*
** Turn on and off the continuing display of the incremental search line
** (when off, it is popped up and down as needed via TempShowISearch)
*/
void ShowISearchLine(WindowInfo *window, int state)
{
    if (window->showISearchLine == state)
        return;
    window->showISearchLine = state;
    showISearch(window, state);
}

/*
** Temporarily show and hide the incremental search line if the line is not
** already up.
*/
void TempShowISearch(WindowInfo *window, int state)
{
    if (window->showISearchLine)
        return;
    if (XtIsManaged(window->iSearchForm) != state)
        showISearch(window, state);
}

/*
** Put up or pop-down the incremental search line regardless of settings
** of showISearchLine or TempShowISearch
*/
static void showISearch(WindowInfo *window, int state)
{
    if (state) {
        XtManageChild(window->iSearchForm);
        XtVaSetValues(window->statsLineForm, XmNtopAttachment, 
                XmATTACH_WIDGET, XmNtopWidget, window->iSearchForm, NULL);
        showStatsForm(window, True);
    } else {
        XtUnmanageChild(window->iSearchForm);
        XtVaSetValues(window->statsLineForm, XmNtopAttachment, 
                XmATTACH_FORM, NULL);
        showStatsForm(window, window->showStats || 
                window->modeMessageDisplayed);
    }
      
    /* Tell WM that the non-expandable part of the window has changed size */
    /* This is already done in showStatsForm */
    /* UpdateWMSizeHints(window); */
}

/*
** Show or hide the extra display area under the main menu bar which
** optionally contains the status line and the incremental search bar
*/
static void showStatsForm(WindowInfo *window, int state)
{
    Widget statsAreaForm = XtParent(window->statsLineForm);
    Widget mainW = XtParent(statsAreaForm);

    /* The very silly use of XmNcommandWindowLocation and XmNshowSeparator
       below are to kick the main window widget to position and remove the
       status line when it is managed and unmanaged.  At some Motif version
       level, the showSeparator trick backfires and leaves the separator
       shown, but fortunately the dynamic behavior is fixed, too so the
       workaround is no longer necessary, either.  (... the version where
       this occurs may be earlier than 2.1.  If the stats line shows
       double thickness shadows in earlier Motif versions, the #if XmVersion
       directive should be moved back to that earlier version) */
    if (state) {
        XtUnmanageChild(statsAreaForm);    /*... will this fix Solaris 7??? */
        XtVaSetValues(mainW, XmNcommandWindowLocation,
                XmCOMMAND_ABOVE_WORKSPACE, NULL);
#if XmVersion < 2001
        XtVaSetValues(mainW, XmNshowSeparator, True, NULL);
#endif
        XtManageChild(statsAreaForm);
        XtVaSetValues(mainW, XmNshowSeparator, False, NULL);
        UpdateStatsLine(window);
    } else {
        XtUnmanageChild(statsAreaForm);
        XtVaSetValues(mainW, XmNcommandWindowLocation,
                XmCOMMAND_BELOW_WORKSPACE, NULL);
    }
      
    /* Tell WM that the non-expandable part of the window has changed size */
    UpdateWMSizeHints(window);
}

/*
** Display a special message in the stats line (show the stats line if it
** is not currently shown).
*/
void SetModeMessage(WindowInfo *window, const char *message)
{
    window->modeMessageDisplayed = True;
    XmTextSetString(window->statsLine, (char*)message);
    /*
     * Don't invoke the stats line again, if stats line is already displayed.
     */
    if (!window->showStats)
	showStats(window, True);
}

/*
** Clear special statistics line message set in SetModeMessage, returns
** the statistics line to its original state as set in window->showStats
*/
void ClearModeMessage(WindowInfo *window)
{
    window->modeMessageDisplayed = False;
    /*
     * Remove the stats line only if indicated by it's window state.
     */
    if (!window->showStats)
        showStats(window, False);
    UpdateStatsLine(window);
}

/*
** Count the windows
*/
int NWindows(void)
{
    WindowInfo *win;
    int n;
    
    for (win=WindowList, n=0; win!=NULL; win=win->next, n++);
    return n;
}

/*
** Set autoindent state to one of  NO_AUTO_INDENT, AUTO_INDENT, or SMART_INDENT.
*/
void SetAutoIndent(WindowInfo *window, int state)
{
    int autoIndent = state == AUTO_INDENT, smartIndent = state == SMART_INDENT;
    int i;
    
    if (window->indentStyle == SMART_INDENT && !smartIndent)
        EndSmartIndent(window);
    else if (smartIndent && window->indentStyle != SMART_INDENT)
        BeginSmartIndent(window, True);
    window->indentStyle = state;
    XtVaSetValues(window->textArea, textNautoIndent, autoIndent,
            textNsmartIndent, smartIndent, NULL);
    for (i=0; i<window->nPanes; i++)
        XtVaSetValues(window->textPanes[i], textNautoIndent, autoIndent,
                textNsmartIndent, smartIndent, NULL);
    XmToggleButtonSetState(window->smartIndentItem, smartIndent, False);
    XmToggleButtonSetState(window->autoIndentItem, autoIndent, False);
    XmToggleButtonSetState(window->autoIndentOffItem, state == NO_AUTO_INDENT,
            False);
}

/*
** Set showMatching state to one of NO_FLASH, FLASH_DELIMIT or FLASH_RANGE.
** Update the menu to reflect the change of state.
*/
void SetShowMatching(WindowInfo *window, int state)
{
    window->showMatchingStyle = state;
    XmToggleButtonSetState(window->showMatchingOffItem, 
        state == NO_FLASH, False);
    XmToggleButtonSetState(window->showMatchingDelimitItem, 
        state == FLASH_DELIMIT, False);
    XmToggleButtonSetState(window->showMatchingRangeItem, 
        state == FLASH_RANGE, False);
}

/*
** Set the fonts for "window" from a font name, and updates the display.
** Also updates window->fontList which is used for statistics line.
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
void SetFonts(WindowInfo *window, const char *fontName, const char *italicName,
        const char *boldName, const char *boldItalicName)
{
    XFontStruct *font, *oldFont;
    int i, oldFontWidth, oldFontHeight, fontWidth, fontHeight;
    int borderWidth, borderHeight, marginWidth, marginHeight;
    int primaryChanged, highlightChanged = False;
    Dimension oldWindowWidth, oldWindowHeight, oldTextWidth, oldTextHeight;
    Dimension textHeight, newWindowWidth, newWindowHeight;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    
    /* Check which fonts have changed */
    primaryChanged = strcmp(fontName, window->fontName);
    if (strcmp(italicName, window->italicFontName)) highlightChanged = True;
    if (strcmp(boldName, window->boldFontName)) highlightChanged = True;
    if (strcmp(boldItalicName, window->boldItalicFontName))
        highlightChanged = True;
    if (!primaryChanged && !highlightChanged)
        return;
        
    /* Get information about the current window sizing, to be used to
       determine the correct window size after the font is changed */
    XtVaGetValues(window->shell, XmNwidth, &oldWindowWidth, XmNheight,
            &oldWindowHeight, NULL);
    XtVaGetValues(window->textArea, XmNheight, &textHeight,
            textNmarginHeight, &marginHeight, textNmarginWidth,
            &marginWidth, textNfont, &oldFont, NULL);
    oldTextWidth = textD->width + textD->lineNumWidth;
    oldTextHeight = textHeight - 2*marginHeight;
    for (i=0; i<window->nPanes; i++) {
        XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, NULL);
        oldTextHeight += textHeight - 2*marginHeight;
    }
    borderWidth = oldWindowWidth - oldTextWidth;
    borderHeight = oldWindowHeight - oldTextHeight;
    oldFontWidth = oldFont->max_bounds.width;
    oldFontHeight = textD->ascent + textD->descent;
    
        
    /* Change the fonts in the window data structure.  If the primary font
       didn't work, use Motif's fallback mechanism by stealing it from the
       statistics line.  Highlight fonts are allowed to be NULL, which
       is interpreted as "use the primary font" */
    if (primaryChanged) {
        strcpy(window->fontName, fontName);
        font = XLoadQueryFont(TheDisplay, fontName);
        if (font == NULL)
            XtVaGetValues(window->statsLine, XmNfontList, &window->fontList,
                    NULL);
        else
            window->fontList = XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
    }
    if (highlightChanged) {
        strcpy(window->italicFontName, italicName);
        window->italicFontStruct = XLoadQueryFont(TheDisplay, italicName);
        strcpy(window->boldFontName, boldName);
        window->boldFontStruct = XLoadQueryFont(TheDisplay, boldName);
        strcpy(window->boldItalicFontName, boldItalicName);
        window->boldItalicFontStruct = XLoadQueryFont(TheDisplay, boldItalicName);
    }

    /* Change the primary font in all the widgets */
    if (primaryChanged) {
        font = GetDefaultFontStruct(window->fontList);
        XtVaSetValues(window->textArea, textNfont, font, NULL);
        for (i=0; i<window->nPanes; i++)
            XtVaSetValues(window->textPanes[i], textNfont, font, NULL);
    }
    
    /* Change the highlight fonts, even if they didn't change, because
       primary font is read through the style table for syntax highlighting */
    if (window->highlightData != NULL)
        UpdateHighlightStyles(window);
        
    /* Change the window manager size hints. 
       Note: this has to be done _before_ we set the new sizes. ICCCM2
       compliant window managers (such as fvwm2) would otherwise resize
       the window twice: once because of the new sizes requested, and once
       because of the new size increments, resulting in an overshoot. */
    UpdateWMSizeHints(window);
    
    /* Use the information from the old window to re-size the window to a
       size appropriate for the new font */
    fontWidth = GetDefaultFontStruct(window->fontList)->max_bounds.width;
    fontHeight = textD->ascent + textD->descent;
    newWindowWidth = (oldTextWidth*fontWidth) / oldFontWidth + borderWidth;
    newWindowHeight = (oldTextHeight*fontHeight) / oldFontHeight + borderHeight;
    XtVaSetValues(window->shell, XmNwidth, newWindowWidth, XmNheight,
            newWindowHeight, NULL);
    
    /* Change the minimum pane height */
    UpdateMinPaneHeights(window);
}

void SetColors(WindowInfo *window, const char *textFg, const char *textBg,
        const char *selectFg, const char *selectBg, const char *hiliteFg, 
        const char *hiliteBg, const char *lineNoFg, const char *cursorFg)
{
    int i, dummy;
    Pixel   textFgPix   = AllocColor( window->textArea, textFg, 
                    &dummy, &dummy, &dummy),
            textBgPix   = AllocColor( window->textArea, textBg, 
                    &dummy, &dummy, &dummy),
            selectFgPix = AllocColor( window->textArea, selectFg, 
                    &dummy, &dummy, &dummy),
            selectBgPix = AllocColor( window->textArea, selectBg, 
                    &dummy, &dummy, &dummy),
            hiliteFgPix = AllocColor( window->textArea, hiliteFg, 
                    &dummy, &dummy, &dummy),
            hiliteBgPix = AllocColor( window->textArea, hiliteBg, 
                    &dummy, &dummy, &dummy),
            lineNoFgPix = AllocColor( window->textArea, lineNoFg, 
                    &dummy, &dummy, &dummy),
            cursorFgPix = AllocColor( window->textArea, cursorFg, 
                    &dummy, &dummy, &dummy);
    textDisp *textD;

    /* Update the main pane */
    XtVaSetValues(window->textArea,
            XmNforeground, textFgPix,
            XmNbackground, textBgPix,
            NULL);
    textD = ((TextWidget)window->textArea)->text.textD;
    TextDSetColors( textD, textFgPix, textBgPix, selectFgPix, selectBgPix, 
            hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix );
    /* Update any additional panes */
    for (i=0; i<window->nPanes; i++) {
        XtVaSetValues(window->textPanes[i],
                XmNforeground, textFgPix,
                XmNbackground, textBgPix,
                NULL);
        textD = ((TextWidget)window->textPanes[i])->text.textD;
        TextDSetColors( textD, textFgPix, textBgPix, selectFgPix, selectBgPix, 
                hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix );
    }
    
    /* Redo any syntax highlighting */
    if (window->highlightData != NULL)
        UpdateHighlightStyles(window);
}

/*
** Set insert/overstrike mode
*/
void SetOverstrike(WindowInfo *window, int overstrike)
{
    int i;

    XtVaSetValues(window->textArea, textNoverstrike, overstrike, NULL);
    for (i=0; i<window->nPanes; i++)
        XtVaSetValues(window->textPanes[i], textNoverstrike, overstrike, NULL);
    window->overstrike = overstrike;
}

/*
** Select auto-wrap mode, one of NO_WRAP, NEWLINE_WRAP, or CONTINUOUS_WRAP
*/
void SetAutoWrap(WindowInfo *window, int state)
{
    int i;
    int autoWrap = state == NEWLINE_WRAP, contWrap = state == CONTINUOUS_WRAP;

    XtVaSetValues(window->textArea, textNautoWrap, autoWrap,
            textNcontinuousWrap, contWrap, NULL);
    for (i=0; i<window->nPanes; i++)
        XtVaSetValues(window->textPanes[i], textNautoWrap, autoWrap,
                textNcontinuousWrap, contWrap, NULL);
    window->wrapMode = state;
    
    XmToggleButtonSetState(window->newlineWrapItem, autoWrap, False);
    XmToggleButtonSetState(window->continuousWrapItem, contWrap, False);
    XmToggleButtonSetState(window->noWrapItem, state == NO_WRAP, False);
}

/*
** Set the wrap margin (0 == wrap at right edge of window)
*/
void SetWrapMargin(WindowInfo *window, int margin)
{
    int i;
    
    XtVaSetValues(window->textArea, textNwrapMargin, margin, NULL);
    for (i=0; i<window->nPanes; i++)
        XtVaSetValues(window->textPanes[i], textNwrapMargin, margin, NULL);
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
    XtVaGetValues(w, XmNuserData, &window, NULL);
    return window;
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
void UpdateWindowTitle(const WindowInfo *window)
{
    char *title = FormatWindowTitle(window->filename,
                                    window->path,
#ifdef VMS
                                    NULL,
#else
                                    GetClearCaseViewTag(),
#endif /* VMS */
                                    GetPrefServerName(),
                                    IsServer,
                                    window->filenameSet,
                                    window->lockReasons,
                                    window->fileChanged,
                                    GetPrefTitleFormat());
                   
    char *iconTitle = XtMalloc(strlen(window->filename) + 2); /* strlen("*")+1 */

    strcpy(iconTitle, window->filename);
    if (window->fileChanged)
        strcat(iconTitle, "*");
    XtVaSetValues(window->shell, XmNtitle, title, XmNiconName, iconTitle, NULL);
    
    /* If there's a find or replace dialog up in "Keep Up" mode, with a
       file name in the title, update it too */
    if (window->findDlog && XmToggleButtonGetState(window->findKeepBtn)) {
        sprintf(title, "Find (in %s)", window->filename);
        XtVaSetValues(XtParent(window->findDlog), XmNtitle, title, NULL);
    }
    if (window->replaceDlog && XmToggleButtonGetState(window->replaceKeepBtn)) {
        sprintf(title, "Replace (in %s)", window->filename);
        XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, NULL);
    }
    XtFree(iconTitle);

    /* Update the Windows menus with the new name */
    InvalidateWindowMenus();
}

/*
** Update the read-only state of the text area(s) in the window, and
** the ReadOnly toggle button in the File menu to agree with the state in
** the window data structure.
*/
void UpdateWindowReadOnly(WindowInfo *window)
{
    int i, state;
    
    state = IS_ANY_LOCKED(window->lockReasons);
    XtVaSetValues(window->textArea, textNreadOnly, state, NULL);
    for (i=0; i<window->nPanes; i++)
        XtVaSetValues(window->textPanes[i], textNreadOnly, state, NULL);
    XmToggleButtonSetState(window->readOnlyItem, state, FALSE);
    XtSetSensitive(window->readOnlyItem,
        !IS_ANY_LOCKED_IGNORING_USER(window->lockReasons));
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
    textDisp *textD = ((TextWidget)textPane)->text.textD;
    int topChar = TextFirstVisiblePos(textPane);
    int lastChar = TextLastVisiblePos(textPane);
    int targetLineNum;
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
            (left <= topChar && right >= lastChar))) {
        XtVaGetValues(textPane, textNrows, &rows, NULL);
        scrollOffset = rows/3;
        TextGetScroll(textPane, &topLineNum, &horizOffset);
        if (right > lastChar) {
            /* End of sel. is below bottom of screen */
            leftLineNum = topLineNum +
                    TextDCountLines(textD, topChar, left, False);
            targetLineNum = topLineNum + scrollOffset;
            if (leftLineNum >= targetLineNum) {
                /* Start of sel. is not between top & target */
                linesToScroll = TextDCountLines(textD, lastChar, right, False) +
                        scrollOffset;
                if (leftLineNum - linesToScroll < targetLineNum)
                    linesToScroll = leftLineNum - targetLineNum;
                /* Scroll start of selection to the target line */
                TextSetScroll(textPane, topLineNum+linesToScroll, horizOffset);
            }
        } else if (left < topChar) {
            /* Start of sel. is above top of screen */
            lastLineNum = topLineNum + rows;
            rightLineNum = lastLineNum -
                    TextDCountLines(textD, right, lastChar, False);
            targetLineNum = lastLineNum - scrollOffset;
            if (rightLineNum <= targetLineNum) {
                /* End of sel. is not between bottom & target */
                linesToScroll = TextDCountLines(textD, left, topChar, False) +
                        scrollOffset;
                if (rightLineNum + linesToScroll > targetLineNum)
                    linesToScroll = targetLineNum - rightLineNum;
                /* Scroll end of selection to the target line */
                TextSetScroll(textPane, topLineNum-linesToScroll, horizOffset);
            }
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
        XtVaGetValues(textPane, XmNwidth, &width, textNmarginWidth, &margin,
                NULL);
        if (leftX < margin + textD->lineNumLeft + textD->lineNumWidth)
            horizOffset -=
                    margin + textD->lineNumLeft + textD->lineNumWidth - leftX;
        else if (rightX > width - margin)
            horizOffset += rightX - (width - margin);
        TextSetScroll(textPane, topLineNum, horizOffset);
    }

    /* make sure that the statistics line is up to date */
    UpdateStatsLine(window);
}

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
        int cols, int emTabDist, char *delimiters, int wrapMargin,
        int lineNumCols)
{
    Widget text, sw, hScrollBar, vScrollBar, frame;
        
    /* Create a text widget inside of a scrolled window widget */
    sw = XtVaCreateManagedWidget("scrolledW", xmScrolledWindowWidgetClass,
            parent, XmNpaneMaximum, SHRT_MAX,
            XmNpaneMinimum, PANE_MIN_HEIGHT, XmNhighlightThickness, 0, NULL); 
    hScrollBar = XtVaCreateManagedWidget("textHorScrollBar",
            xmScrollBarWidgetClass, sw, XmNorientation, XmHORIZONTAL, 
            XmNrepeatDelay, 10, NULL);
    vScrollBar = XtVaCreateManagedWidget("textVertScrollBar",
            xmScrollBarWidgetClass, sw, XmNorientation, XmVERTICAL,
            XmNrepeatDelay, 10, NULL);
    frame = XtVaCreateManagedWidget("textFrame", xmFrameWidgetClass, sw,
            XmNshadowType, XmSHADOW_IN, NULL);      
    text = XtVaCreateManagedWidget("text", textWidgetClass, frame,
            textNbacklightCharTypes, window->backlightCharTypes,
            textNrows, rows, textNcolumns, cols,
            textNlineNumCols, lineNumCols,
            textNemulateTabs, emTabDist,
            textNfont, GetDefaultFontStruct(window->fontList),
            textNhScrollBar, hScrollBar, textNvScrollBar, vScrollBar,
            textNreadOnly, IS_ANY_LOCKED(window->lockReasons),
            textNwordDelimiters, delimiters,
            textNwrapMargin, wrapMargin,
            textNautoIndent, window->indentStyle == AUTO_INDENT,
            textNsmartIndent, window->indentStyle == SMART_INDENT,
            textNautoWrap, window->wrapMode == NEWLINE_WRAP,
            textNcontinuousWrap, window->wrapMode == CONTINUOUS_WRAP,
            textNoverstrike, window->overstrike,
            textNhidePointer, (Boolean) GetPrefTypingHidesPointer(),
            NULL);

    XtVaSetValues(sw, XmNworkWindow, frame, XmNhorizontalScrollBar, 
                    hScrollBar, XmNverticalScrollBar, vScrollBar, NULL);

    /* add focus, drag, cursor tracking, and smart indent callbacks */
    XtAddCallback(text, textNfocusCallback, (XtCallbackProc)focusCB, window);
    XtAddCallback(text, textNcursorMovementCallback, (XtCallbackProc)movedCB,
            window);
    XtAddCallback(text, textNdragStartCallback, (XtCallbackProc)dragStartCB,
            window);
    XtAddCallback(text, textNdragEndCallback, (XtCallbackProc)dragEndCB,
            window);
    XtAddCallback(text, textNsmartIndentCallback, SmartIndentCB, window);
            
    /* This makes sure the text area initially has a the insert point shown
       ... (check if still true with the nedit text widget, probably not) */
    XmAddTabGroup(containingPane(text));
    
    /* compensate for Motif delete/backspace problem */
    RemapDeleteKey(text);

    /* Augment translation table for right button popup menu */
    AddBGMenuAction(text);
    
    /* If absolute line numbers will be needed for display in the statistics
       line, tell the widget to maintain them (otherwise, it's a costly
       operation and performance will be better without it) */
    TextDMaintainAbsLineNum(((TextWidget)text)->text.textD, window->showStats);
   
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
    
    /* Check for changes to read-only status and/or file modifications */
    CheckForChangesToFile(window);
}

static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
        char *deletedText, void *cbArg) 
{
    WindowInfo *window = (WindowInfo *)cbArg;
    int selected = window->buffer->primary.selected;
    
    /* update the table of bookmarks */
    UpdateMarkTable(window, pos, nInserted, nDeleted);
    
    /* Check and dim/undim selection related menu items */
    if ((window->wasSelected && !selected) ||
        (!window->wasSelected && selected)) {
        window->wasSelected = selected;
        XtSetSensitive(window->printSelItem, selected);
        XtSetSensitive(window->cutItem, selected);
        XtSetSensitive(window->copyItem, selected);
        XtSetSensitive(window->delItem, selected);
        /* Note we don't change the selection for items like
           "Open Selected" and "Find Selected".  That's because
           it works on selections in external applications.
           Desensitizing it if there's no NEdit selection 
           disables this feature. */
#ifndef VMS
        XtSetSensitive(window->filterItem, selected);
#endif

        DimSelectionDepUserMenuItems(window, selected);
        if (window->replaceDlog != NULL)
        {
            UpdateReplaceActionButtons(window);
        }
    }
    
    /* Make sure line number display is sufficient for new data */
    UpdateLineNumDisp(window);
    
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
    
    /* Check if external changes have been made to file and warn user */
    CheckForChangesToFile(window);
}

static void focusCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    /* record which window pane last had the keyboard focus */
    window->lastFocus = w;
    
    /* update line number statistic to reflect current focus pane */
    UpdateStatsLine(window);
    
    /* finish off the current incremental search */
    EndISearch(window);
    
    /* Check for changes to read-only status and/or file modifications */
    CheckForChangesToFile(window);
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
        if (!CheckPrefsChangesSaved(window->shell))
            return;
        if (!WindowList->fileChanged)
            exit(EXIT_SUCCESS);
        if (CloseFileAndWindow(window, PROMPT_SBC_DIALOG_RESPONSE))
            exit(EXIT_SUCCESS);
    } else
        CloseFileAndWindow(window, PROMPT_SBC_DIALOG_RESPONSE);
}

static void saveYourselfCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    WindowInfo *win, **revWindowList;
    char geometry[MAX_GEOM_STRING_LEN];
    int argc = 0, maxArgc, nWindows, i;
    char **argv;
    int wasIconic = False;
    
    /* Only post a restart command on the first window in the window list so
       session manager can restart the whole set of windows in one executable,
       rather than one nedit per file.  Even if the restart command is not on
       this window, the protocol demands that we set the window's WM_COMMAND
       property in response to the "save yourself" message */
    if (window != WindowList) {
        XSetCommand(TheDisplay, XtWindow(window->shell), NULL, 0);
        return;
    }
   
    /* Allocate memory for an argument list and for a reversed list of
       windows.  The window list is reversed for IRIX 4DWM and any other
       window/session manager combination which uses window creation
       order for re-associating stored geometry information with
       new windows created by a restored application */
    maxArgc = 1;
    nWindows = 0;
    for (win=WindowList; win!=NULL; win=win->next) {
        maxArgc += 4;
        nWindows++;
    }
    argv = (char **)XtMalloc(maxArgc*sizeof(char *));
    revWindowList = (WindowInfo **)XtMalloc(sizeof(WindowInfo *)*nWindows);
    for (win=WindowList, i=nWindows-1; win!=NULL; win=win->next, i--)
        revWindowList[i] = win;
        
    /* Create command line arguments for restoring each window in the list */
    argv[argc++] = XtNewString(ArgV0);
    if (IsServer) {
        argv[argc++] = XtNewString("-server");
        if (GetPrefServerName()[0] != '\0') {
            argv[argc++] = XtNewString("-svrname");
            argv[argc++] = XtNewString(GetPrefServerName());
        }
    }
    for (i=0; i<nWindows; i++) {
        win = revWindowList[i];
        getGeometryString(win, geometry);
        argv[argc++] = XtNewString("-geometry");
        argv[argc++] = XtNewString(geometry);
        if (isIconic(win)) {
            argv[argc++] = XtNewString("-iconic");
            wasIconic = True;
        } else if (wasIconic) {
            argv[argc++] = XtNewString("-noiconic");
            wasIconic = False;
        }
        if (win->filenameSet) {
            argv[argc] = XtMalloc(strlen(win->path) +
                    strlen(win->filename) + 1);
            sprintf(argv[argc++], "%s%s", win->path, win->filename);
        }
    }
    XtFree((char *)revWindowList);
    
    /* Set the window's WM_COMMAND property to the created command line */
    XSetCommand(TheDisplay, XtWindow(window->shell), argv, argc);
    for (i=0; i<argc; i++)
        XtFree(argv[i]);
    XtFree((char *)argv);
}

/*
** Returns true if window is iconic (as determined by the WM_STATE property
** on the shell window.  I think this is the most reliable way to tell,
** but if someone has a better idea please send me a note).
*/
static int isIconic(WindowInfo *window)
{
    unsigned long *property = NULL;
    unsigned long nItems;
    unsigned long leftover;
    static Atom wmStateAtom = 0;
    Atom actualType;
    int actualFormat;
    int result;
  
    if (wmStateAtom == 0)
        wmStateAtom = XInternAtom (XtDisplay(window->shell), "WM_STATE", False); 
    if (XGetWindowProperty(XtDisplay(window->shell), XtWindow(window->shell),
            wmStateAtom, 0L, 1L, False, wmStateAtom, &actualType, &actualFormat,
            &nItems, &leftover, (unsigned char **)&property) != Success ||
            nItems != 1 || property == NULL)
        return FALSE;
    result = *property == IconicState;
    XtFree((char *)property);
    return result;
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
** If necessary, enlarges the window and line number display area
** to make room for numbers.
*/
void UpdateLineNumDisp(WindowInfo *window)
{
    Dimension windowWidth;
    int i, fontWidth, reqCols, lineNumCols;
    int oldWidth, newWidth, marginWidth;
    Widget text;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    
    if (!window->showLineNumbers)
        return;
    
    /* Decide how wide the line number field has to be to display all
       possible line numbers */
    reqCols = textD->nBufferLines<1 ? 1 : log10((double)textD->nBufferLines)+1;
    if (reqCols < MIN_LINE_NUM_COLS)
        reqCols = MIN_LINE_NUM_COLS;
     
    /* Is the width of the line number area sufficient to display all the
       line numbers in the file?  If not, expand line number field, and the
       window width */
    XtVaGetValues(window->textArea, textNlineNumCols, &lineNumCols,
            textNmarginWidth, &marginWidth, NULL);
    if (lineNumCols < reqCols) {
        fontWidth = textD->fontStruct->max_bounds.width;
        oldWidth = textD->left - marginWidth;
        newWidth = reqCols * fontWidth + marginWidth;
        XtVaGetValues(window->shell, XmNwidth, &windowWidth, NULL);
        XtVaSetValues(window->shell, XmNwidth,
                windowWidth + newWidth-oldWidth, NULL);
        UpdateWMSizeHints(window);
        for (i=0; i<=window->nPanes; i++) {
            text = i==0 ? window->textArea : window->textPanes[i-1];
            XtVaSetValues(text, textNlineNumCols, reqCols, NULL);
        }               
    }
}

/*
** Update the optional statistics line.  
*/
void UpdateStatsLine(WindowInfo *window)
{
    int line, pos, colNum;
    char *string, *format, slinecol[32];
    Widget statW = window->statsLine;
    XmString xmslinecol;
#ifdef SGI_CUSTOM
    char *sleft, *smid, *sright;
#endif
    
    /* This routine is called for each character typed, so its performance
       affects overall editor perfomance.  Only update if the line is on. */ 
    if (!window->showStats)
        return;
    
    /* Compose the string to display. If line # isn't available, leave it off */
    pos = TextGetCursorPos(window->lastFocus);
    string = XtMalloc(strlen(window->filename) + strlen(window->path) + 45);
    format = window->fileFormat == DOS_FILE_FORMAT ? " DOS" :
            (window->fileFormat == MAC_FILE_FORMAT ? " Mac" : "");
    if (!TextPosToLineAndCol(window->lastFocus, pos, &line, &colNum)) {
        sprintf(string, "%s%s%s %d bytes", window->path, window->filename,
                format, window->buffer->length);
        sprintf(slinecol, "L: ---  C: ---");
    } else {
        sprintf(slinecol, "L: %d  C: %d", line, colNum);
        if (window->showLineNumbers)
            sprintf(string, "%s%s%s byte %d of %d", window->path,
                    window->filename, format, pos, 
                    window->buffer->length);
        else
            sprintf(string, "%s%s%s %d bytes", window->path,
                    window->filename, format, window->buffer->length);
    }
    
    /* Don't clobber the line if there's a special message being displayed */
    if (!window->modeMessageDisplayed) {
        /* Change the text in the stats line */
#ifdef SGI_CUSTOM
        /* don't show full pathname, just dir and filename (+ byte info) */
        smid = strchr(string, '/'); 
        if ( smid != NULL ) {
            sleft = smid;
            sright = strrchr(string, '/'); 
            while (strcmp(smid, sright)) {
                    sleft = smid;
                    smid = strchr(sleft + 1, '/');
            }
            XmTextReplace(statW, 0, XmTextGetLastPosition(statW), sleft + 1);
        } else
            XmTextReplace(statW, 0, XmTextGetLastPosition(statW), string);
#else
        XmTextReplace(statW, 0, XmTextGetLastPosition(statW), string);
#endif
    }
    XtFree(string);
    
    /* Update the line/col display */
    xmslinecol = XmStringCreateSimple(slinecol);
    XtVaSetValues(window->statsLineColNo,
            XmNlabelString, xmslinecol, NULL);
    XmStringFree(xmslinecol);
}

static Boolean currentlyBusy = False;
static long busyStartTime = 0;
static Boolean modeMessageSet = False;

/*
 * Auxiliary function for measuring elapsed time during busy waits.
 */
static long getRelTimeInTenthsOfSeconds()
{
#ifdef __unix__
    struct timeval current;
    gettimeofday(&current, NULL);
    return (current.tv_sec*10 + current.tv_usec/100000) & 0xFFFFFFFL;
#else
    time_t current;
    time(&current);
    return (current*10) & 0xFFFFFFFL;
#endif
}

void AllWindowsBusy(const char *message)
{
    WindowInfo *w;

    if (!currentlyBusy)
    {
	busyStartTime = getRelTimeInTenthsOfSeconds();
	modeMessageSet = False;
        
        for (w=WindowList; w!=NULL; w=w->next)
        {
            /* We don't the display message here yet, but defer it for 
               a while. If the wait is short, we don't want
               to have it flash on and off the screen.  However,
               we can't use a time since in generally we are in
               a tight loop and only processing exposure events, so it's
               up to the caller to make sure that this routine is called 
               at regular intervals.
            */
            BeginWait(w->shell);
        }
    } else if (!modeMessageSet && message && 
		getRelTimeInTenthsOfSeconds() - busyStartTime > 10) {
	/* Show the mode message when we've been busy for more than a second */ 
	for (w=WindowList; w!=NULL; w=w->next) {
	    SetModeMessage(w, message);
	}
	modeMessageSet = True;
    }
    BusyWait(WindowList->shell);
            
    currentlyBusy = True;        
}

void AllWindowsUnbusy(void)
{
    WindowInfo *w;

    for (w=WindowList; w!=NULL; w=w->next)
    {
        ClearModeMessage(w);
        EndWait(w->shell);
    }

    currentlyBusy = False;
    modeMessageSet = False;
    busyStartTime = 0;
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
void UpdateWMSizeHints(WindowInfo *window)
{
    Dimension shellWidth, shellHeight, textHeight, hScrollBarHeight;
    int marginHeight, marginWidth, totalHeight;
    XFontStruct *fs;
    int i, baseWidth, baseHeight, fontHeight, fontWidth;
    Widget hScrollBar;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;

    /* Find the base (non-expandable) width and height of the editor window */
    XtVaGetValues(window->textArea, XmNheight, &textHeight,
            textNmarginHeight, &marginHeight, textNmarginWidth, &marginWidth,
            NULL);
    totalHeight = textHeight - 2*marginHeight;
    for (i=0; i<window->nPanes; i++) {
        XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, 
                textNhScrollBar, &hScrollBar, NULL);
        totalHeight += textHeight - 2*marginHeight;
        if (!XtIsManaged(hScrollBar)) {
            XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, NULL);
            totalHeight -= hScrollBarHeight;
        }
    }
    XtVaGetValues(window->shell, XmNwidth, &shellWidth,
            XmNheight, &shellHeight, NULL);
    baseWidth = shellWidth - textD->width;
    baseHeight = shellHeight - totalHeight;
    
    /* Find the dimensions of a single character of the text font */
    XtVaGetValues(window->textArea, textNfont, &fs, NULL);
    fontHeight = textD->ascent + textD->descent;
    fontWidth = fs->max_bounds.width;
    
    /* Set the size hints in the shell widget */
    XtVaSetValues(window->shell, XmNwidthInc, fs->max_bounds.width,
            XmNheightInc, fontHeight,
            XmNbaseWidth, baseWidth, XmNbaseHeight, baseHeight,
            XmNminWidth, baseWidth + fontWidth,
            XmNminHeight, baseHeight + (1+window->nPanes) * fontHeight, NULL);
}

/*
** Update the minimum allowable height for a split window pane after a change
** to font or margin height.
*/
void UpdateMinPaneHeights(WindowInfo *window)
{
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    Dimension hsbHeight, swMarginHeight,frameShadowHeight;
    int i, marginHeight, minPaneHeight;
    Widget hScrollBar;

    /* find the minimum allowable size for a pane */
    XtVaGetValues(window->textArea, textNhScrollBar, &hScrollBar, NULL);
    XtVaGetValues(containingPane(window->textArea),
            XmNscrolledWindowMarginHeight, &swMarginHeight, NULL);
    XtVaGetValues(XtParent(window->textArea),
            XmNshadowThickness, &frameShadowHeight, NULL);
    XtVaGetValues(window->textArea, textNmarginHeight, &marginHeight, NULL);
    XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, NULL);
    minPaneHeight = textD->ascent + textD->descent + marginHeight*2 +
            swMarginHeight*2 + hsbHeight + 2*frameShadowHeight;
    
    /* Set it in all of the widgets in the window */
    setPaneMinHeight(containingPane(window->textArea), minPaneHeight);
    for (i=0; i<window->nPanes; i++)
        setPaneMinHeight(containingPane(window->textPanes[i]),
                   minPaneHeight);
}

/* Add an icon to an applicaction shell widget.  addWindowIcon adds a large
** (primary window) icon, AddSmallIcon adds a small (secondary window) icon.
**
** Note: I would prefer that these were not hardwired, but yhere is something
** weird about the  XmNiconPixmap resource that prevents it from being set
** from the defaults in the application resource database.
*/
static void addWindowIcon(Widget shell)
{ 
    static Pixmap iconPixmap = 0, maskPixmap = 0;

    if (iconPixmap == 0) {
        iconPixmap = XCreateBitmapFromData(TheDisplay,
                RootWindowOfScreen(XtScreen(shell)), (char *)iconBits,
                iconBitmapWidth, iconBitmapHeight);
        maskPixmap = XCreateBitmapFromData(TheDisplay,
                RootWindowOfScreen(XtScreen(shell)), (char *)maskBits,
                iconBitmapWidth, iconBitmapHeight);
    }
    XtVaSetValues(shell, XmNiconPixmap, iconPixmap, XmNiconMask, maskPixmap,
            NULL);
}
void AddSmallIcon(Widget shell)
{ 
    static Pixmap iconPixmap = 0, maskPixmap = 0;

    if (iconPixmap == 0) {
        iconPixmap = XCreateBitmapFromData(TheDisplay,
                RootWindowOfScreen(XtScreen(shell)), (char *)n_bits,
                n_width, n_height);
        maskPixmap = XCreateBitmapFromData(TheDisplay,
                RootWindowOfScreen(XtScreen(shell)), (char *)n_mask,
                n_width, n_height);
    }
    XtVaSetValues(shell, XmNiconPixmap, iconPixmap,
            XmNiconMask, maskPixmap, NULL);
}

/*
** Save the position and size of a window as an X standard geometry string.
** A string of at least MAX_GEOMETRY_STRING_LEN characters should be
** provided in the argument "geomString" to receive the result.
*/
static void getGeometryString(WindowInfo *window, char *geomString)
{
    int x, y, fontWidth, fontHeight, baseWidth, baseHeight;
    unsigned int width, height, dummyW, dummyH, bw, depth, nChild;
    Window parent, root, *child, w = XtWindow(window->shell);
    Display *dpy = XtDisplay(window->shell);
    
    /* Find the width and height from the window of the shell */
    XGetGeometry(dpy, w, &root, &x, &y, &width, &height, &bw, &depth);
    
    /* Find the top left corner (x and y) of the window decorations.  (This
       is what's required in the geometry string to restore the window to it's
       original position, since the window manager re-parents the window to
       add it's title bar and menus, and moves the requested window down and
       to the left.)  The position is found by traversing the window hier-
       archy back to the window to the last parent before the root window */
    for(;;) {
        XQueryTree(dpy, w, &root, &parent,  &child, &nChild);
        XFree((char*)child);
        if (parent == root)
            break;
        w = parent;
    }
    XGetGeometry(dpy, w, &root, &x, &y, &dummyW, &dummyH, &bw, &depth);
    
    /* Use window manager size hints (set by UpdateWMSizeHints) to
       translate the width and height into characters, as opposed to pixels */
    XtVaGetValues(window->shell, XmNwidthInc, &fontWidth,
            XmNheightInc, &fontHeight, XmNbaseWidth, &baseWidth,
            XmNbaseHeight, &baseHeight, NULL);
    width = (width-baseWidth) / fontWidth;
    height = (height-baseHeight) / fontHeight;

    /* Write the string */
    CreateGeometryString(geomString, x, y, width, height,
                XValue | YValue | WidthValue | HeightValue);
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
    UpdateWMSizeHints((WindowInfo *)clientData);
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

/*
** Set the backlight character class string
*/
void SetBacklightChars(WindowInfo *window, char *applyBacklightTypes)
{
    int i;
    int is_applied = XmToggleButtonGetState(window->backlightCharsItem) ? 1 : 0;
    int do_apply = applyBacklightTypes ? 1 : 0;

    window->backlightChars = do_apply;

    XtFree(window->backlightCharTypes);
    if (window->backlightChars &&
      (window->backlightCharTypes = XtMalloc(strlen(applyBacklightTypes)+1)))
      strcpy(window->backlightCharTypes, applyBacklightTypes);
    else
      window->backlightCharTypes = NULL;

    XtVaSetValues(window->textArea,
          textNbacklightCharTypes, window->backlightCharTypes, 0);
    for (i=0; i<window->nPanes; i++)
      XtVaSetValues(window->textPanes[i],
              textNbacklightCharTypes, window->backlightCharTypes, 0);
    if (is_applied != do_apply)
      XmToggleButtonSetState(window->backlightCharsItem, do_apply, False);
}

static int sortAlphabetical(const void* k1, const void* k2)
{
    const char* key1 = *(const char**)k1;
    const char* key2 = *(const char**)k2;
    return strcmp(key1, key2);
}

/*
 * Checks whether a given virtual key binding string is invalid. 
 * A binding is considered invalid if there are duplicate key entries.
 */
static int virtKeyBindingsAreInvalid(const unsigned char* bindings)
{
    int maxCount = 1, i, count;
    const char  *pos = (const char*)bindings;
    char *copy;
    char *pos2, *pos3;
    char **keys;
    /* First count the number of bindings; bindings are separated by \n
       strings. The number of bindings equals the number of \n + 1.
       Beware of leading and trailing \n; the number is actually an
       upper bound on the number of entries. */
    while ((pos = strstr(pos, "\n"))) { ++pos; ++maxCount; }
    
    if (maxCount == 1) return False; /* One binding is always ok */
    
    keys = (char**)malloc(maxCount*sizeof(char*));
    copy = XtNewString((const char*)bindings);
    i = 0;
    pos2 = copy;
    
    count = 0;
    while (i<maxCount && pos2 && *pos2)
    {
        while (isspace((int) *pos2) || *pos2 == '\n') ++pos2;
        
        if (*pos2 == '!') /* Ignore comment lines */
        {
            pos2 = strstr(pos2, "\n");
            continue; /* Go to the next line */
        }

        if (*pos2)
        {
            keys[i++] = pos2;
            ++count;
            pos3 = strstr(pos2, ":");
            if (pos3) 
            {
                *pos3++ = 0; /* Cut the string and jump to the next entry */
                pos2 = pos3;
            }
            pos2 = strstr(pos2, "\n");
        }
    }
    
    if (count <= 1)
    {
       free(keys);
       XtFree(copy);
       return False; /* No conflict */
    }
    
    /* Sort the keys and look for duplicates */
    qsort((void*)keys, count, sizeof(const char*), sortAlphabetical);
    for (i=1; i<count; ++i)
    {
        if (!strcmp(keys[i-1], keys[i]))
        {
            /* Duplicate detected */
            free(keys);
            XtFree(copy);
            return True;
        }
    }
    free(keys);
    XtFree(copy);
    return False;
}

/*
 * Optionally sanitizes the Motif default virtual key bindings. 
 * Some applications install invalid bindings (attached to the root window),
 * which cause certain keys to malfunction in NEdit.
 * Through an X-resource, users can choose whether they want
 *   - to always keep the existing bindings
 *   - to override the bindings only if they are invalid
 *   - to always override the existing bindings.
 */

static Atom virtKeyAtom;

static unsigned char* sanitizeVirtualKeyBindings()
{
    int overrideBindings = GetPrefOverrideVirtKeyBindings();
    Window rootWindow;
    const char *virtKeyPropName = "_MOTIF_DEFAULT_BINDINGS";
    Atom dummyAtom;
    int getFmt;
    unsigned long dummyULong, nItems;
    unsigned char *insaneVirtKeyBindings = NULL;
    
    if (overrideBindings == VIRT_KEY_OVERRIDE_NEVER) return NULL;
    
    virtKeyAtom =  XInternAtom(TheDisplay, virtKeyPropName, False);
    rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));

    /* Remove the property, if it exists; we'll restore it later again */
    if (XGetWindowProperty(TheDisplay, rootWindow, virtKeyAtom, 0, INT_MAX, 
                           True, XA_STRING, &dummyAtom, &getFmt, &nItems, 
                           &dummyULong, &insaneVirtKeyBindings) != Success 
        || nItems == 0) 
    {
        return NULL; /* No binding yet; nothing to do */
    }
    
    if (overrideBindings == VIRT_KEY_OVERRIDE_AUTO)
    {   
        if (!virtKeyBindingsAreInvalid(insaneVirtKeyBindings))
        {
            /* Restore the property immediately; it seems valid */
            XChangeProperty(TheDisplay, rootWindow, virtKeyAtom, XA_STRING, 8,
                            PropModeReplace, insaneVirtKeyBindings, 
                            strlen((const char*)insaneVirtKeyBindings));
            XFree((char*)insaneVirtKeyBindings);
            return NULL; /* Prevent restoration */
        }
    }
    return insaneVirtKeyBindings;
}

/*
 * NEdit should not mess with the bindings installed by other apps, so we
 * just restore whatever was installed, if necessary
 */
static void restoreInsaneVirtualKeyBindings(unsigned char *insaneVirtKeyBindings)
{
   if (insaneVirtKeyBindings)
   {
      Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
      /* Restore the root window atom, such that we don't affect 
         other apps. */
      XChangeProperty(TheDisplay, rootWindow, virtKeyAtom, XA_STRING, 8,
                      PropModeReplace, insaneVirtKeyBindings, 
                      strlen((const char*)insaneVirtKeyBindings));
      XFree((char*)insaneVirtKeyBindings);
   }
}

static Widget containingPane(Widget w)
{
    /* The containing pane used to simply be the first parent, but with
       the introduction of an XmFrame, it's the grandparent. */
    return XtParent(XtParent(w));
}
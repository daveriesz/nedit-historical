/*******************************************************************************
*									       *
* menu.c -- Nirvana Editor menus					       *
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
static char SCCSID[] = "@(#)menu.c	1.31     9/20/94";
#include <stdlib.h>
#include <stdio.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <X11/X.h>
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/MenuShell.h>
#include "../util/getfiles.h"
#include "../util/fontsel.h"
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "nedit.h"
#include "file.h"
#include "menu.h"
#include "window.h"
#include "search.h"
#include "clipboard.h"
#include <Xm/Text.h>
#include "undo.h"
#include "shift.h"
#include "help.h"
#include "preferences.h"
#include "tags.h"
#include "shell.h"

typedef void (*menuCallbackProc)();

static void newCB(Widget w, WindowInfo *window, caddr_t callData);
static void openCB(Widget w, WindowInfo *window, caddr_t callData);
static void openSelCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void closeCB(Widget w, WindowInfo *window, caddr_t callData);
static void saveCB(Widget w, WindowInfo *window, caddr_t callData);
static void saveAsCB(Widget w, WindowInfo *window, caddr_t callData); 
static void revertCB(Widget w, WindowInfo *window, caddr_t callData); 
static void includeCB(Widget w, WindowInfo *window, caddr_t callData);
static void loadTagsCB(Widget w, WindowInfo *window, caddr_t callData);
static void printCB(Widget w, WindowInfo *window, caddr_t callData);
static void printSelCB(Widget w, WindowInfo *window, caddr_t callData);
static void exitCB(Widget w, WindowInfo *window, caddr_t callData);
static void undoCB(Widget w, WindowInfo *window, caddr_t callData);
static void redoCB(Widget w, WindowInfo *window, caddr_t callData);
static void cutCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData); 
static void copyCB(Widget w,WindowInfo *window,XmAnyCallbackStruct *callData);
static void pasteCB(Widget w,WindowInfo *window, XmAnyCallbackStruct *callData);
static void clearCB(Widget w, WindowInfo *window, caddr_t callData);
static void selAllCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void shiftLeftCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void shiftRightCB (Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData); 
static void findSameCB(Widget w, WindowInfo *window,
		       XmAnyCallbackStruct *callData);
static void findSelCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData);
static void replaceCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData);
static void replaceSameCB(Widget w, WindowInfo *window,
			  XmAnyCallbackStruct *callData); 
static void gotoCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData); 
static void gotoSelectedCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData); 
static void findMatchingCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData); 
static void findDefCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData);
static void overstrikeCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabDistCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsLineDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabDistDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void filterDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void reposDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchLiteralCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchCaseSenseCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchRegexCB(Widget w, WindowInfo *window, caddr_t callData);
static void size24x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size40x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size60x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size80x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void sizeCustomCB(Widget w, WindowInfo *window, caddr_t callData);
static void savePrefCB(Widget w, WindowInfo *window, caddr_t callData);
static void splitWindowCB(Widget w, WindowInfo *window, caddr_t callData);
static void closePaneCB(Widget w, WindowInfo *window, caddr_t callData);
static void capitalizeCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void lowercaseCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void fillCB(Widget w, WindowInfo *window, caddr_t callData);
static void formFeedCB(Widget w, WindowInfo *window, caddr_t callData);
static void controlCB(Widget w, WindowInfo *window, caddr_t callData);
static void shellFilterCB(Widget w, WindowInfo *window, caddr_t callData);
static void execCB(Widget w, WindowInfo *window, caddr_t callData);
static void execLineCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData);
static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData);
static Widget createMenu(Widget parent, char *name, char *label);
static Widget createMenuItem(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg);
static Widget createFakeMenuItem(Widget parent, char *name,
	menuCallbackProc callback, void *cbArg);
static Widget createMenuToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set);
static Widget createMenuSeparator(Widget parent, char *name);
static void checkCloseDim(void);
static void updateWindowMenu(WindowInfo *window);
static void raiseCB(Widget w, WindowInfo *window, caddr_t callData);
static void setWindowSizeDefault(int rows, int cols);
static void updateWindowSizeMenus();
static void updateWindowSizeMenu(WindowInfo *win);

/*
** Create the menu bar
*/
Widget CreateMenuBar(Widget parent, WindowInfo *window)
{
    Widget menuBar, menuPane, btn, subPane, subSubPane, cascade;
    XmString st1;

    /*
    ** Create the menu bar (row column) widget
    */
    menuBar = XmCreateMenuBar(parent, "menuBar", NULL, 0);

    /*
    ** Create "File" pull down menu.
    */
    menuPane = createMenu(menuBar, "fileMenu", "File");
    createMenuItem(menuPane, "new", "New", 'N', newCB, window);
    createMenuItem(menuPane, "open", "Open...", 'O', openCB, window);
    createMenuItem(menuPane, "openSelected", "Open Selected", 'd',
    	    openSelCB, window);
    createMenuSeparator(menuPane, "sep1");
    window->closeItem = createMenuItem(menuPane, "close", "Close", 'C',
    	    closeCB, window);
    checkCloseDim();
    createMenuItem(menuPane, "save", "Save", 'S', saveCB, window);
    createMenuItem(menuPane, "saveAs", "Save As...", 'A', saveAsCB, window);
    createMenuItem(menuPane, "revertToSaved", "Revert to Saved", 'R',
    	    revertCB, window);
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "includeFile", "Include File...", 'I',
    	    includeCB, window);
    createMenuItem(menuPane, "loadTagsFile", "Load Tags File...", 'L',
    	    loadTagsCB, window);
    createMenuSeparator(menuPane, "sep3");
    createMenuItem(menuPane, "print", "Print...", 'P', printCB, window);
    createMenuItem(menuPane, "printSelection", "Print Selection...", 't',
    	    printSelCB, window);
    createMenuSeparator(menuPane, "sep4");
    createMenuItem(menuPane, "exit", "Exit", 'x', exitCB, window);

    /* 
    ** Create "Edit" pull down menu.
    */
    menuPane = createMenu(menuBar, "editMenu", "Edit");
    window->undoItem = createMenuItem(menuPane, "undo", "Undo", 'U',
    	    undoCB, window);
    XtSetSensitive(window->undoItem, False);
    window->redoItem = createMenuItem(menuPane, "redo", "Redo", 'd',
    	    redoCB, window);
    XtSetSensitive(window->redoItem, False);
    createMenuSeparator(menuPane, "sep1");
    createMenuItem(menuPane, "cut", "Cut", 't', cutCB, window);
    createMenuItem(menuPane, "copy", "Copy", 'C', copyCB, window);
    createMenuItem(menuPane, "paste", "Paste", 'P', pasteCB, window);
    createMenuItem(menuPane, "clear", "Clear", 'e', clearCB, window);
    createMenuItem(menuPane, "selectAll", "Select All", 'S', selAllCB, window);
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "shiftLeft", "Shift Left", 'L',
    	    shiftLeftCB, window);
    createFakeMenuItem(menuPane, "shiftLeftShift", shiftLeftCB, window);
    createMenuItem(menuPane, "shiftRight", "Shift Right", 'R',
    	    shiftRightCB, window);
    createFakeMenuItem(menuPane, "shiftRightShift", shiftRightCB, window);
    createMenuItem(menuPane, "capitalize", "Capitalize", 'z',
    	    capitalizeCB, window);
    createMenuItem(menuPane, "lowerCase", "Lower-case", 'o',
    	    lowercaseCB, window);
    createMenuItem(menuPane, "fillParagraph", "Fill Paragraph", 'F',
    	    fillCB, window);
    createMenuSeparator(menuPane, "sep3");
    createMenuItem(menuPane, "insertFormFeed", "Insert Form Feed", 'I',
    	    formFeedCB, window);
    createMenuItem(menuPane, "insControlCode", "Ins. Control Code", 'n',
    	    controlCB, window);

    /* 
    ** Create "Search" pull down menu.
    */
    menuPane = createMenu(menuBar, "searchMenu", "Search");
    createMenuItem(menuPane, "find", "Find...", 'F', findCB, window);
    createFakeMenuItem(menuPane, "findShift", findCB, window);
    createMenuItem(menuPane, "findSame", "Find Same", 'i', findSameCB, window);
    createFakeMenuItem(menuPane, "findSameShift", findSameCB, window);
    createMenuItem(menuPane, "findSelection", "Find Selection", 'S',
    	    findSelCB, window);
    createFakeMenuItem(menuPane, "findSelectionShift", findSelCB, window);
    createMenuItem(menuPane, "replace", "Replace...", 'R', replaceCB, window);
    createFakeMenuItem(menuPane, "replaceShift", replaceCB, window);
    createMenuItem(menuPane, "replaceSame", "Replace Same", 'p',
    	    replaceSameCB, window);
    createFakeMenuItem(menuPane, "replaceSameShift", replaceSameCB, window);
    createMenuSeparator(menuPane, "sep1");
    createMenuItem(menuPane, "gotoLine", "Goto Line Number", 'L',
    	    gotoCB, window);
    createMenuItem(menuPane, "gotoSelected", "Goto Selected", 'G',
    	    gotoSelectedCB, window);
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "match", "Match (..)", 'M',
    	    findMatchingCB, window);
    window->findDefItem = createMenuItem(menuPane, "findDefinition",
    	    "Find Definition", 'D', findDefCB, window);
    XtSetSensitive(window->findDefItem, TagsFileLoaded());
    
    /*
    ** Create the Preferences menu
    */
    menuPane = createMenu(menuBar, "preferencesMenu", "Preferences");
    createMenuToggle(menuPane, "autoIndent", "Auto Indent", 'A',
    	    autoIndentCB, window, window->autoIndent);
    createMenuToggle(menuPane, "autoWrap", "Auto Wrap", 'W',
    	    wrapCB, window, window->wrap);
    createMenuToggle(menuPane, "maintainBackup", "Maintain Backup", 'B',
    	    autoSaveCB, window, window->autoSave);
    createMenuToggle(menuPane, "showMatching", "Show Matching (..)", 'M',
    	    showMatchingCB, window, window->showMatching);
#ifndef IBM_DESTROY_BUG
    createMenuItem(menuPane, "textFont", "Text Font...", 'F', fontCB, window);
#endif /*IBM_DESTROY_BUG*/
    createMenuItem(menuPane, "tabDistance", "Tab Distance...", 'T',
    	    tabDistCB, window);
#if XmVersion >= 1002
    createMenuToggle(menuPane, "overstrike", "Overstrike", 'O',
    	    overstrikeCB, window, False);
#endif
    createMenuToggle(menuPane, "statisticsLine", "Statistics Line", 'S',
    	    statsCB, window, GetPrefStatsLine());
    createMenuSeparator(menuPane, "sep1");
    
    /* Default Settings sub menu */
    subPane = AddSubMenu(menuPane, "defaultSettings", "Default Settings", 'D');
    window->autoIndentDefItem = createMenuToggle(subPane, "autoIndent",
    	    "Auto Indent", 'A', autoIndentDefCB, window, GetPrefAutoIndent());
    window->wrapTextDefItem = createMenuToggle(subPane, "autoWrap",
    	    "Auto Wrap", 'W', wrapDefCB, window, GetPrefWrap());
    window->autoSaveDefItem = createMenuToggle(subPane, "periodicBackup",
    	    "Maintain Backup", 'B', autoSaveDefCB, window, GetPrefAutoSave());
    window->showMatchingDefItem = createMenuToggle(subPane, "showMatching",
    	    "Show Matching (..)", 'M', showMatchingDefCB, window,
    	    GetPrefShowMatching());
    createMenuItem(subPane, "textFont", "Text Font...", 'F', fontDefCB, window);
    createMenuItem(subPane, "tabDistance", "Tab Distance...", 'T',
    	    tabDistDefCB, window);
    window->statsLineDefItem = createMenuToggle(subPane, "statisticsLine",
    	    "Statistics Line", 'S', statsLineDefCB, window, GetPrefStatsLine());
#ifndef VMS
    createMenuItem(subPane, "shellCommands", "Shell Commands...", 'l',
    	    filterDefCB, window);
#endif
    window->searchDlogsDefItem = createMenuToggle(subPane, "verboseSearch",
    	    "Verbose Search", 'V', searchDlogsDefCB, window,
    	    GetPrefSearchDlogs());
    window->reposDlogsDefItem = createMenuToggle(subPane, "popupsUnderPointer",
    	    "Popups Under Pointer", 'P', reposDlogsDefCB, window,
    	    GetPrefRepositionDialogs());

    /* Search Method sub menu (a radio btn only pane w/ XmNradioBehavior on) */
    subSubPane = AddSubMenu(subPane, "defaultSearchStyle",
    	    "Default Search Style", 'D');
    XtVaSetValues(subSubPane, XmNradioBehavior, True, 0); 
    window->searchLiteralDefItem = createMenuToggle(subSubPane, "literal",
    	    "Literal", 'L', searchLiteralCB, window,
    	    GetPrefSearch() == SEARCH_LITERAL);
    window->searchCaseSenseDefItem = createMenuToggle(subSubPane,
    	    "caseSensitive", "Case Sensitive", 'C', searchCaseSenseCB, window,
    	    GetPrefSearch() == SEARCH_CASE_SENSE);
    window->searchRegexDefItem = createMenuToggle(subSubPane,
    	    "regularExpression", "Regular Expression", 'R', searchRegexCB,
    	    window, GetPrefSearch() == SEARCH_REGEX);
    
    /* Initial Window Size sub menu (simulates radioBehavior) */
    subSubPane = AddSubMenu(subPane, "initialwindowSize",
    	    "Initial Window Size", 'I');
    /* XtVaSetValues(subSubPane, XmNradioBehavior, True, 0);  */
    window->size24x80DefItem = btn = createMenuToggle(subSubPane, "24X80",
    	    "24 x 80", '2', size24x80CB, window, False);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, 0);
    window->size40x80DefItem = btn = createMenuToggle(subSubPane, "40X80",
    	    "40 x 80", '4', size40x80CB, window, False);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, 0);
    window->size60x80DefItem = btn = createMenuToggle(subSubPane, "60X80",
    	    "60 x 80", '6', size60x80CB, window, False);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, 0);
    window->size80x80DefItem = btn = createMenuToggle(subSubPane, "80X80",
    	    "80 x 80", '8', size80x80CB, window, False);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, 0);
    window->sizeCustomDefItem = btn = createMenuToggle(subSubPane, "custom",
    	    "Custom...", 'C', sizeCustomCB, window, False);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, 0);
    updateWindowSizeMenu(window);
    
    createMenuItem(menuPane, "saveDefaults", "Save Defaults", 'v',
    	    savePrefCB, window);

#ifndef VMS
    /*
    ** Create the Shell menu
    */
    menuPane = window->filterMenuPane =
    	    createMenu(menuBar, "shellMenu", "Shell");
    btn = createMenuItem(menuPane, "filterSelection", "Filter Selection...",
    	    'F', shellFilterCB, window);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    btn = createMenuItem(menuPane, "executeCommand", "Execute Command...",
    	    'E', execCB, window);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    btn = createMenuItem(menuPane, "executeCommandLine", "Execute Command Line",
    	    'x', execLineCB, window);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    btn = createMenuSeparator(menuPane, "sep2");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    UpdateFilterMenu(window);
#endif

    /*
    ** Create the Windows menu
    */
    menuPane = window->windowMenuPane =
    	    XmCreatePulldownMenu(menuBar, "windowsMenu", NULL, 0);
    cascade = XtVaCreateManagedWidget("windowsMenu", xmCascadeButtonWidgetClass,
    	    menuBar, XmNlabelString, st1=XmStringCreateSimple("Windows"),
    	    XmNsubMenuId, menuPane, 0);	    
    XtAddCallback(cascade, XmNcascadingCallback, (XtCallbackProc)windowMenuCB,
    	    window);
    window->splitWindowItem = createMenuItem(menuPane, "splitWindow",
    	    "Split Window", 'S', splitWindowCB, window);
    XtVaSetValues(window->splitWindowItem, XmNuserData, PERMANENT_MENU_ITEM, 0);
    window->closePaneItem = createMenuItem(menuPane, "closePane",
    	    "Close Pane", 'C', closePaneCB, window);
    XtVaSetValues(window->closePaneItem, XmNuserData, PERMANENT_MENU_ITEM, 0);
    XtSetSensitive(window->closePaneItem, False);
    btn = createMenuSeparator(menuPane, "sep1");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    
    /* 
    ** Create "Help" pull down menu.
    */
    menuPane = XmCreatePulldownMenu(menuBar, "helpMenu", NULL, 0);
    cascade = XtVaCreateManagedWidget("helpMenu", xmCascadeButtonWidgetClass,
    	menuBar, XmNlabelString, st1=XmStringCreateSimple("Help"),
    	XmNsubMenuId, menuPane, 0);
    XtVaSetValues(menuBar, XmNmenuHelpWidget, cascade, 0);
    XmStringFree(st1);
    createMenuItem(menuPane, "gettingStarted", "Getting Started", 'G',
    	    helpStartCB, window);
    createMenuItem(menuPane, "findingReplacingText",
    	    "Finding & Replacing Text", 'F', helpSearchCB, window);
    createMenuItem(menuPane, "selectingText", "Selecting Text", 'S',
    	    helpSelectCB, window);
    createMenuItem(menuPane, "cutPaste", "Cut & Paste", 'C',
    	    helpClipCB, window);
    createMenuItem(menuPane, "featuresForProgramming",
    	    "Features for Programming", 'a', helpProgCB, window);
    createMenuItem(menuPane, "keyboardShortcuts",
    	    "Keyboard Shortcuts", 'K', helpKbdCB, window);
    createMenuItem(menuPane, "crashRecovery", "Crash Recovery", 'R',
    	    helpRecoveryCB, window);
    createMenuItem(menuPane, "preferences", "Preferences", 'P',
    	    helpPrefCB, window);
#ifndef VMS
    createMenuItem(menuPane, "shellCommandsFilters", "Shell Commands/Filters",
    	    'h', helpShellCB, window);
#endif
    createMenuItem(menuPane, "regularExpressions", "Regular Expressions", 'E',
    	    helpRegexCB, window);
    createMenuItem(menuPane, "neditCommandLine", "NEdit Command Line", 'N',
    	    helpCmdLineCB, window);
    createMenuItem(menuPane, "customizingNEdit", "Customizing NEdit", 'u',
    	    helpCustCB, window);
    createMenuSeparator(menuPane, "sep1");
    createMenuItem(menuPane, "version", "Version", 'V',
    	    helpVerCB, window);
    createMenuItem(menuPane, "distributionPolicy", "Distribution Policy", 'D',
    	    helpDistCB, window);
    createMenuItem(menuPane, "mailingList", "Mailing List", 'M',
    	    helpMailingCB, window);
    createMenuItem(menuPane, "problemsBugs", "Problems/Bugs", 'B',
    	    helpBugsCB, window);

    return menuBar;
}

static void newCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    EditNewFile();
    checkCloseDim();
}

static void openCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    char fullname[MAXPATHLEN], filename[MAXPATHLEN], pathname[MAXPATHLEN];
    int response;
    
    response = GetExistingFilename(window->shell, "File to Edit:", fullname);
    if (response == GFN_OK) {
    	ParseFilename(fullname, filename, pathname);
    	EditExistingFile(window, filename, pathname, False);
    }
    checkCloseDim();
}

static void openSelCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData) 
{
    OpenSelectedFile(window, callData->event->xbutton.time);
    checkCloseDim();
}

static void closeCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    CloseFileAndWindow(window);
    checkCloseDim();
}

static void saveCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    SaveWindow(window);
}

static void saveAsCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    SaveWindowAs(window);
}

static void revertCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    RevertToSaved(window, False);
}

static void includeCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    char filename[MAXPATHLEN];
    int response;
    
    while(True) {
	response = GetExistingFilename(window->shell, "File to include:",
		filename);
	if (response == GFN_OK) {
    	    if (IncludeFile(window, filename))
    	    	break;
    	} else
    	    break;
    }
}

static void loadTagsCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    char filename[MAXPATHLEN];
    int response;
    
    while(True) {
	response = GetExistingFilename(window->shell, "ctags file:", filename);
	if (response == GFN_OK)
    	    if (!LoadTagsFile(filename))
    		DialogF(DF_WARN, window->shell, 1,
    	    		"Error reading ctags file,\ntags not loaded", "OK");
    	    else
    	    	break;
    	else
    	    break;
    }
}

static void printCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    PrintWindow(window, False);
}

static void printSelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    PrintWindow(window, True);
}

static void exitCB(Widget w, WindowInfo *window, caddr_t callData) 
{
#ifdef EXIT_WARNING
    int resp, titleLen;
    char exitMsg[DF_MAX_MSG_LENGTH], *ptr, *title;
    WindowInfo *win;
    
    /* If this is the last window, don't ask, just try to close and exit */
    if (window == WindowList && window->next == NULL) {
	if (CloseAllFilesAndWindows())
    	    exit(0);
    	else
    	    return;
    }
    
    /* List the windows being edited and make sure the
       user really wants to exit */
    ptr = exitMsg;
    strcpy(ptr, "Editing:\n"); ptr += 9;
    for (win=WindowList; win!=NULL; win=win->next) {
    	XtVaGetValues(win->shell, XmNtitle, &title, 0);
    	titleLen = strlen(title);
    	if (titleLen > DF_MAX_MSG_LENGTH - 30) {
    	    sprintf(ptr, "   ...\n"); ptr += 7;
    	    break;
    	}
    	sprintf(ptr, "   %s\n", title); ptr += titleLen + 4;
    }
    sprintf(ptr, " \nExit NEdit?");
    resp = DialogF(DF_QUES, window->shell, 2, "%s", "Exit", "Cancel", exitMsg);
    if (resp == 2)
    	return;
#endif /* EXIT_WARNING */

    /* Close all files and exit when the last one is closed */
    if (CloseAllFilesAndWindows())
    	exit(0);
}

static void undoCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    Undo(window);
}

static void redoCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    Redo(window);
}

static void cutCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData) 
{
    CutToClipboard(window, callData->event->xbutton.time);
}

static void copyCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    CopyToClipboard(window, callData->event->xbutton.time);
}

static void pasteCB(Widget w,WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    PasteFromClipboard(window, callData->event->xbutton.time);
}

static void clearCB(Widget w, WindowInfo *window, caddr_t callData)
{
    DeletePrimarySelection(window->textArea);
}

static void selAllCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData)
{
#if XmVersion >= 1002 /* version 1.2 Motif tries to scroll to top or bottom */
    XmTextPosition topPos = XmTextGetTopCharacter(window->textArea);
    
    XmTextDisableRedisplay(window->textArea);
    XmTextSetSelection(window->textArea, 0,
    	    XmTextGetLastPosition(window->textArea),
    	    callData->event->xbutton.time);
    XmTextSetTopCharacter(window->textArea, topPos);
    XmTextEnableRedisplay(window->textArea);
#else
    XmTextSetSelection(window->textArea, 0,
    	    XmTextGetLastPosition(window->textArea),
    	    callData->event->xbutton.time);
#endif
}

static void shiftLeftCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData) 
{
    ShiftSelection(window, SHIFT_LEFT,
    	    callData->event->xbutton.state & ShiftMask, CurrentTime);
}

static void shiftRightCB (Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData)
{
    ShiftSelection(window, SHIFT_RIGHT,
    	    callData->event->xbutton.state & ShiftMask, CurrentTime);
}

static void shiftLeftTabCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    ShiftSelection(window, SHIFT_LEFT, True, CurrentTime);
}

static void shiftRightTabCB (Widget w, WindowInfo *window, caddr_t callData) 
{
    ShiftSelection(window, SHIFT_RIGHT, True, CurrentTime);
}

static void findCB(Widget w, WindowInfo *window,XmAnyCallbackStruct *callData) 
{
    DoFindDlog(window, callData->event->xbutton.state&ShiftMask);
}

static void findSameCB(Widget w, WindowInfo *window,
			XmAnyCallbackStruct *callData) 
{
    SearchAndSelect(window, callData->event->xbutton.state&ShiftMask,
    			 callData->event->xbutton.time);
}

static void findSelCB(Widget w, WindowInfo *window,
		       XmAnyCallbackStruct *callData) 
{
    SearchForSelected(window, callData->event->xbutton.state&ShiftMask,
    			   callData->event->xbutton.time);
}

static void replaceCB(Widget w, WindowInfo *window,
		       XmAnyCallbackStruct *callData) 
{
   DoReplaceDlog(window, callData->event->xbutton.state&ShiftMask);
}

static void replaceSameCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData) 
{
    SearchAndReplace(window, callData->event->xbutton.state&ShiftMask);
}

static void gotoCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData) 
{
    GotoLineNumber(window, callData->event->xbutton.time);
}

static void gotoSelectedCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData) 
{
    GotoSelectedLineNumber(window, callData->event->xbutton.time);
}

static void findMatchingCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData) 
{
    MatchSelectedCharacter(window, CurrentTime);
}

static void findDefCB(Widget w, WindowInfo *window,
			   XmAnyCallbackStruct *callData) 
{
    FindDefinition(window, CurrentTime);
}

static void overstrikeCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SetOverstrike(window, XmToggleButtonGetState(w));
}

static void autoIndentCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SetAutoIndent(window, XmToggleButtonGetState(w));
}

static void autoSaveCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->autoSave = XmToggleButtonGetState(w);
}

static void fontCB(Widget w, WindowInfo *window, caddr_t callData)
{
    char *fontName;
    
    fontName = FontSel(window->shell, PREF_FIXED, window->fontName);
    if (fontName == NULL)
    	return;
    SetFontByName(window, fontName);
    XtFree(fontName);
}

static void wrapCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->wrap = XmToggleButtonGetState(w);
}

static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->showMatching = XmToggleButtonGetState(w);
}

static void tabDistCB(Widget w, WindowInfo *window, caddr_t callData)
{
    char numText[DF_MAX_PROMPT_LENGTH];
    int num, nRead, resp;

    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Number of characters between tab stops", numText, "OK", "Cancel");
    if (resp == 2)
    	return;
    nRead = sscanf(numText, "%d", &num);
    if (nRead != 1) {
    	XBell(TheDisplay, 100);
	return;
    }
    SetTabDistance(window, num);
}

static void statsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ShowStatsLine(window, XmToggleButtonGetState(w));
}

static void autoIndentDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefAutoIndent(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->autoIndentDefItem, state, False);
}

static void autoSaveDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefAutoSave(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->autoSaveDefItem, state, False);
}

static void fontDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    char *fontName;
    
    fontName = FontSel(window->shell, PREF_FIXED, GetPrefFontName());
    if (fontName == NULL)
    	return;
    SetPrefFont(fontName);
    XtFree(fontName);
}

static void wrapDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefWrap(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->wrapTextDefItem, state, False);
}

static void tabDistDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    char numText[DF_MAX_PROMPT_LENGTH];
    int num, nRead, resp;

    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Number of characters between tab stops", numText, "OK", "Cancel");
    if (resp == 2)
    	return;
    nRead = sscanf(numText, "%d", &num);
    if (nRead != 1) {
    	XBell(TheDisplay, 100);
	return;
    }
    SetPrefTabDist(num);
}

static void showMatchingDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefShowMatching(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->showMatchingDefItem, state, False);
}

#ifndef VMS
static void filterDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditShellMenu(window);
}
#endif /* VMS */

static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefSearchDlogs(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->searchDlogsDefItem, state, False);
}

static void reposDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefRepositionDialogs(state);
    SetPointerCenteredDialogs(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->reposDlogsDefItem, state, False);
}

static void statsLineDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefStatsLine(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->statsLineDefItem, state, False);
}

static void searchLiteralCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;

    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_LITERAL);
    	for (win=WindowList; win!=NULL; win=win->next){
    	    XmToggleButtonSetState(win->searchLiteralDefItem, True, False);
    	    XmToggleButtonSetState(win->searchCaseSenseDefItem, False, False);
    	    XmToggleButtonSetState(win->searchRegexDefItem, False, False);
    	}
    }
}

static void searchCaseSenseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;

    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_CASE_SENSE);
    	for (win=WindowList; win!=NULL; win=win->next) {
    	    XmToggleButtonSetState(win->searchLiteralDefItem, False, False);
    	    XmToggleButtonSetState(win->searchCaseSenseDefItem, True, False);
    	    XmToggleButtonSetState(win->searchRegexDefItem, False, False);
    	}
    }
}

static void searchRegexCB(Widget w, WindowInfo *window, caddr_t callData)
{
   WindowInfo *win;

    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_REGEX);
    	for (win=WindowList; win!=NULL; win=win->next){
    	    XmToggleButtonSetState(win->searchLiteralDefItem, False, False);
    	    XmToggleButtonSetState(win->searchCaseSenseDefItem, False, False);
    	    XmToggleButtonSetState(win->searchRegexDefItem, True, False);
    	}
    }
}

static void size24x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(24, 80);
}

static void size40x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(40, 80);
}

static void size60x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(60, 80);
}

static void size80x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(80, 80);
}

static void sizeCustomCB(Widget w, WindowInfo *window, caddr_t callData)
{
    RowColumnPrefDialog(window->shell);
    updateWindowSizeMenus();
}

static void savePrefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SaveNEditPrefs(window->shell);
}

static void splitWindowCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SplitWindow(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void closePaneCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ClosePane(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void capitalizeCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData)
{
    UpcaseSelection(window);
}

static void lowercaseCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData)
{
    DowncaseSelection(window);
}

static void fillCB(Widget w, WindowInfo *window, caddr_t callData)
{
    FillSelection(window);
}

static void formFeedCB(Widget w, WindowInfo *window, caddr_t callData)
{
    XmTextPosition textPos = XmTextGetInsertionPosition(window->lastFocus);
    
    XmTextInsert(window->lastFocus,textPos, "\f");
    XmTextSetInsertionPosition(window->lastFocus, textPos+1);
}

static void controlCB(Widget w, WindowInfo *window, caddr_t callData)
{
    char charCodeString[2], charCodeText[DF_MAX_PROMPT_LENGTH];
    int charCode, nRead, response;
    XmTextPosition textPos = XmTextGetInsertionPosition(window->lastFocus);
    
    response = DialogF(DF_PROMPT, window->shell, 2,
    	    "ASCII Character Code (decimal):", charCodeText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(charCodeText, "%d", &charCode);
    if (nRead != 1 || charCode <=0 || charCode >= 256) {
    	XBell(TheDisplay, 100);
	return;
    }
    charCodeString[0] = (char)charCode; charCodeString[1] = '\0';
    XmTextInsert(window->lastFocus, textPos, charCodeString);
    XmTextSetInsertionPosition(window->lastFocus, textPos+1);
}

#ifndef VMS
static void shellFilterCB(Widget w, WindowInfo *window, caddr_t callData)
{
    FilterSelection(window);
}

static void execCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ExecShellCommand(window);
}

static void execLineCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ExecCursorLine(window);
}
#endif

static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_START);
}

static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_SEARCH);
}

static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_SELECT);
}

static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_CLIPBOARD);
}

static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_PROGRAMMER);
}

static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_KEYBOARD);
}

static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_RECOVERY);
}

static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_PREFERENCES);
}

static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_SHELL);
}

static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_REGEX);
}

static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_COMMAND_LINE);
}

static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_CUSTOMIZE);
}

static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_VERSION);
}

static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_DISTRIBUTION);
}

static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_MAILING_LIST);
}

static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window, HELP_BUGS);
}

static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData)
{
    if (!window->windowMenuValid) {
    	updateWindowMenu(window);
    	window->windowMenuValid = True;
    }
}

/*
** Same as AddSubMenu from libNUtil.a without setting the mnemonic (NEdit
** users like to be able to re-arrange the mnemonics so they can set Alt
** key combinations as accelerators).
*/
static Widget createMenu(Widget parent, char *name, char *label)
{
    Widget menu;
    XmString st1;
    
    menu = XmCreatePulldownMenu(parent, name, NULL, 0);
    XtVaCreateManagedWidget(name, xmCascadeButtonWidgetClass, parent, 
    	XmNlabelString, st1=XmStringCreateSimple(label),
    	XmNsubMenuId, menu, 0);
    XmStringFree(st1);
    return menu;
}

/*
** Same as AddMenuItem from libNUtil.a without setting the accelerator
** (these are set in the fallback app-defaults so users can change them).
*/
static Widget createMenuItem(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg)
{
    Widget button;
    XmString st1;
    
    button = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent, 
    	    XmNlabelString, st1=XmStringCreateSimple(label),
    	    XmNmnemonic, mnemonic, NULL);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)callback, cbArg);
    XmStringFree(st1);
    return button;
}

/*
** "fake" menu items allow accelerators to be attached, but don't show up
** in the menu.  They are necessary to process the shifted menu items because
** Motif does not properly process the event descriptions in accelerator
** resources, and you can't specify "shift key is optional"
*/
static Widget createFakeMenuItem(Widget parent, char *name,
	menuCallbackProc callback, void *cbArg)
{
    Widget button;
    XmString st1;
    
    button = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,
    	    XmNlabelString, st1=XmStringCreateSimple(""),
    	    XmNshadowThickness, 0,
    	    XmNmarginHeight, 0,
    	    XmNheight, 0, 0);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)callback, cbArg);
    XmStringFree(st1);
    XtVaSetValues(button, XmNtraversalOn, False, 0);

    return button;
}

/*
** Add a toggle button item to an already established pull-down or pop-up
** menu, including mnemonics, accelerators and callbacks.
*/
static Widget createMenuToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set)
{
    Widget button;
    XmString st1, st2;
    
    button = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent, 
    	    XmNlabelString, st1=XmStringCreateSimple(label),
    	    XmNmnemonic, mnemonic,
    	    XmNset, set, NULL);
    XtAddCallback(button, XmNvalueChangedCallback, (XtCallbackProc)callback,
    	    cbArg);
    XmStringFree(st1);
    return button;
}

static Widget createMenuSeparator(Widget parent, char *name)
{
    Widget button;
    
    button = XmCreateSeparator(parent, name, NULL, 0);
    XtManageChild(button);
    return button;
}

/*
** Make sure the close menu item is dimmed appropriately for the current
** set of windows.  It should be dim only for the last Untitled, unmodified,
** editor window, and sensitive otherwise.
*/
static void checkCloseDim(void)
{
    WindowInfo *window;
    
    if (WindowList == NULL)
    	return;
    if (WindowList->next==NULL &&
    	    !WindowList->filenameSet && !WindowList->fileChanged) {
    	XtSetSensitive(WindowList->closeItem, FALSE);
    	return;
    }
    
    for (window=WindowList; window!=NULL; window=window->next)
    	XtSetSensitive(window->closeItem, True);
}

/*
** Invalidate the Window menus of all NEdit windows to but don't change
** the menus until they're needed (Originally, this was "UpdateWindowMenus",
** but creating and destroying manu items for every window every time a
** new window was created or something changed, made things move very
** slowly with more than 10 or so windows).
*/
void InvalidateWindowMenus()
{
    WindowInfo *w;

    /* Mark the window menus invalid (to be updated when the user pulls one
       down), unless the menu is torn off, meaning it is visible to the user
       and should be updated immediately */
    for (w=WindowList; w!=NULL; w=w->next) {
    	if (!XmIsMenuShell(XtParent(w->windowMenuPane)))
    	    updateWindowMenu(w);
    	else
    	    w->windowMenuValid = False;
    }
}

/*
** Update the Window menu of a single window to reflect the current state of
** all NEdit windows as determined by the global WindowList.
*/
static void updateWindowMenu(WindowInfo *window)
{
    WindowInfo *w;
    char *title;
    Widget btn;
    WidgetList items;
    int nItems, n, userData;
    XmString st1;
    
    /* While it is not possible on some systems (ibm at least) to substitute
       a new menu pane, it is possible to substitute menu items, as long as
       at least one remains in the menu at all times. This routine assumes
       that the menu contains permanent items marked with the value PERMANENT
       _MENU_ITEM in the userData resource, and adds and removes items which
       it marks with the value TEMPORARY_MENU_ITEM */
    
    /* Go thru all of the items in the menu and rename them to
       match the window list.  Delete any extras */
    XtVaGetValues(window->windowMenuPane, XmNchildren, &items,
    	    XmNnumChildren, &nItems,0);
    w = WindowList;
    for (n=0; n<nItems; n++) {
    	XtVaGetValues(items[n], XmNuserData, &userData, 0);
    	if (userData == TEMPORARY_MENU_ITEM) {
	    if (w == NULL) {
    		/* unmanaging before destroying stops parent from displaying */
    		XtUnmanageChild(items[n]);
    		XtDestroyWidget(items[n]);	    	
	    } else {
		XtVaGetValues(w->shell, XmNtitle, &title, 0);
		XtVaSetValues(items[n], XmNlabelString,
    	    		st1=XmStringCreateSimple(title), 0);
		XtRemoveAllCallbacks(items[n], XmNactivateCallback);
		XtAddCallback(items[n], XmNactivateCallback,
			(XtCallbackProc)raiseCB, w);
	    	XmStringFree(st1);
		w = w->next;
	    }
	}
    }
    
    /* Add new items for the titles of the remaining windows to the menu */
    for (; w!=NULL; w=w->next) {
    	XtVaGetValues(w->shell, XmNtitle, &title, 0);
    	btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
    		window->windowMenuPane, 
    		XmNlabelString, st1=XmStringCreateSimple(title),
    		XmNuserData, TEMPORARY_MENU_ITEM, NULL);
	XtAddCallback(btn, XmNactivateCallback, (XtCallbackProc)raiseCB, w);
    	XmStringFree(st1);
    }
}

static void setWindowSizeDefault(int rows, int cols)
{
    SetPrefRows(rows);
    SetPrefCols(cols);
    updateWindowSizeMenus();
}

static void updateWindowSizeMenus()
{
    WindowInfo *win;
    
    for (win=WindowList; win!=NULL; win=win->next)
    	updateWindowSizeMenu(win);
}

static void updateWindowSizeMenu(WindowInfo *win)
{
    int rows = GetPrefRows(), cols = GetPrefCols();
    char title[50];
    XmString st1;
    
    XmToggleButtonSetState(win->size24x80DefItem, rows==24&&cols==80,False);
    XmToggleButtonSetState(win->size40x80DefItem, rows==40&&cols==80,False);
    XmToggleButtonSetState(win->size60x80DefItem, rows==60&&cols==80,False);
    XmToggleButtonSetState(win->size80x80DefItem, rows==80&&cols==80,False);
    if ((rows!=24 && rows!=40 && rows!=60 && rows!=80) || cols!=80) {
    	XmToggleButtonSetState(win->sizeCustomDefItem, True, False);
    	sprintf(title, "Custom... (%d x %d)", rows, cols);
    	XtVaSetValues(win->sizeCustomDefItem,
    	    	XmNlabelString, st1=XmStringCreateSimple(title), 0);
    	XmStringFree(st1);
    } else {
    	XmToggleButtonSetState(win->sizeCustomDefItem, False, False);
    	XtVaSetValues(win->sizeCustomDefItem,
    	    	XmNlabelString, st1=XmStringCreateSimple("Custom..."), 0);
    	XmStringFree(st1);
    }
}

static void raiseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* XMapRaised as opposed to XRaiseWindow will uniconify as well as raise */
    XMapRaised(TheDisplay, XtWindow(window->shell));
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
}

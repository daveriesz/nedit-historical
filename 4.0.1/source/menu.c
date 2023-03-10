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
#include <string.h>
#include <ctype.h>
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
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "file.h"
#include "menu.h"
#include "window.h"
#include "search.h"
#include "selection.h"
#include "undo.h"
#include "shift.h"
#include "help.h"
#include "preferences.h"
#include "tags.h"
#include "userCmds.h"
#include "shell.h"
#include "macro.h"

typedef void (*menuCallbackProc)();

static void doActionCB(Widget w, XtPointer clientData, XtPointer callData);
static void readOnlyCB(Widget w, XtPointer clientData, XtPointer callData);
static void pasteColCB(Widget w, XtPointer clientData, XtPointer callData); 
static void shiftLeftCB(Widget w, XtPointer clientData, XtPointer callData);
static void shiftRightCB(Widget w, XtPointer clientData, XtPointer callData);
static void findCB(Widget w, XtPointer clientData, XtPointer callData);
static void findSameCB(Widget w, XtPointer clientData, XtPointer callData);
static void findSelCB(Widget w, XtPointer clientData, XtPointer callData);
static void replaceCB(Widget w, XtPointer clientData, XtPointer callData);
static void replaceSameCB(Widget w, XtPointer clientData, XtPointer callData);
static void markCB(Widget w, XtPointer clientData, XtPointer callData);
static void gotoMarkCB(Widget w, XtPointer clientData, XtPointer callData);
static void overstrikeCB(Widget w, WindowInfo *window, XtPointer callData);
static void autoIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void preserveCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapMarginCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabsCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void preserveDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapMarginDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsLineDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void shellDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void macroDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void keepSearchDlogsDefCB(Widget w, WindowInfo *window,
	caddr_t callData);
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
static void formFeedCB(Widget w, XtPointer clientData, XtPointer callData);
static void cancelShellCB(Widget w, XtPointer clientData, XtPointer callData);
static void learnCB(Widget w, WindowInfo *window, caddr_t callData);
static void finishLearnCB(Widget w, WindowInfo *window, caddr_t callData);
static void cancelLearnCB(Widget w, WindowInfo *window, caddr_t callData);
static void replayCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMouseCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpFillCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpServerCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMacroCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpResourcesCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpResourcesCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpActionsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData);
static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData);
static void newAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void openDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void openAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void openSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void closeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void saveAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void saveAsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void saveAsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void revertAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void includeDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void includeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void loadTagsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void loadTagsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void printAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void printSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void exitAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void undoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void redoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void clearAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void selAllAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shiftLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shiftLeftTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shiftRightAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shiftRightTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void findSameAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void findSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void replaceDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void replaceAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceInSelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void gotoDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void markAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void markDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoMarkAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void gotoMarkDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findDefAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void splitWindowAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void closePaneAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void capitalizeAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void lowercaseAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void fillAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void controlDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
#ifndef VMS
static void filterDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shellFilterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void execDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void execAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void execLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shellMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
#endif
static void macroMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void beginningOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void endOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static Widget createMenu(Widget parent, char *name, char *label);
static Widget createMenuItem(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg);
static Widget createFakeMenuItem(Widget parent, char *name,
	menuCallbackProc callback, void *cbArg);
static Widget createMenuToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set);
static Widget createMenuSeparator(Widget parent, char *name);
static void updateWindowMenu(WindowInfo *window);
static int searchDirection(int ignoreArgs, String *args, Cardinal *nArgs);
static int searchType(int ignoreArgs, String *args, Cardinal *nArgs);
static char **shiftKeyToDir(XtPointer callData);
static void raiseCB(Widget w, WindowInfo *window, caddr_t callData);
static void setWindowSizeDefault(int rows, int cols);
static void updateWindowSizeMenus();
static void updateWindowSizeMenu(WindowInfo *win);
static int strCaseCmp(char *str1, char *str2);

/* Application action table */
XtActionsRec Actions[] = {
    {"new", newAP},
    {"open", openAP},
    {"open-dialog", openDialogAP},
    {"open-selected", openSelectedAP},
    {"close", closeAP},
    {"save", saveAP},
    {"save-as", saveAsAP},
    {"save-as-dialog", saveAsDialogAP},
    {"revert-to-saved", revertAP},
    {"include-file", includeAP},
    {"include-file-dialog", includeDialogAP},
    {"load-tags-file", loadTagsAP},
    {"load-tags-file-dialog", loadTagsDialogAP},
    {"print", printAP},
    {"print-selection", printSelAP},
    {"exit", exitAP},
    {"undo", undoAP},
    {"redo", redoAP},
    {"clear", clearAP},
    {"select-all", selAllAP},
    {"shift-left", shiftLeftAP},
    {"shift-left-by-tab", shiftLeftTabAP},
    {"shift-right", shiftRightAP},
    {"shift-right-by-tab", shiftRightTabAP},
    {"find", findAP},
    {"find-dialog", findDialogAP},
    {"find-same", findSameAP},
    {"find-selection", findSelAP},
    {"replace", replaceAP},
    {"replace-dialog", replaceDialogAP},
    {"replace-all", replaceAllAP},
    {"replace-in-selection", replaceInSelAP},
    {"replace-same", replaceSameAP},
    {"goto-line-number", gotoAP},
    {"goto-line-number-dialog", gotoDialogAP},
    {"goto-selected", gotoSelectedAP},
    {"mark", markAP},
    {"mark-dialog", markDialogAP},
    {"goto-mark", gotoMarkAP},
    {"goto-mark-dialog", gotoMarkDialogAP},
    {"match", findMatchingAP},
    {"find-definition", findDefAP},
    {"split-window", splitWindowAP},
    {"close-pane", closePaneAP},
    {"capitalize", capitalizeAP},
    {"lowercase", lowercaseAP},
    {"fill-paragraph", fillAP},
    {"control-code-dialog", controlDialogAP},
#ifndef VMS
    {"filter-selection-dialog", filterDialogAP},
    {"filter-selection", shellFilterAP},
    {"execute-command", execAP},
    {"execute-command-dialog", execDialogAP},
    {"execute-command-line", execLineAP},
    {"shell-menu-command", shellMenuAP},
#endif /*VMS*/
    {"macro-menu-command", macroMenuAP},
    {"beginning-of-selection", beginningOfSelectionAP},
    {"end-of-selection", endOfSelectionAP},
};

/*
** Install actions for use in translation tables and macro recording, relating
** to menu item commands
*/
void InstallMenuActions(XtAppContext context)
{
    XtAppAddActions(context, Actions, XtNumber(Actions));
}

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
    createMenuItem(menuPane, "new", "New", 'N', doActionCB, "new");
    createMenuItem(menuPane, "open", "Open...", 'O', doActionCB, "open-dialog");
    createMenuItem(menuPane, "openSelected", "Open Selected", 'd',
    	    doActionCB, "open-selected");
    window->readOnlyItem = createMenuToggle(menuPane, "readOnly", "Read Only",
    	    'e', readOnlyCB, window, window->lockWrite);
    createMenuSeparator(menuPane, "sep1");
    window->closeItem = createMenuItem(menuPane, "close", "Close", 'C',
    	    doActionCB, "close");
    CheckCloseDim();
    createMenuItem(menuPane, "save", "Save", 'S', doActionCB, "save");
    createMenuItem(menuPane, "saveAs", "Save As...", 'A', doActionCB,
    	    "save-as-dialog");
    createMenuItem(menuPane, "revertToSaved", "Revert to Saved", 'R',
    	    doActionCB, "revert-to-saved");
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "includeFile", "Include File...", 'I',
    	    doActionCB, "include-file-dialog");
    createMenuItem(menuPane, "loadTagsFile", "Load Tags File...", 'L',
    	    doActionCB, "load-tags-file-dialog");
    createMenuSeparator(menuPane, "sep3");
    createMenuItem(menuPane, "print", "Print...", 'P', doActionCB, "print");
    window->printSelItem = createMenuItem(menuPane, "printSelection",
    	    "Print Selection...", 't', doActionCB, "print-selection");
    XtSetSensitive(window->printSelItem, window->wasSelected);
    createMenuSeparator(menuPane, "sep4");
    createMenuItem(menuPane, "exit", "Exit", 'x', doActionCB, "exit");

    /* 
    ** Create "Edit" pull down menu.
    */
    menuPane = createMenu(menuBar, "editMenu", "Edit");
    window->undoItem = createMenuItem(menuPane, "undo", "Undo", 'U',
    	    doActionCB, "undo");
    XtSetSensitive(window->undoItem, False);
    window->redoItem = createMenuItem(menuPane, "redo", "Redo", 'd',
    	    doActionCB, "redo");
    XtSetSensitive(window->redoItem, False);
    createMenuSeparator(menuPane, "sep1");
    window->cutItem = createMenuItem(menuPane, "cut", "Cut", 't', doActionCB,
    	    "cut-clipboard");
    XtSetSensitive(window->cutItem, window->wasSelected);
    window->copyItem = createMenuItem(menuPane, "copy", "Copy", 'C', doActionCB,
    	    "copy-clipboard");
    XtSetSensitive(window->copyItem, window->wasSelected);
    createMenuItem(menuPane, "paste", "Paste", 'P', doActionCB,
    	    "paste-clipboard");
    createMenuItem(menuPane, "pasteColumn", "Paste Column", 'a', pasteColCB,
    	    window);
    createMenuItem(menuPane, "clear", "Clear", 'e', doActionCB, "clear");
    createMenuItem(menuPane, "selectAll", "Select All", 'S', doActionCB,
    	    "select-all");
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "shiftLeft", "Shift Left", 'L',
    	    shiftLeftCB, window);
    createFakeMenuItem(menuPane, "shiftLeftShift", shiftLeftCB, window);
    createMenuItem(menuPane, "shiftRight", "Shift Right", 'R',
    	    shiftRightCB, window);
    createFakeMenuItem(menuPane, "shiftRightShift", shiftRightCB, window);
    createMenuItem(menuPane, "capitalize", "Capitalize", 'z',
    	    doActionCB, "capitalize");
    createMenuItem(menuPane, "lowerCase", "Lower-case", 'o',
    	    doActionCB, "lowercase");
    createMenuItem(menuPane, "fillParagraph", "Fill Paragraph", 'F',
    	    doActionCB, "fill-paragraph");
    createMenuSeparator(menuPane, "sep3");
    createMenuItem(menuPane, "insertFormFeed", "Insert Form Feed", 'I',
    	    formFeedCB, window);
    createMenuItem(menuPane, "insControlCode", "Ins. Control Code", 'n',
    	    doActionCB, "control-code-dialog");

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
    	    doActionCB, "goto-line-number-dialog");
    createMenuItem(menuPane, "gotoSelected", "Goto Selected", 'G',
    	    doActionCB, "goto-selected");
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "mark", "Mark", 'k', markCB, window);
    createMenuItem(menuPane, "gotoMark", "Goto Mark", 'k', gotoMarkCB, window);
    createMenuSeparator(menuPane, "sep3");
    createMenuItem(menuPane, "match", "Match (..)", 'M',
    	    doActionCB, "match");
    window->findDefItem = createMenuItem(menuPane, "findDefinition",
    	    "Find Definition", 'D', doActionCB, "find-definition");
    XtSetSensitive(window->findDefItem, TagsFileLoaded());
    
    /*
    ** Create the Preferences menu
    */
    menuPane = createMenu(menuBar, "preferencesMenu", "Preferences");
    createMenuToggle(menuPane, "autoIndent", "Auto Indent", 'A',
    	    autoIndentCB, window, window->autoIndent);
    createMenuToggle(menuPane, "autoWrap", "Auto Wrap", 'W',
    	    wrapCB, window, window->wrap);
    createMenuItem(menuPane, "wrapMargin", "Wrap Margin...", 'r',
    	    wrapMarginCB, window);
#ifndef VMS
    window->saveLastItem = createMenuToggle(menuPane, "preserveLastVersion",
    	    "Preserve Last Version", 'P', preserveCB, window,
    	    window->saveOldVersion);
#endif
    window->autoSaveItem = createMenuToggle(menuPane, "incrementalBackup",
    	    "Incremental Backup", 'B', autoSaveCB, window, window->autoSave);
    createMenuToggle(menuPane, "showMatching", "Show Matching (..)", 'M',
    	    showMatchingCB, window, window->showMatching);
#ifndef IBM_DESTROY_BUG
    createMenuItem(menuPane, "textFont", "Text Font...", 'F', fontCB, window);
#endif /*IBM_DESTROY_BUG*/
    createMenuItem(menuPane, "tabs", "Tabs...", 'T', tabsCB, window);
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
    createMenuItem(subPane, "wrapMargin", "Wrap Margin...", 'r',
    	    wrapMarginDefCB, window);
    window->saveLastDefItem = createMenuToggle(subPane, "preserveLastVersion",
    	    "Preserve Last Version", 'P', preserveDefCB, window,
    	    GetPrefSaveOldVersion());
    window->autoSaveDefItem = createMenuToggle(subPane, "incrementalBackup",
    	    "Incremental Backup", 'B', autoSaveDefCB, window, GetPrefAutoSave());
    window->showMatchingDefItem = createMenuToggle(subPane, "showMatching",
    	    "Show Matching (..)", 'M', showMatchingDefCB, window,
    	    GetPrefShowMatching());
    createMenuItem(subPane, "textFont", "Text Font...", 'F', fontDefCB, window);
    createMenuItem(subPane, "tabDistance", "Tabs...", 'T', tabsDefCB, window);
    window->statsLineDefItem = createMenuToggle(subPane, "statisticsLine",
    	    "Statistics Line", 'S', statsLineDefCB, window, GetPrefStatsLine());
#ifndef VMS
    createMenuItem(subPane, "shellCommands", "Shell Commands...", 'l',
    	    shellDefCB, window);
#endif
    createMenuItem(subPane, "macroCommands", "Macro Commands...", 'c',
    	    macroDefCB, window);
    window->searchDlogsDefItem = createMenuToggle(subPane, "verboseSearch",
    	    "Verbose Search", 'V', searchDlogsDefCB, window,
    	    GetPrefSearchDlogs());
    window->keepSearchDlogsDefItem = createMenuToggle(subPane,
    	    "keepSearchDialogsUp", "Keep Search Dialogs Up", 'K',
    	    keepSearchDlogsDefCB, window, GetPrefKeepSearchDlogs());
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
    menuPane = window->shellMenuPane =
    	    createMenu(menuBar, "shellMenu", "Shell");
    window->filterItem = createMenuItem(menuPane, "filterSelection",
    	    "Filter Selection...", 'F', doActionCB, "filter-selection-dialog");
    XtVaSetValues(window->filterItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, window->wasSelected, 0);
    btn = createMenuItem(menuPane, "executeCommand", "Execute Command...",
    	    'E', doActionCB, "execute-command-dialog");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    btn = createMenuItem(menuPane, "executeCommandLine", "Execute Command Line",
    	    'x', doActionCB, "execute-command-line");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    window->cancelShellItem = createMenuItem(menuPane, "cancelShellCommand",
    	    "Cancel Shell Command", 'C', cancelShellCB, NULL);
    XtVaSetValues(window->cancelShellItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, 0);
    btn = createMenuSeparator(menuPane, "sep1");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    UpdateShellMenu(window);
#endif

    /*
    ** Create the Macro menu
    */
    menuPane = window->macroMenuPane =
    	    createMenu(menuBar, "macroMenu", "Macro");
    window->learnItem = createMenuItem(menuPane, "learnKeystrokes",
    	    "Learn Keystrokes", 'L', learnCB, window);
    XtVaSetValues(window->learnItem , XmNuserData, PERMANENT_MENU_ITEM, 0);
    window->finishLearnItem = createMenuItem(menuPane, "finishLearn",
    	    "Finish Learn", 'F', finishLearnCB, window);
    XtVaSetValues(window->finishLearnItem , XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, 0);
    window->cancelLearnItem = createMenuItem(menuPane, "cancelLearn",
    	    "Cancel Learn", 'C', cancelLearnCB, window);
    XtVaSetValues(window->cancelLearnItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, 0);
    window->replayItem = createMenuItem(menuPane, "replayKeystrokes",
    	    "Replay Keystrokes", 'R', replayCB, window);
    XtVaSetValues(window->replayItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, 0);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    btn = createMenuSeparator(menuPane, "sep1");
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, 0);
    UpdateMacroMenu(window);

    /*
    ** Create the Windows menu
    */
    menuPane = window->windowMenuPane =
    	    XmCreatePulldownMenu(menuBar, "windowsMenu", NULL, 0);
    cascade = XtVaCreateManagedWidget("windowsMenu", xmCascadeButtonWidgetClass,
    	    menuBar, XmNlabelString, st1=XmStringCreateSimple("Windows"),
    	    XmNsubMenuId, menuPane, 0);	    
    XmStringFree(st1);
    XtAddCallback(cascade, XmNcascadingCallback, (XtCallbackProc)windowMenuCB,
    	    window);
    window->splitWindowItem = createMenuItem(menuPane, "splitWindow",
    	    "Split Window", 'S', doActionCB, "split-window");
    XtVaSetValues(window->splitWindowItem, XmNuserData, PERMANENT_MENU_ITEM, 0);
    window->closePaneItem = createMenuItem(menuPane, "closePane",
    	    "Close Pane", 'C', doActionCB, "close-pane");
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
    createMenuItem(menuPane, "selectingText", "Selecting Text", 'S',
    	    helpSelectCB, window);
    createMenuItem(menuPane, "findingReplacingText",
    	    "Finding and Replacing Text", 'F', helpSearchCB, window);
    createMenuItem(menuPane, "cutPaste", "Cut and Paste", 'C',
    	    helpClipCB, window);
    createMenuItem(menuPane, "usingTheMouse",
    	    "Using the Mouse", 'U', helpMouseCB, window);
    createMenuItem(menuPane, "keyboardShortcuts",
    	    "Keyboard Shortcuts", 'K', helpKbdCB, window);
    createMenuItem(menuPane, "shiftingAndFilling",
    	    "Shifting and Filling", 'i', helpFillCB, window);
    createMenuItem(menuPane, "featuresForProgramming",
    	    "Features for Programming", 'a', helpProgCB, window);
    createMenuItem(menuPane, "crashRecovery", "Crash Recovery", 'R',
    	    helpRecoveryCB, window);
#ifndef VMS
    createMenuItem(menuPane, "shellCommandsFilters", "Shell Commands/Filters",
    	    'h', helpShellCB, window);
#endif
    createMenuItem(menuPane, "macros", "Macros, Learn/Replay", 'M',
    	    helpMacroCB, window);
    createMenuItem(menuPane, "regularExpressions", "Regular Expressions", 'E',
    	    helpRegexCB, window);
    createMenuItem(menuPane, "neditCommandLine", "NEdit Command Line", 'N',
    	    helpCmdLineCB, window);
    createMenuItem(menuPane, "serverModeAndNc", "Server Mode and nc", 'o',
    	    helpServerCB, window);
    createMenuSeparator(menuPane, "sep1");
    createMenuItem(menuPane, "customizingNEdit", "Customizing NEdit", 'z',
    	    helpCustCB, window);
    createMenuItem(menuPane, "preferences", "Preferences", 'P',
    	    helpPrefCB, window);
    createMenuItem(menuPane, "xResources", "X Resources", 'X',
    	    helpResourcesCB, window);
    createMenuItem(menuPane, "action routines", "Action Routines", 't',
    	    helpActionsCB, window);
    createMenuSeparator(menuPane, "sep2");
    createMenuItem(menuPane, "version", "Version", 'V',
    	    helpVerCB, window);
    createMenuItem(menuPane, "distributionPolicy", "Distribution Policy", 'D',
    	    helpDistCB, window);
    createMenuItem(menuPane, "mailingLists", "Mailing Lists", 'L',
    	    helpMailingCB, window);
    createMenuItem(menuPane, "problemsBugs", "Problems/Bugs", 'B',
    	    helpBugsCB, window);

    return menuBar;
}

static void doActionCB(Widget w, XtPointer clientData, XtPointer callData)
{
#if XmVersion >= 1002
    Widget menu = XmGetPostedFromWidget(XtParent(w));
#else
    Widget menu = w;
#endif

    XtCallActionProc(WidgetToWindow(menu)->lastFocus, (char *)clientData,
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static void readOnlyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    
    window->lockWrite = XmToggleButtonGetState(w);
    UpdateWindowTitle(window);
    UpdateWindowReadOnly(window);
}

static void pasteColCB(Widget w, XtPointer clientData, XtPointer callData) 
{
    static char *params[1] = {"rect"};
    
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "paste-clipboard",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

static void shiftLeftCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus,
    	    ((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask
    	    ? "shift-left-by-tab" : "shift-left",
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static void shiftRightCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus,
    	    ((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask
    	    ? "shift-right-by-tab" : "shift-right",
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static void findCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find-dialog",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void findSameCB(Widget w, XtPointer clientData, XtPointer callData)
{
     XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find-same",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void findSelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find-selection",
    	    ((XmAnyCallbackStruct *)callData)->event, 
    	    shiftKeyToDir(callData), 1);
}

static void replaceCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "replace-dialog",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void replaceSameCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "replace-same",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void markCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XEvent *event = ((XmAnyCallbackStruct *)callData)->event;
    WindowInfo *window = (WindowInfo *)clientData;
    
    if (event->type == KeyPress)
    	BeginMarkCommand(window);
    else
    	XtCallActionProc(window->lastFocus, "mark-dialog", event, NULL, 0);
}

static void gotoMarkCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XEvent *event = ((XmAnyCallbackStruct *)callData)->event;
    WindowInfo *window = (WindowInfo *)clientData;
    
    if (event->type == KeyPress)
    	BeginGotoMarkCommand(window);
    else
    	XtCallActionProc(window->lastFocus, "goto-mark-dialog", event, NULL, 0);
}

static void overstrikeCB(Widget w, WindowInfo *window, XtPointer callData)
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

static void preserveCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->saveOldVersion = XmToggleButtonGetState(w);
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
    SetAutoWrap(window, XmToggleButtonGetState(w));
}

static void wrapMarginCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WrapMarginDialog(window->shell, window);
}

static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->showMatching = XmToggleButtonGetState(w);
}

static void tabsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    TabsPrefDialog(window->shell, window);
}

static void statsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    window->showStats = XmToggleButtonGetState(w);
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

static void preserveDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefSaveOldVersion(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->saveLastDefItem, state, False);
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

static void wrapMarginDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WrapMarginDialog(window->shell, NULL);
}

static void tabsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    TabsPrefDialog(window->shell, NULL);
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
static void shellDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditShellMenu(window);
}
#endif /* VMS */

static void macroDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditMacroMenu(window);
}

static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefSearchDlogs(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->searchDlogsDefItem, state, False);
}

static void keepSearchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
    SetPrefKeepSearchDlogs(state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->keepSearchDlogsDefItem, state, False);
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

static void formFeedCB(Widget w, XtPointer clientData, XtPointer callData)
{
    static char *params[1] = {"\f"};
    
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "insert-string",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

static void cancelShellCB(Widget w, XtPointer clientData, XtPointer callData)
{
#ifndef VMS
    AbortShellCommand();
#endif
}

static void learnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    BeginLearn(window);
}

static void finishLearnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    FinishLearn();
}

static void cancelLearnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    CancelLearn();
}

static void replayCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Replay(window);
}

static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_START);
}

static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SEARCH);
}

static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SELECT);
}

static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_CLIPBOARD);
}

static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_PROGRAMMER);
}

static void helpMouseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MOUSE);
}

static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_KEYBOARD);
}

static void helpFillCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_FILL);
}

static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_RECOVERY);
}

static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_PREFERENCES);
}

static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SHELL);
}

static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_REGEX);
}

static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_COMMAND_LINE);
}

static void helpServerCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SERVER);
}

static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_CUSTOMIZE);
}

static void helpMacroCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MACROS);
}

static void helpResourcesCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_RESOURCES);
}

static void helpActionsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_ACTIONS);
}

static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_VERSION);
}

static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_DISTRIBUTION);
}

static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MAILING_LIST);
}

static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_BUGS);
}

static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData)
{
    if (!window->windowMenuValid) {
    	updateWindowMenu(window);
    	window->windowMenuValid = True;
    }
}

/*
** Action Procedures for menu item commands
*/
static void newAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    EditNewFile();
    CheckCloseDim();
}

static void openDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char fullname[MAXPATHLEN], *params[1];
    int response;
    
    response = PromptForExistingFile(window, "File to Edit:", fullname);
    if (response != GFN_OK)
    	return;
    params[0] = fullname;
    XtCallActionProc(window->lastFocus, "open", event, params, 1);
    CheckCloseDim();
}

static void openAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
    
    if (*nArgs == 0) {
    	fprintf(stderr, "NEdit: open action requires file argument\n");
    	return;
    }
    ParseFilename(args[0], filename, pathname);
    EditExistingFile(window, filename, pathname, False);
    CheckCloseDim();
}

static void openSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    OpenSelectedFile(WidgetToWindow(w), event->xbutton.time);
    CheckCloseDim();
}

static void closeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    CloseFileAndWindow(WidgetToWindow(w));
    CheckCloseDim();
}

static void saveAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);

    if (CheckReadOnly(window))
    	return;
    SaveWindow(window);
}

static void saveAsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    int response;
    char fullname[MAXPATHLEN], *params[1];
    
    response = PromptForNewFile(window, "Save File As:", fullname);
    if (response != GFN_OK)
    	return;
    params[0] = fullname;
    XtCallActionProc(window->lastFocus, "save-as", event, params, 1);
}

static void saveAsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    if (*nArgs == 0) {
    	fprintf(stderr, "NEdit: save-as action requires file argument\n");
    	return;
    }
    SaveWindowAs(WidgetToWindow(w), args[0]);
}

static void revertAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    RevertToSaved(WidgetToWindow(w), False);
}

static void includeDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], *params[1];
    int response;
    
    if (CheckReadOnly(window))
    	return;
    response = PromptForExistingFile(window, "File to include:", filename);
    if (response != GFN_OK)
    	return;
    params[0] = filename;
    XtCallActionProc(window->lastFocus, "include-file", event, params, 1);
}

static void includeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);

    if (CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
    	fprintf(stderr, "NEdit: include action requires file argument\n");
    	return;
    }
    IncludeFile(WidgetToWindow(w), args[0]);
}

static void loadTagsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], *params[1];
    int response;
    
    response = PromptForExistingFile(window, "ctags file:", filename);
    if (response != GFN_OK)
    	return;
    params[0] = filename;
    XtCallActionProc(window->lastFocus, "load-tags-file", event, params, 1);
}

static void loadTagsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    if (*nArgs == 0) {
    	fprintf(stderr, "NEdit: load-tags-file action requires file argument\n");
    	return;
    }
    if (!LoadTagsFile(args[0]))
    	DialogF(DF_WARN, WidgetToWindow(w)->shell, 1,
    		"Error reading ctags file,\ntags not loaded", "OK");
}

static void printAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    PrintWindow(WidgetToWindow(w), False);
}

static void printSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    PrintWindow(WidgetToWindow(w), True);
}

static void exitAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
#ifdef EXIT_WARNING
    WindowInfo *window = WidgetToWindow(w);
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

static void undoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    Undo(window);
}

static void redoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    Redo(window);
}

static void clearAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    BufRemoveSelected(window->buffer);
}

static void selAllAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    BufSelect(window->buffer, 0, window->buffer->length);
}

static void shiftLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_LEFT, False);
}

static void shiftLeftTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_LEFT, True);
}

static void shiftRightAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_RIGHT, False);
}

static void shiftRightTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_RIGHT, True);
}

static void findDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    DoFindDlog(WidgetToWindow(w), searchDirection(0, args, nArgs));
}

static void findAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0) {
    	fprintf(stderr, "NEdit: find action requires search string argument\n");
    	return;
    }
    SearchAndSelect(WidgetToWindow(w), searchDirection(1, args, nArgs), args[0],
    	    searchType(1, args, nArgs));    
}

static void findSameAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    SearchAndSelectSame(WidgetToWindow(w), searchDirection(0, args, nArgs));
}

static void findSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    SearchForSelected(WidgetToWindow(w), searchDirection(0, args, nArgs),
    	    event->xbutton.time);
}

static void replaceDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    DoReplaceDlog(window, searchDirection(0, args, nArgs));
}

static void replaceAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
    	fprintf(stderr,
    	"NEdit: replace action requires search and replace string arguments\n");
    	return;
    }
    SearchAndReplace(window, searchDirection(2, args, nArgs),
    	    args[0], args[1], searchType(2, args, nArgs));
}

static void replaceAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
    	fprintf(stderr,
    "NEdit: replace-all action requires search and replace string arguments\n");
    	return;
    }
    ReplaceAll(window, args[0], args[1], searchType(2, args, nArgs));
}

static void replaceInSelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
    	fprintf(stderr,
  "NEdit: replace-in-selection requires search and replace string arguments\n");
    	return;
    }
    ReplaceInSelection(window, args[0], args[1],
    	    searchType(2, args, nArgs));
}

static void replaceSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    ReplaceSame(window, searchDirection(0, args, nArgs));
}

static void gotoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    int lineNum;
    
    if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1) {
    	fprintf(stderr,"NEdit: goto-line-number action requires line number\n");
    	return;
    }
    SelectNumberedLine(WidgetToWindow(w), lineNum);
}

static void gotoDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoLineNumber(WidgetToWindow(w));
}

static void gotoSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoSelectedLineNumber(WidgetToWindow(w), event->xbutton.time);
}

static void markAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0 || strlen(args[0]) != 1 || !isalnum(args[0][0])) {
    	fprintf(stderr,"NEdit: mark action requires a single-letter label\n");
    	return;
    }
    AddMark(WidgetToWindow(w), w, args[0][0]);
}

static void markDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    MarkDialog(WidgetToWindow(w));
}

static void gotoMarkAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0 || strlen(args[0]) != 1 || !isalnum(args[0][0])) {
     	fprintf(stderr,
     	    	"NEdit: goto-mark action requires a single-letter label\n");
     	return;
    }
    GotoMark(WidgetToWindow(w), w, args[0][0]);
}

static void gotoMarkDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoMarkDialog(WidgetToWindow(w));
}

static void findMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    MatchSelectedCharacter(WidgetToWindow(w));
}

static void findDefAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    FindDefinition(WidgetToWindow(w));
}

static void splitWindowAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    SplitWindow(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void closePaneAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    ClosePane(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void capitalizeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    UpcaseSelection(window);
}

static void lowercaseAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    DowncaseSelection(window);
}

static void fillAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if (CheckReadOnly(window))
    	return;
    FillSelection(window);
}

static void controlDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    char charCodeString[2], charCodeText[DF_MAX_PROMPT_LENGTH], *params[1];
    int charCode, nRead, response;
    
    if (CheckReadOnly(window))
    	return;
    response = DialogF(DF_PROMPT, window->shell, 2,
    	    "ASCII Character Code (decimal):", charCodeText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(charCodeText, "%d", &charCode);
    if (nRead != 1 || charCode <=0 || charCode >= 256) {
    	XBell(TheDisplay, 0);
	return;
    }
    charCodeString[0] = (char)charCode; charCodeString[1] = '\0';
    params[0] = charCodeString;
    XtCallActionProc(w, "insert-string", event, params, 1);
}

#ifndef VMS
static void filterDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    char *params[1], cmdText[DF_MAX_PROMPT_LENGTH];
    int resp;
    
    if (CheckReadOnly(window))
    	return;
    if (!window->buffer->primary.selected) {
    	XBell(TheDisplay, 0);
	return;
    }
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Enter shell command for filtering selection:",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    params[0] = cmdText;
    XtCallActionProc(w, "filter-selection", event, params, 1);;
}

static void shellFilterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if (CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
    	fprintf(stderr,
    		"NEdit: filter-selection requires shell command argument\n");
    	return;
    }
    FilterSelection(window, args[0]);
}

static void execDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    char *params[1], cmdText[DF_MAX_PROMPT_LENGTH];
    int resp;

    if (CheckReadOnly(window))
    	return;
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Enter shell command to execute:",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    params[0] = cmdText;
    XtCallActionProc(w, "execute-command", event, params, 1);;
}

static void execAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if (CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
    	fprintf(stderr,
    		"NEdit: execute-command requires shell command argument\n");
    	return;
    }
    ExecShellCommand(window, args[0]);
}

static void execLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if (CheckReadOnly(window))
    	return;
    ExecCursorLine(window);
}

static void shellMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0) {
    	fprintf(stderr,
    		"NEdit: shell-menu-command requires item-name argument\n");
    	return;
    }
    DoNamedShellMenuCmd(WidgetToWindow(w), args[0]);
}
#endif

static void macroMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0) {
    	fprintf(stderr,
    		"NEdit: shell-menu-command requires item-name argument\n");
    	return;
    }
    DoNamedMacroMenuCmd(WidgetToWindow(w), args[0]);
}

static void beginningOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textBuffer *buf = TextGetBuffer(w);
    int start, end, isRect, rectStart, rectEnd;
    
    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    if (!isRect)
    	TextSetCursorPos(w, start);
    else
    	TextSetCursorPos(w, BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, start), rectStart));
}

static void endOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textBuffer *buf = TextGetBuffer(w);
    int start, end, isRect, rectStart, rectEnd;
    
    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    if (!isRect)
    	TextSetCursorPos(w, end);
    else
    	TextSetCursorPos(w, BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, end), rectEnd));
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
    XmString st1;
    
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
void CheckCloseDim(void)
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
void InvalidateWindowMenus(void)
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

/*
** Scans action argument list for arguments "forward" or "backward" to
** determine search direction for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchDirection(int ignoreArgs, String *args, Cardinal *nArgs)
{
    int i;
    
    for (i=ignoreArgs; i<*nArgs; i++) {
    	if (!strCaseCmp(args[i], "forward"))
    	    return SEARCH_FORWARD;
    	if (!strCaseCmp(args[i], "backward"))
    	    return SEARCH_BACKWARD;
    }
    return SEARCH_FORWARD;
}

/*
** Scans action argument list for arguments "literal", "case" or "regex" to
** determine search type for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchType(int ignoreArgs, String *args, Cardinal *nArgs)
{
    int i;
    
    for (i=ignoreArgs; i<*nArgs; i++) {
    	if (!strCaseCmp(args[i], "literal"))
    	    return SEARCH_LITERAL;
    	if (!strCaseCmp(args[i], "case"))
    	    return SEARCH_CASE_SENSE;
    	if (!strCaseCmp(args[i], "regex"))
    	    return SEARCH_REGEX;
    }
    return GetPrefSearch();
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters given a callback XmAnyCallbackStruct pointed to by
** "callData", by checking if the shift key is pressed (for search callbacks).
*/
static char **shiftKeyToDir(XtPointer callData)
{
    static char *backwardParam[1] = {"backward"};
    static char *forwardParam[1] = {"forward"};
    if (((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask)
    	return backwardParam;
    return forwardParam;
}

static void raiseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* XMapRaised as opposed to XRaiseWindow will uniconify as well as raise */
    XMapRaised(TheDisplay, XtWindow(window->shell));
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
}

/*
** strCaseCmp compares its arguments and returns 0 if the two strings
** are equal IGNORING case differences.  Otherwise returns 1.
*/
static int strCaseCmp(char *str1, char *str2)
{
    char *c1, *c2;
    
    for (c1=str1, c2=str2; *c1!='\0' && *c2!='\0'; c1++, c2++)
    	if (toupper(*c1) != toupper(*c2))
    	    return 1;
    return 0;
}

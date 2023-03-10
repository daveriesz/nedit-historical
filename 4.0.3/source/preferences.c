/*******************************************************************************
*									       *
* preferences.c -- Nirvana Editor preferences processing		       *
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
* April 20, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)preferences.c	1.17     9/8/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <errno.h>
#include <Xm/Xm.h>
#include <Xm/SelectioB.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>
#include "../util/prefFile.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "textBuf.h"
#include "nedit.h"
#include "text.h"
#include "search.h"
#include "preferences.h"
#include "window.h"
#include "userCmds.h"
#include "help.h"

#define PREF_FILE_NAME ".nedit"

/* The shell and macro commands items can theoretically be of any length,
   however, the prefFile mechanism requires a buffer in which to store the
   text.  These are reasonable guesses at the maximum length of commands that
   anyone would want, about 100 80 character shell commands with 20 character
   names, and about 100K of macro commands.  This space is temporarily
   allocated durring the RestoreNEditPrefs call */
#define MAX_SHELL_CMDS_LEN 10000
#define MAX_MACRO_CMDS_LEN 100000

/* maximum number of word delimiters allowed (256 allows whole character set) */
#define MAX_WORD_DELIMITERS 256

char *SearchMethodStrings[] = {"Literal", "CaseSense", "RegExp", NULL};

static struct prefData {
    int wrapText;		/* whether to set text widget for wrap */
    int wrapMargin;		/* 0=wrap at window width, other=wrap margin */
    int autoIndent;		/* whether to set text widget for auto-indent */
    int autoSave;		/* whether automatic backup feature is on */
    int saveOldVersion;		/* whether to preserve a copy of last version */
    int searchDlogs;		/* whether to show explanatory search dialogs */
    int keepSearchDlogs;	/* whether to retain find and replace dialogs */
    int statsLine;		/* whether to show the statistics line */
    int searchMethod;		/* initial search method as a text string */
    int textRows;		/* initial window height in characters */
    int textCols;		/* initial window width in characters */
    int tabDist;		/* number of characters between tab stops */
    int emTabDist;		/* non-zero tab dist. if emulated tabs are on */
    int insertTabs;		/* whether to use tabs for padding */
    int showMatching;		/* whether to flash matching parenthesis */
#ifdef SGI_CUSTOM
    int shortMenus; 	    	/* short menu mode */
#endif
    char fontString[MAX_FONT_LEN]; /* name of font for text widget */
    XmFontList fontList;	/* XmFontList corresp. to above named font */
    int repositionDialogs;	/* w. to reposition dialogs under the pointer */
    int mapDelete;		/* whether to map delete to backspace */
    int stdOpenDialog;		/* w. to retain redundant text field in Open */
    char tagFile[MAXPATHLEN];	/* name of tags file to look for at startup */
    int maxPrevOpenFiles;   	/* limit to size of Open Previous menu */
    char delimiters[MAX_WORD_DELIMITERS]; /* punctuation characters */
    char shell[MAXPATHLEN];	/* shell to use for executing commands */
    char serverName[MAXPATHLEN];/* server name for multiple servers per disp. */
} PrefData;

/* preference descriptions for SavePreferences and RestorePreferences.  Note
   that shellCommands and macroCommands must come first, since the address of
   the buffer where the strings are written are added at runtime. */
static PrefDescripRec PrefDescrip[] = {
#ifndef VMS
#ifdef linux
    {"shellCommands", "ShellCommands", PREF_STRING, "spell:Alt+B:s:EX:\n\
	cat>spellTmp; xterm -e ispell -x spellTmp; cat spellTmp; rm spellTmp\n\
	wc::w:ED:\nwc\nsort::o:EX:\nsort\n\
	number lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
	expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
    	NULL, (void *)MAX_SHELL_CMDS_LEN, True},
#else
    {"shellCommands", "ShellCommands", PREF_STRING, "spell:Alt+B:s:ED:\n\
    	(cat;echo \"\") | spell\nwc::w:ED:\nwc\nsort::o:EX:\nsort\n\
	number lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
	expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
    	NULL, (void *)MAX_SHELL_CMDS_LEN, True},
#endif /* linux */
#endif /* VMS */
    {"macroCommands", "MacroCommands", PREF_STRING, "Quote Mail Reply:::: {\n\
		replace-in-selection(\"^.*$\", \"> &\", \"regex\")\n\
	}\n\
	Unquote Mail Reply:::: {\n\
		replace-in-selection(\"(^> )(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	C Comment Out Sel.:::: {\n\
		beginning-of-selection()\n\
		mark(\"1\")\n\
		deselect-all()\n\
		insert-string(\"/* \")\n\
		goto-mark(\"1\")\n\
		beginning-of-selection()\n\
		backward-character(\"extend\")\n\
		backward-character(\"extend\")\n\
		backward-character(\"extend\")\n\
		mark(\"1\")\n\
		end-of-selection()\n\
		deselect-all()\n\
		insert-string(\" */\")\n\
		goto-mark(\"1\")\n\
	}\n\
	C Uncomment Sel.:::: {\n\
		beginning-of-selection()\n\
		mark(\"1\")\n\
		deselect-all()\n\
		replace(\"/* \", \"\")\n\
		goto-mark(\"1\")\n\
		end-of-selection()\n\
		deselect-all()\n\
		replace(\" */\", \"\", \"backward\")\n\
		goto-mark(\"1\")\n\
		\n\
	}\n\
	+ C++ Comment:::: {\n\
		replace-in-selection(\"^.*$\", \"// &\", \"regex\")\n\
	}\n\
	- C++ Comment:::: {\n\
		replace-in-selection(\"(^// )(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	+ C Bar Comment 1:::: {\n\
		replace-in-selection(\"^.*$\", \" * &\", \"regex\")\n\
		mark(\"1\")\n\
		beginning-of-selection()\n\
		deselect-all()\n\
		insert-string(\"/*\")\n\
		newline()\n\
		goto-mark(\"1\")\n\
		beginning-of-selection()\n\
		backward-character(\"extend\")\n\
		backward-character(\"extend\")\n\
		backward-character(\"extend\")\n\
		mark(\"1\")\n\
		end-of-selection()\n\
		deselect-all()\n\
		newline()\n\
		backward-character()\n\
		insert-string(\" */\")\n\
		goto-mark(\"1\")}\n\
	- C Bar Comment 1:::: {\n\
		replace-in-selection(\"^(( |\\\\t)*)\\\\*(.*)$\", \"\\\\1\\\\3\", \"regex\")\n\
		shift-left()\n\
		shift-left()\n\
		mark(\"1\")\n\
		beginning-of-selection()\n\
		deselect-all()\n\
		end-of-line(\"extend\")\n\
		delete-next-character()\n\
		delete-next-character()\n\
		goto-mark(\"1\")\n\
		end-of-selection()\n\
		deselect-all()\n\
		delete-previous-character()\n\
		beginning-of-line(\"extend\")\n\
		delete-next-character()\n\
		goto-mark(\"1\")}\n",
	NULL, (void *)MAX_MACRO_CMDS_LEN, True},
    {"autoWrap", "AutoWrap", PREF_BOOLEAN, "True",
    	&PrefData.wrapText, NULL, True},
    {"wrapMargin", "WrapMargin", PREF_INT, "0",
    	&PrefData.wrapMargin, NULL, True},
    {"autoIndent", "AutoIndent", PREF_BOOLEAN, "True",
    	&PrefData.autoIndent, NULL, True},
    {"autoSave", "AutoSave", PREF_BOOLEAN, "True",
    	&PrefData.autoSave, NULL, True},
    {"saveOldVersion", "SaveOldVersion", PREF_BOOLEAN, "False",
    	&PrefData.saveOldVersion, NULL, True},
    {"showMatching", "ShowMatching", PREF_BOOLEAN, "True",
    	&PrefData.showMatching, NULL, True},
    {"searchDialogs", "SearchDialogs", PREF_BOOLEAN, "False",
    	&PrefData.searchDlogs, NULL, True},
    {"retainSearchDialogs", "RetainSearchDialogs", PREF_BOOLEAN, "False",
    	&PrefData.keepSearchDlogs, NULL, True},
#if XmVersion < 1002 /* Flashing is annoying in 1.1 versions */
    {"repositionDialogs", "RepositionDialogs", PREF_BOOLEAN, "False",
    	&PrefData.repositionDialogs, NULL, True},
#else
    {"repositionDialogs", "RepositionDialogs", PREF_BOOLEAN, "True",
    	&PrefData.repositionDialogs, NULL, True},
#endif
    {"statisticsLine", "StatisticsLine", PREF_BOOLEAN, "False",
    	&PrefData.statsLine, NULL, True},
    {"searchMethod", "SearchMethod", PREF_ENUM, "Literal",
    	&PrefData.searchMethod, SearchMethodStrings, True},
    {"textRows", "TextRows", PREF_INT, "24",
    	&PrefData.textRows, NULL, True},
    {"textCols", "TextCols", PREF_INT, "80",
    	&PrefData.textCols, NULL, True},
    {"tabDistance", "TabDistance", PREF_INT, "8",
    	&PrefData.tabDist, NULL, True},
    {"emulateTabs", "EmulateTabs", PREF_INT, "0",
    	&PrefData.emTabDist, NULL, True},
    {"insertTabs", "InsertTabs", PREF_BOOLEAN, "True",
    	&PrefData.insertTabs, NULL, True},
    {"textFont", "TextFont", PREF_STRING,
    	"-adobe-courier-medium-r-normal--14-*-*-*-*-*-*-*",
    	PrefData.fontString, (void *)sizeof(PrefData.fontString), True},
    {"shell", "Shell", PREF_STRING, "/bin/csh",
    	PrefData.shell, (void *)sizeof(PrefData.shell), False},
    {"remapDeleteKey", "RemapDeleteKey", PREF_BOOLEAN, "True",
    	&PrefData.mapDelete, NULL, False},
    {"stdOpenDialog", "StdOpenDialog", PREF_BOOLEAN, "False",
    	&PrefData.stdOpenDialog, NULL, False},
    {"tagFile", "TagFile", PREF_STRING,
    	"", PrefData.tagFile, (void *)sizeof(PrefData.tagFile), False},
    {"wordDelimiters", "WordDelimiters", PREF_STRING,
    	".,/\\`'!@#%^&*()-=+{}[]\":;<>?",
    	PrefData.delimiters, (void *)sizeof(PrefData.delimiters), False},
    {"serverName", "serverName", PREF_STRING, "", PrefData.serverName,
      (void *)sizeof(PrefData.serverName), False},
    {"maxPrevOpenFiles", "MaxPrevOpenFiles", PREF_INT, "30",
    	&PrefData.maxPrevOpenFiles, NULL, False},
#ifdef SGI_CUSTOM
    {"shortMenus", "ShortMenus", PREF_BOOLEAN, "False", &PrefData.shortMenus,
      NULL, True},
#endif
};

static XrmOptionDescRec OpTable[] = {
    {"-wrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"True"},
    {"-nowrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"False"},
    {"-autoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"True"},
    {"-noautoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"False"},
    {"-autosave", ".autoSave", XrmoptionNoArg, (caddr_t)"True"},
    {"-noautosave", ".autoSave", XrmoptionNoArg, (caddr_t)"False"},
    {"-rows", ".textRows", XrmoptionSepArg, (caddr_t)NULL},
    {"-columns", ".textCols", XrmoptionSepArg, (caddr_t)NULL},
    {"-tabs", ".tabDistance", XrmoptionSepArg, (caddr_t)NULL},
    {"-font", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
    {"-fn", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
    {"-svrname", ".serverName", XrmoptionSepArg, (caddr_t)NULL},
};

static char HeaderText[] = "\
# Preferences file for NEdit\n\
#\n\
# This file is overwritten by the \"Save Defaults...\" command in NEdit \n\
# and serves only the interactively setable options presented in the NEdit\n\
# \"Preferences\" menu.  To modify other options, such as background colors\n\
# and key bindings, use the .Xdefaults file in your home directory (or\n\
# the X resource specification method appropriate to your system).  The\n\
# contents of this file can be moved into an X resource file, but since\n\
# resources in this file override their corresponding X resources, either\n\
# this file should be deleted or individual resource lines in the file\n\
# should be deleted for the moved lines to take effect.\n";

/* Module-global variables to support Initial Window Size... dialog */
static int DoneWithSizeDialog;
static Widget RowText, ColText;

/* Module-global variables for Tabs dialog */
static int DoneWithTabsDialog;
static WindowInfo *TabsDialogForWindow;
static Widget TabDistText, EmTabText, EmTabToggle, UseTabsToggle, EmTabLabel;

/* Module-global variables for Wrap Margin dialog */
static int DoneWithWrapDialog;
static WindowInfo *WrapDialogForWindow;
static Widget WrapText, WrapTextLabel, WrapWindowToggle;

static void sizeOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void sizeCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsHelpCB(Widget w, XtPointer clientData, XtPointer callData);
static void emTabsCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapWindowCB(Widget w, XtPointer clientData, XtPointer callData);

XrmDatabase CreateNEditPrefDB(int *argcInOut, char **argvInOut)
{
    return CreatePreferencesDatabase(PREF_FILE_NAME, APP_NAME, 
	    OpTable, XtNumber(OpTable), (unsigned int *)argcInOut, argvInOut);
}

void RestoreNEditPrefs(XrmDatabase prefDB, XrmDatabase appDB)
{
    XFontStruct *font;
    char *shellCmdsStr, *macroCmdsStr;
    int i = 0;
    
#ifndef VMS
    shellCmdsStr = XtMalloc(MAX_SHELL_CMDS_LEN);
    PrefDescrip[i++].valueAddr = shellCmdsStr;
#endif /* VMS */
    macroCmdsStr = XtMalloc(MAX_MACRO_CMDS_LEN);
    PrefDescrip[i++].valueAddr = macroCmdsStr;
    RestorePreferences(prefDB, appDB, APP_NAME,
    	    APP_CLASS, PrefDescrip, XtNumber(PrefDescrip));
#ifndef VMS
    LoadShellCmdsString(shellCmdsStr);
    XtFree(shellCmdsStr);
#endif /* VMS */
    LoadMacroCmdsString(macroCmdsStr);
    XtFree(macroCmdsStr);
    
    /* translate the font name into a fontList suitable for the text widget */
    font = XLoadQueryFont(TheDisplay, PrefData.fontString);
    PrefData.fontList = font==NULL ? NULL :
	    XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
}

void SaveNEditPrefs(Widget parent)
{
    char *shellCmdsStr, *macroCmdsStr;
    int i = 0;
    
#ifndef VMS
    shellCmdsStr = WriteShellCmdsString();
    PrefDescrip[i++].valueAddr = shellCmdsStr;
#endif /* VMS */
    macroCmdsStr = WriteMacroCmdsString();
    PrefDescrip[i++].valueAddr = macroCmdsStr;
    if (SavePreferences(XtDisplay(parent), PREF_FILE_NAME, HeaderText,
    	    PrefDescrip, XtNumber(PrefDescrip)))
    	DialogF(DF_INF, parent, 1, 
#ifdef VMS
		"Default preferences saved in SYS$LOGIN:.NEDIT\n\
NEdit automatically loads this file each\ntime it is started.", "OK");
#else
    		"Default preferences saved in $HOME/.nedit\n\
NEdit automatically loads this file each\ntime it is started.", "OK");
#endif /*VMS*/
    else
    	DialogF(DF_WARN, parent, 1,
#ifdef VMS
    		"Unable to save preferences in SYS$LOGIN:.NEDIT", "OK");
#else
    		"Unable to save preferences in $HOME/.nedit", "OK");
#endif /*VMS*/

#ifndef VMS
    XtFree(shellCmdsStr);
#endif
    XtFree(macroCmdsStr);
}

void SetPrefWrap(int state)
{
    PrefData.wrapText = state;
}

int GetPrefWrap(void)
{
    return PrefData.wrapText;
}

void SetPrefWrapMargin(int margin)
{
    PrefData.wrapMargin = margin;
}

int GetPrefWrapMargin(void)
{
    return PrefData.wrapMargin;
}

void SetPrefSearch(int searchType)
{
    PrefData.searchMethod = searchType;
}

int GetPrefSearch(void)
{
    return PrefData.searchMethod;
}

void SetPrefAutoIndent(int state)
{
    PrefData.autoIndent = state;
}

int GetPrefAutoIndent(void)
{
    return PrefData.autoIndent;
}

void SetPrefAutoSave(int state)
{
    PrefData.autoSave = state;
}

int GetPrefAutoSave(void)
{
    return PrefData.autoSave;
}

void SetPrefSaveOldVersion(int state)
{
    PrefData.saveOldVersion = state;
}

int GetPrefSaveOldVersion(void)
{
    return PrefData.saveOldVersion;
}

void SetPrefSearchDlogs(int state)
{
    PrefData.searchDlogs = state;
}

int GetPrefSearchDlogs(void)
{
    return PrefData.searchDlogs;
}

void SetPrefKeepSearchDlogs(int state)
{
    PrefData.keepSearchDlogs = state;
}

int GetPrefKeepSearchDlogs(void)
{
    return PrefData.keepSearchDlogs;
}

void SetPrefStatsLine(int state)
{
    PrefData.statsLine = state;
}

int GetPrefStatsLine(void)
{
    return PrefData.statsLine;
}

void SetPrefMapDelete(int state)
{
    PrefData.mapDelete = state;
}

int GetPrefMapDelete(void)
{
    return PrefData.mapDelete;
}

void SetPrefStdOpenDialog(int state)
{
    PrefData.stdOpenDialog = state;
}

int GetPrefStdOpenDialog(void)
{
    return PrefData.stdOpenDialog;
}

void SetPrefRows(int nRows)
{
    PrefData.textRows = nRows;
}

int GetPrefRows(void)
{
    return PrefData.textRows;
}

void SetPrefCols(int nCols)
{
   PrefData.textCols = nCols;
}

int GetPrefCols(void)
{
    return PrefData.textCols;
}

void SetPrefTabDist(int tabDist)
{
    PrefData.tabDist = tabDist;
}

int GetPrefTabDist(void)
{
    return PrefData.tabDist;
}

void SetPrefEmTabDist(int tabDist)
{
    PrefData.emTabDist = tabDist;
}

int GetPrefEmTabDist(void)
{
    return PrefData.emTabDist;
}

void SetPrefInsertTabs(int state)
{
    PrefData.insertTabs = state;
}

int GetPrefInsertTabs(void)
{
    return PrefData.insertTabs;
}

void SetPrefShowMatching(int state)
{
    PrefData.showMatching = state;
}

int GetPrefShowMatching(void)
{
    return PrefData.showMatching;
}

void SetPrefRepositionDialogs(int state)
{
    PrefData.repositionDialogs = state;
}

int GetPrefRepositionDialogs(void)
{
    return PrefData.repositionDialogs;
}

void SetPrefTagFile(char *tagFileName)
{
    strcpy(PrefData.tagFile, tagFileName);
}

char *GetPrefTagFile(void)
{
    return PrefData.tagFile;
}

char *GetPrefDelimiters(void)
{
    return PrefData.delimiters;
}

/*
** Set the font preference using the font name (the fontList is generated
** in this call).  Note that this leaks memory and server resources each
** time the default font is re-set.  See note on SetFontByName for more
** information.
*/
void SetPrefFont(char *fontName)
{
    XFontStruct *font;
    
    strcpy(PrefData.fontString, fontName);
    font = XLoadQueryFont(TheDisplay, fontName);
    PrefData.fontList = font==NULL ? NULL :
	    XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
}

char *GetPrefFontName(void)
{
    return PrefData.fontString;
}

XmFontList GetPrefFontList(void)
{
    return PrefData.fontList;
}

void SetPrefShell(char *shell)
{
    strcpy(PrefData.shell, shell);
}

char *GetPrefShell(void)
{
    return PrefData.shell;
}

char *GetPrefServerName(void)
{
    return PrefData.serverName;
}

int GetPrefMaxPrevOpenFiles(void)
{
    return PrefData.maxPrevOpenFiles;
}

#ifdef SGI_CUSTOM
void SetPrefShortMenus(int state)
{
    PrefData.shortMenus = state;
}

int GetPrefShortMenus(void)
{
    return PrefData.shortMenus;
}
#endif

void RowColumnPrefDialog(Widget parent)
{
    Widget form, selBox, topLabel;
    Arg selBoxArgs[2];
    XmString s1;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "customSize", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)sizeOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)sizeCancelCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Initial Window Size", 0);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, 0);

    topLabel = XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
    	    	"Enter desired size in rows\nand columns of characters:"), 0);
    XmStringFree(s1);
 
    RowText = XtVaCreateManagedWidget("rows", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 5,
    	    XmNrightPosition, 45, 0);
    RemapDeleteKey(RowText);
 
    XtVaCreateManagedWidget("xLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("x"),
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
    	    XmNtopWidget, topLabel,
    	    XmNbottomWidget, RowText,
    	    XmNleftPosition, 45,
    	    XmNrightPosition, 55, 0);
    XmStringFree(s1);

    ColText = XtVaCreateManagedWidget("cols", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 55,
    	    XmNrightPosition, 95, 0);
    RemapDeleteKey(ColText);

    /* put up dialog and wait for user to press ok or cancel */
    DoneWithSizeDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithSizeDialog)
        XtAppProcessEvent (XtWidgetToApplicationContext(parent), XtIMAll);
    
    XtDestroyWidget(selBox);
}

static void sizeOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int rowValue, colValue, stat;
    
    /* get the values that the user entered and make sure they're ok */
    stat = GetIntTextWarn(RowText, &rowValue, "number of rows", True);
    if (stat != TEXT_READ_OK)
    	return;
    stat = GetIntTextWarn(ColText, &colValue, "number of columns", True);
    if (stat != TEXT_READ_OK)
    	return;
    
    /* set the corresponding preferences and dismiss the dialog */
    SetPrefRows(rowValue);
    SetPrefCols(colValue);
    DoneWithSizeDialog = True;
}

static void sizeCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithSizeDialog = True;
}

/*
** Present the user a dialog for setting tab related preferences, either as
** defaults, or for a specific window (pass "forWindow" as NULL to set default
** preference, or a window to set preferences for the specific window.
*/
void TabsPrefDialog(Widget parent, WindowInfo *forWindow)
{
    Widget form, selBox;
    Arg selBoxArgs[2];
    XmString s1;
    int emulate, emTabDist, useTabs, tabDist;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "customSize", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)tabsOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)tabsCancelCB,NULL);
    XtAddCallback(selBox, XmNhelpCallback, (XtCallbackProc)tabsHelpCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Tabs", 0);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, 0);

    TabDistText = XtVaCreateManagedWidget("tabDistText", xmTextWidgetClass,
    	    form, XmNcolumns, 7,
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_FORM, 0);
    RemapDeleteKey(TabDistText);
    XtVaCreateManagedWidget("tabDistLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Tab spacing (for hardware tab characters)"),
	    XmNmnemonic, 'T',
    	    XmNuserData, TabDistText,
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, TabDistText,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, TabDistText, 0);
    XmStringFree(s1);
 
    EmTabText = XtVaCreateManagedWidget("emTabText", xmTextWidgetClass, form,
    	    XmNcolumns, 7,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
    	    XmNrightWidget, TabDistText, 0);
    RemapDeleteKey(EmTabText);
    EmTabLabel = XtVaCreateManagedWidget("emTabLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("Emulated tab spacing"),
	    XmNmnemonic, 's',
    	    XmNuserData, EmTabText,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, EmTabText,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, EmTabText, 0);
    XmStringFree(s1);
    EmTabToggle = XtVaCreateManagedWidget("emTabToggle",
    	    xmToggleButtonGadgetClass, form, XmNlabelString,
    	    	s1=XmStringCreateSimple("Emulate tabs"),
	    XmNmnemonic, 'E',
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, EmTabText, 0);
    XmStringFree(s1);
    XtAddCallback(EmTabToggle, XmNvalueChangedCallback, emTabsCB, NULL);
    UseTabsToggle = XtVaCreateManagedWidget("useTabsToggle",
    	    xmToggleButtonGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Use tab characters in padding and emulated tabs"),
	    XmNmnemonic, 'U',
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, EmTabText,
    	    XmNtopOffset, 5,
    	    XmNleftAttachment, XmATTACH_FORM, 0);
    XmStringFree(s1);

    /* Set default values */
    if (forWindow == NULL) {
    	emTabDist = GetPrefEmTabDist();
    	useTabs = GetPrefInsertTabs();
    	tabDist = GetPrefTabDist();
    } else {
    	XtVaGetValues(forWindow->textArea, textNemulateTabs, &emTabDist, 0);
    	useTabs = forWindow->buffer->useTabs;
    	tabDist = BufGetTabDistance(forWindow->buffer);
    }
    emulate = emTabDist != 0;
    SetIntText(TabDistText, tabDist);
    XmToggleButtonGadgetSetState(EmTabToggle, emulate, True);
    if (emulate)
    	SetIntText(EmTabText, emTabDist);
    XmToggleButtonGadgetSetState(UseTabsToggle, useTabs, False);
    XtSetSensitive(EmTabText, emulate);
    XtSetSensitive(EmTabLabel, emulate);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* Set the widget to get focus */
#if XmVersion >= 1002
    XtVaSetValues(form, XmNinitialFocus, TabDistText, 0);
#endif
    
    /* put up dialog and wait for user to press ok or cancel */
    TabsDialogForWindow = forWindow;
    DoneWithTabsDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithTabsDialog)
        XtAppProcessEvent(XtWidgetToApplicationContext(parent), XtIMAll);
    
    XtDestroyWidget(selBox);
}

static void tabsOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int i, emulate, useTabs, stat, tabDist, emTabDist;
    WindowInfo *window = TabsDialogForWindow;
    
    /* get the values that the user entered and make sure they're ok */
    emulate = XmToggleButtonGadgetGetState(EmTabToggle);
    useTabs = XmToggleButtonGadgetGetState(UseTabsToggle);
    stat = GetIntTextWarn(TabDistText, &tabDist, "tab spacing", True);
    if (stat != TEXT_READ_OK)
    	return;
    if (tabDist <= 0 || tabDist > MAX_EXP_CHAR_LEN) {
    	DialogF(DF_WARN, TabDistText, 1, "Tab spacing out of range", "Dismiss");
    	return;
    }
    if (emulate) {
	stat = GetIntTextWarn(EmTabText, &emTabDist, "emulated tab spacing",True);
	if (stat != TEXT_READ_OK)
	    return;
	if (emTabDist <= 0 || tabDist >= 1000) {
	    DialogF(DF_WARN, EmTabText, 1, "Emulated tab spacing out of range",
	    	    "Dismiss");
	    return;
	}
    } else
    	emTabDist = 0;
    
    /* Set the value in either the requested window or default preferences */
    if (TabsDialogForWindow == NULL) {
    	SetPrefTabDist(tabDist);
    	SetPrefEmTabDist(emTabDist);
    	SetPrefInsertTabs(useTabs);
    } else {
    	if (window->buffer->tabDist != tabDist) {
    	    window->ignoreModify = True;
    	    BufSetTabDistance(window->buffer, tabDist);
    	    window->ignoreModify = False;
    	}
	XtVaSetValues(window->textArea, textNemulateTabs, emTabDist, 0);
	for (i=0; i<window->nPanes; i++)
	    XtVaSetValues(window->textPanes[i], textNemulateTabs, emTabDist, 0);
    	window->buffer->useTabs = useTabs;
    }
    DoneWithTabsDialog = True;
}

static void tabsCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithTabsDialog = True;
}

static void tabsHelpCB(Widget w, XtPointer clientData, XtPointer callData)
{
    Help(XtParent(EmTabLabel), HELP_TABS_DIALOG);
}

static void emTabsCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int state = XmToggleButtonGadgetGetState(w);
    
    XtSetSensitive(EmTabLabel, state);
    XtSetSensitive(EmTabText, state);
}

/*
** Present the user a dialog for setting wrap margin.
*/
void WrapMarginDialog(Widget parent, WindowInfo *forWindow)
{
    Widget form, selBox;
    Arg selBoxArgs[2];
    XmString s1;
    int margin;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "customSize", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)wrapOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)wrapCancelCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Wrap Margin", 0);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, 0);

    WrapWindowToggle = XtVaCreateManagedWidget("wrapWindowToggle",
    	    xmToggleButtonGadgetClass, form, XmNlabelString,
    	    	s1=XmStringCreateSimple("Wrap and Fill at width of window"),
	    XmNmnemonic, 'W',
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM, 0);
    XmStringFree(s1);
    XtAddCallback(WrapWindowToggle, XmNvalueChangedCallback, wrapWindowCB,NULL);
    WrapText = XtVaCreateManagedWidget("wrapText", xmTextWidgetClass, form,
    	    XmNcolumns, 5,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, WrapWindowToggle,
    	    XmNrightAttachment, XmATTACH_FORM, 0);
    RemapDeleteKey(WrapText);
    WrapTextLabel = XtVaCreateManagedWidget("wrapMarginLabel",
    	    xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Margin for Wrap and Fill"),
	    XmNmnemonic, 'M',
    	    XmNuserData, WrapText,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, WrapWindowToggle,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, WrapText,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, WrapText, 0);
    XmStringFree(s1);

    /* Set default value */
    if (forWindow == NULL)
    	margin = GetPrefWrapMargin();
    else
    	XtVaGetValues(forWindow->textArea, textNwrapMargin, &margin, 0);
    XmToggleButtonGadgetSetState(WrapWindowToggle, margin==0, True);
    if (margin != 0)
    	SetIntText(WrapText, margin);
    XtSetSensitive(WrapText, margin!=0);
    XtSetSensitive(WrapTextLabel, margin!=0);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* put up dialog and wait for user to press ok or cancel */
    WrapDialogForWindow = forWindow;
    DoneWithWrapDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithWrapDialog)
        XtAppProcessEvent(XtWidgetToApplicationContext(parent), XtIMAll);
    
    XtDestroyWidget(selBox);
}

static void wrapOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int i, wrapAtWindow, margin, stat;
    WindowInfo *window = WrapDialogForWindow;
    
    /* get the values that the user entered and make sure they're ok */
    wrapAtWindow = XmToggleButtonGadgetGetState(WrapWindowToggle);
    if (wrapAtWindow)
    	margin = 0;
    else {
	stat = GetIntTextWarn(WrapText, &margin, "wrap Margin", True);
	if (stat != TEXT_READ_OK)
    	    return;
	if (margin <= 0 || margin >= 1000) {
    	    DialogF(DF_WARN, WrapText, 1, "Wrap margin out of range", "Dismiss");
    	    return;
	}
    }
    
    /* Set the value in either the requested window or default preferences */
    if (WrapDialogForWindow == NULL)
    	SetPrefWrapMargin(margin);
    else {
	XtVaSetValues(window->textArea, textNwrapMargin, margin, 0);
	for (i=0; i<window->nPanes; i++)
	    XtVaSetValues(window->textPanes[i], textNwrapMargin, margin, 0);
    }
    DoneWithWrapDialog = True;
}

static void wrapCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithWrapDialog = True;
}

static void wrapWindowCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int wrapAtWindow = XmToggleButtonGadgetGetState(w);
    
    XtSetSensitive(WrapTextLabel, !wrapAtWindow);
    XtSetSensitive(WrapText, !wrapAtWindow);
}

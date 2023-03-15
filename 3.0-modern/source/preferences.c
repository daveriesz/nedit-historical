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
static char SCCSID[] = "@(#)preferences.c	1.10     2/24/94";
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
#include <Xm/Text.h>
#include "../util/prefFile.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "nedit.h"
#include "search.h"
#include "preferences.h"
#include "window.h"
#ifndef VMS
#include "shell.h"
#endif

/* the shell commands item can theoretically be of any length, however, the
   prefFile mechanism requires a buffer in which to store the text, so this
   is a reasonable guess at the maximum that anyone would want, about 100
   80 character commands with 20 character names.  This space is temporarily
   allocated as a stack variable durring the RestoreNEditPrefs call */
#define MAX_SHELL_CMDS_LEN 10000

char *SearchMethodStrings[] = {"Literal", "CaseSense", "RegExp", NULL};

static struct prefData {
    int wrapText;		/* whether to set text widget for wrap */
    int autoIndent;		/* whether to set text widget for auto-indent */
    int autoSave;		/* whether automatic backup feature is on */
    int searchDlogs;		/* whether to show explanatory search dialogs */
    int statsLine;		/* whether to show the statistics line */
    int searchMethod;		/* initial search method as a text string */
    int textRows;		/* initial window height in characters */
    int textCols;		/* initial window width in characters */
    int tabDist;		/* number of characters between tab stops */
    char fontString[MAX_FONT_LEN]; /* name of font for text widget */
    int mapDelete;		/* whether to map delete to backspace */
    char tagFile[MAXPATHLEN];	/* name of tags file to look for at startup */
    char shell[MAXPATHLEN];	/* shell to use for executing commands */
} PrefData;

/* preference descriptions for SavePreferences and RestorePreferences.  Note
   that shellCommands must come first, since the address of the buffer where
   the string is written is added at runtime. */
static PrefDescripRec PrefDescrip[] = {
#ifndef VMS
    {"shellCommands", "ShellCommands", PREF_STRING, "spell:Alt+L:l:ID:\n\
    	(cat;echo) | spell\nwc::w:ID:\nwc\nsort::s:I:\nsort\n\
	make:Alt+M:m:W:\nmake\nexpand::e:I:\nexpand\nunexpand::u:I:\nunexpand\n",
    	NULL, (void *)MAX_SHELL_CMDS_LEN, True},
#endif /* VMS */
    {"wrapText", "WrapText", PREF_BOOLEAN, "False",
    	&PrefData.wrapText, NULL, True},
    {"autoIndent", "AutoIndent", PREF_BOOLEAN, "True",
    	&PrefData.autoIndent, NULL, True},
    {"autoSave", "AutoSave", PREF_BOOLEAN, "True",
    	&PrefData.autoSave, NULL, True},
    {"searchDialogs", "SearchDialogs", PREF_BOOLEAN, "True",
    	&PrefData.searchDlogs, NULL, True},
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
    {"textFont", "TextFont", PREF_STRING,
    	"-adobe-courier-medium-r-normal--14-*-*-*-*-*-*-*",
    	PrefData.fontString, (void *)sizeof(PrefData.fontString), True},
    {"shell", "Shell", PREF_STRING, "/bin/csh",
    	PrefData.shell, (void *)sizeof(PrefData.shell), False},
    {"remapDeleteKey", "RemapDeleteKey", PREF_BOOLEAN, "True",
    	&PrefData.mapDelete, NULL, False},
    {"tagFile", "TagFile", PREF_STRING,
    	"", PrefData.tagFile, (void *)sizeof(PrefData.tagFile), False},
};

static XrmOptionDescRec OpTable[] = {
    {"-wrap", ".wrapText", XrmoptionNoArg, (caddr_t)"True"},
    {"-nowrap", ".wrapText", XrmoptionNoArg, (caddr_t)"False"},
    {"-autoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"True"},
    {"-noautoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"False"},
    {"-autosave", ".autoSave", XrmoptionNoArg, (caddr_t)"True"},
    {"-noautosave", ".autoSave", XrmoptionNoArg, (caddr_t)"False"},
    {"-rows", ".textRows", XrmoptionSepArg, (caddr_t)NULL},
    {"-columns", ".textCols", XrmoptionSepArg, (caddr_t)NULL},
    {"-tabs", ".tabDistance", XrmoptionSepArg, (caddr_t)NULL},
    {"-font", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
    {"-fn", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
};

static char HeaderText[] = "\
# Preferences file for NEdit\n\
#\n\
# This file is overwritten by the \"Save Preferences...\" command in NEdit \n\
# and serves only the interactively setable options presented in the NEdit\n\
# \"Preferences\" menu.  To modify other options, such as background colors\n\
# and key bindings, use the .Xdefaults file in your home directory (or\n\
# the X resource specification method appropriate to your system).  The\n\
# contents of this file can be moved into an X resource file, but since\n\
# resources in this file override their corresponding X resources, either\n\
# this file should be deleted or individual resource lines in the file\n\
# should be deleted for the moved lines to take effect.\n";

/* module-global variables to support Initial Window Size... dialog */
static int DoneWithSizeDialog;
static Widget RowText, ColText;

static void sizeOKCB(Widget w, caddr_t client_data, caddr_t call_data);
static void sizeCancelCB(Widget w, caddr_t client_data, caddr_t call_data);

XrmDatabase CreateNEditPrefDB(int *argcInOut, char **argvInOut)
{
    return CreatePreferencesDatabase(PREF_FILE_NAME, APP_NAME, 
	    OpTable, XtNumber(OpTable), (unsigned int *)argcInOut, argvInOut);
}

void RestoreNEditPrefs(XrmDatabase prefDB, XrmDatabase appDB)
{
#ifndef VMS
    char shellCmdsStr[MAX_SHELL_CMDS_LEN];
    
    PrefDescrip[0].valueAddr = shellCmdsStr;
    RestorePreferences(prefDB, appDB, APP_NAME,
    	    APP_CLASS, PrefDescrip, XtNumber(PrefDescrip));
    LoadFilterListString(shellCmdsStr);
#else
    RestorePreferences(prefDB, appDB, APP_NAME,
    	    APP_CLASS, PrefDescrip, XtNumber(PrefDescrip));
#endif /* VMS */
}

void SaveNEditPrefs(Widget parent)
{
#ifndef VMS
    char *shellCmdsStr = WriteFilterListString();

    PrefDescrip[0].valueAddr = shellCmdsStr;
#endif /* VMS */
    if (SavePreferences(XtDisplay(parent), PREF_FILE_NAME, HeaderText,
    	    PrefDescrip, XtNumber(PrefDescrip)))
    	DialogF(DF_INF, parent, 1, 
#ifdef VMS
		"Default preferences saved in SYS$LOGIN:" PREF_FILE_NAME "\n\
NEdit automatically loads this file each\ntime it is started.", "OK");
#else
    		"Default preferences saved in $HOME/" PREF_FILE_NAME "\n\
NEdit automatically loads this file each\ntime it is started.", "OK");
#endif /*VMS*/
    else
    	DialogF(DF_WARN, parent, 1,
#ifdef VMS
    		"Unable to save preferences in SYS$LOGIN:" PREF_FILE_NAME "", "OK");
#else
    		"Unable to save preferences in $HOME/" PREF_FILE_NAME "", "OK");
#endif /*VMS*/

#ifndef VMS
    XtFree(shellCmdsStr);
#endif
}

void SetPrefWrap(int state)
{
    PrefData.wrapText = state;
}

int GetPrefWrap()
{
    return PrefData.wrapText;
}

void SetPrefSearch(int searchType)
{
    PrefData.searchMethod = searchType;
}

int GetPrefSearch()
{
    return PrefData.searchMethod;
}

void SetPrefAutoIndent(int state)
{
    PrefData.autoIndent = state;
}

int GetPrefAutoIndent()
{
    return PrefData.autoIndent;
}

void SetPrefAutoSave(int state)
{
    PrefData.autoSave = state;
}

int GetPrefAutoSave()
{
    return PrefData.autoSave;
}

void SetPrefSearchDlogs(int state)
{
    PrefData.searchDlogs = state;
}

int GetPrefSearchDlogs()
{
    return PrefData.searchDlogs;
}

void SetPrefStatsLine(int state)
{
    PrefData.statsLine = state;
}

int GetPrefStatsLine()
{
    return PrefData.statsLine;
}

void SetPrefMapDelete(int state)
{
    PrefData.mapDelete = state;
}

int GetPrefMapDelete()
{
    return PrefData.mapDelete;
}

void SetPrefRows(int nRows)
{
    PrefData.textRows = nRows;
}

int GetPrefRows()
{
    return PrefData.textRows;
}

void SetPrefCols(int nCols)
{
   PrefData.textCols = nCols;
}

int GetPrefCols()
{
    return PrefData.textCols;
}

void SetPrefTabDist(int tabDist)
{
    PrefData.tabDist = tabDist;
}

int GetPrefTabDist()
{
    return PrefData.tabDist;
}

void SetPrefTagFile(char *tagFileName)
{
    strcpy(PrefData.tagFile, tagFileName);
}

char *GetPrefTagFile()
{
    return PrefData.tagFile;
}

void SetPrefFont(char *fontName)
{
    strcpy(PrefData.fontString, fontName);
}

char *GetPrefFontName()
{
    return PrefData.fontString;
}

void SetPrefShell(char *shell)
{
    strcpy(PrefData.shell, shell);
}

char *GetPrefShell()
{
    return PrefData.shell;
}

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
    XtVaSetValues(XtParent(selBox), XmNtitle, "Initial Window Size", NULL);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, NULL);

    topLabel = XtVaCreateManagedWidget("topLabel",
      xmLabelGadgetClass, form,
    	XmNlabelString, s1=MKSTRING("Enter desired size in rows\nand columns of characters:"),
      NULL);
    XmStringFree(s1);
 
    RowText = XtVaCreateManagedWidget("rows", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 5,
    	    XmNrightPosition, 45, NULL);
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
    	    XmNrightPosition, 55, NULL);
    XmStringFree(s1);

    ColText = XtVaCreateManagedWidget("cols", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 55,
    	    XmNrightPosition, 95, NULL);
    RemapDeleteKey(ColText);

    /* put up dialog and wait for user to press ok or cancel */
    DoneWithSizeDialog = False;
    XtManageChild(selBox);
    while (!DoneWithSizeDialog)
        XtAppProcessEvent (XtWidgetToApplicationContext(parent), XtIMAll);
    
    
    XtDestroyWidget(selBox);
}

static void sizeOKCB(Widget w, caddr_t client_data, caddr_t call_data)
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

static void sizeCancelCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    DoneWithSizeDialog = True;
}


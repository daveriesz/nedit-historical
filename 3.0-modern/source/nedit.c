/*******************************************************************************
*									       *
* nedit.c -- Nirvana Editor main program				       *
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
* Modifications:							       *
*									       *
*	8/18/93 - Mark Edel & Joy Kyriakopulos - Ported to VMS		       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)nedit.c	1.12     3/15/94";
#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#ifdef VMS
#include <rmsdef.h>
#include "../util/VMSparam.h"
#include "../util/VMSUtils.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include "../util/misc.h"
#include "../util/printUtils.h"
#include "nedit.h"
#include "file.h"
#include "preferences.h"
#include "tags.h"
#ifdef CODE_CENTER_NEDIT
#include "../ccnedit/ccNEdit.h"
#endif
#include "fileUtils.h"

WindowInfo *WindowList = NULL;
Display *TheDisplay;

static char *fallbackResources[] = {
    "*text.background: white",
    "*text.foreground: black",
    "*fileMenu.new.accelerator: Ctrl<Key>n",
    "*fileMenu.new.acceleratorText: Ctrl+N",
    "*fileMenu.open.accelerator: Ctrl<Key>o",
    "*fileMenu.open.acceleratorText: Ctrl+O",
    "*fileMenu.openSelected.accelerator: Ctrl<Key>q",
    "*fileMenu.openSelected.acceleratorText: Ctrl+Q",
    "*fileMenu.close.accelerator: Ctrl<Key>w",
    "*fileMenu.close.acceleratorText: Ctrl+W",
    "*fileMenu.save.accelerator: Ctrl<Key>s",
    "*fileMenu.save.acceleratorText: Ctrl+S",
    "*fileMenu.includeFile.accelerator: Ctrl<Key>i",
    "*fileMenu.includeFile.acceleratorText: Ctrl+I",
    "*fileMenu.print.accelerator: Ctrl<Key>p",
    "*fileMenu.print.acceleratorText: Ctrl+P",
    "*fileMenu.exit.accelerator: <Key>F3:",
    "*fileMenu.exit.acceleratorText: F3",
    "*editMenu.undo.accelerator: Alt<Key>BackSpace:",
    "*editMenu.undo.acceleratorText: Alt+Backspace",
    "*editMenu.redo.accelerator: Alt<Key>R:",
    "*editMenu.redo.acceleratorText: Alt+R",
    "*editMenu.cut.accelerator: Shift<Key>Delete:",
    "*editMenu.cut.acceleratorText: Shift+Del",
    "*editMenu.copy.accelerator: Ctrl<Key>Insert:",
    "*editMenu.copy.acceleratorText: Ctrl+Ins",
    "*editMenu.paste.accelerator: Shift<Key>Insert:",
    "*editMenu.paste.acceleratorText: Shift+Ins",
    "*editMenu.clear.acceleratorText: Del",
    "*editMenu.shiftLeft.accelerator: Ctrl<Key>9:",
    "*editMenu.shiftLeft.acceleratorText: [Shift]Ctrl+9",
    "*editMenu.shiftLeftShift.accelerator: Shift Ctrl<Key>9",
    "*editMenu.shiftRight.accelerator: Ctrl<Key>0:",
    "*editMenu.shiftRight.acceleratorText: [Shift]Ctrl+0",
    "*editMenu.shiftRightShift.accelerator: Shift Ctrl<Key>0",
    "*editMenu.capitalize.accelerator: Ctrl<Key>6",
    "*editMenu.capitalize.acceleratorText: Ctrl+6",
    "*editMenu.lowerCase.accelerator: Shift Ctrl<Key>6",
    "*editMenu.lowerCase.acceleratorText: Shift+Ctrl+6",
    "*editMenu.fillSelection.accelerator: Ctrl<Key>j",
    "*editMenu.fillSelection.acceleratorText: Ctrl+J",
    "*searchMenu.find.accelerator: Ctrl<Key>f",
    "*searchMenu.find.acceleratorText: [Shift]Ctrl+F",
    "*searchMenu.findShift.accelerator: Shift Ctrl<Key>f",
    "*searchMenu.findSame.accelerator: Ctrl<Key>g",
    "*searchMenu.findSame.acceleratorText: [Shift]Ctrl+G",
    "*searchMenu.findSameShift.accelerator: Shift Ctrl<Key>g",
    "*searchMenu.findSelection.accelerator: Ctrl<Key>h",
    "*searchMenu.findSelection.acceleratorText: [Shift]Ctrl+H",
    "*searchMenu.findSelectionShift.accelerator: Shift Ctrl<Key>h",
    "*searchMenu.replace.accelerator: Ctrl<Key>r",
    "*searchMenu.replace.acceleratorText: [Shift]Ctrl+R",
    "*searchMenu.replaceShift.accelerator: Shift Ctrl<Key>r",
    "*searchMenu.replaceSame.accelerator: Ctrl<Key>t",
    "*searchMenu.replaceSame.acceleratorText: [Shift]Ctrl+T",
    "*searchMenu.replaceSameShift.accelerator: Shift Ctrl<Key>t",
    "*searchMenu.gotoLine.accelerator: Ctrl<Key>l",
    "*searchMenu.gotoLine.acceleratorText: Ctrl+L",
    "*searchMenu.gotoSelected.accelerator: Ctrl<Key>e",
    "*searchMenu.gotoSelected.acceleratorText: Ctrl+E",
    "*searchMenu.match.accelerator: Ctrl<Key>m",
    "*searchMenu.match.acceleratorText: Ctrl+M",
    "*searchMenu.findDefinition.accelerator: Ctrl<Key>d",
    "*searchMenu.findDefinition.acceleratorText: Ctrl+D",
    "*shellMenu.filterSelection.accelerator: Ctrl<Key>k",
    "*shellMenu.filterSelection.acceleratorText: Ctrl+K",
    "*shellMenu.executeCommand.accelerator: Alt<Key>k",
    "*shellMenu.executeCommand.acceleratorText: Alt+K",
    "*shellMenu.executeCommandLine.accelerator: <Key>KP_Enter",
    "*shellMenu.executeCommandLine.acceleratorText: KP Enter",
    "*preferencesMenu.statisticsLine.accelerator: Ctrl<Key>a",
    "*preferencesMenu.statisticsLine.acceleratorText: Ctrl+A",
    "*windowsMenu.splitWindow.accelerator: Ctrl<Key>2",
    "*windowsMenu.splitWindow.acceleratorText: Ctrl+2",
    "*windowsMenu.closePane.accelerator: Ctrl<Key>1",
    "*windowsMenu.closePane.acceleratorText: Ctrl+1",
    0
};

static char cmdLineHelp[] =
#ifndef VMS
"Usage:  nedit [-tags file] [-tabs n] [-wrap] [-nowrap] [-autoindent]\n\
               [-noautoindent] [-autosave] [-noautosave] [-rows n]\n\
               [-columns n] [-font font] [-display [host]:server[.screen]\n\
               [-geometry geometry] [-xrm resourcestring] [file...]\n";
#else
"";
#endif /*VMS*/

int main(int argc, char **argv)
{
    int i, fileSpecified;
    XtAppContext context;
    XrmDatabase prefDB;

    /* Initialize toolkit and open display. */
    XtToolkitInitialize();
    context = XtCreateApplicationContext();
    
    /* Set up a warning handler to trap obnoxious Xt grab warnings */
    SuppressPassiveGrabWarnings();

    /* Set up default resources if no app-defaults file is found */
    XtAppSetFallbackResources(context, fallbackResources);
    
#ifdef VMS
    /* Convert the command line to Unix style (This is not an ideal solution) */
    ConvertVMSCommandLine(&argc, &argv);
#endif /*VMS*/
    
    /* Read the preferences file and command line into a database */
    prefDB = CreateNEditPrefDB(&argc, argv);

    /* Open the display and read X database and remaining command line args */
    TheDisplay = XtOpenDisplay (context, NULL, APP_NAME, APP_CLASS, NULL,
    	    0, &argc, argv);
    if (!TheDisplay) {
	XtWarning ("NEdit: Can't open display\n");
	exit(0);
    }

#ifdef CODE_CENTER_NEDIT
    /* Set up communication port and write ~/.ccNEditPortNumber file */
    InitCCCommunication();
#endif

    /* Store preferences from the command line and .nedit-3.0 file, 
       and set the appropriate preferences */
    RestoreNEditPrefs(prefDB, XtDatabase(TheDisplay));
    LoadPrintPreferences(XtDatabase(TheDisplay), APP_NAME, APP_CLASS, True);
    SetDeleteRemap(GetPrefMapDelete());
    
    /* Process any command line arguments (-tags, and files to edit) not
       already processed by RestoreNEditPrefs. */
    fileSpecified = FALSE;
    for (i=1; i<argc; i++) {
    	if (!strcmp(argv[i], "-tags")) {
    	    if (++i >= argc) {
    	        fprintf(stderr, "NEdit: tags requires an argument\n%s",
    	        	cmdLineHelp);
    	    	exit(0);
	    }
    	    if (!LoadTagsFile(argv[i]))
    	    	fprintf(stderr, "NEdit: Unable to load tags file\n");
    	} else if (*argv[i] == '-') {
#ifdef VMS
	    *argv[i] = '/';
#endif /*VMS*/
    	    fprintf(stderr, "NEdit: Unrecognized option %s\n%s", argv[i],
    	    	    cmdLineHelp);
    	    exit(0);
    	} else {
	    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
#ifdef VMS
	    int numFiles, j;
	    char **nameList = NULL;
	    /* Use VMS's LIB$FILESCAN for filename in argv[i] to process */
	    /* wildcards and to obtain a full VMS file specification     */
	    numFiles = VMSFileScan(argv[i], &nameList, NULL, INCLUDE_FNF);
	    /* for each expanded file name do: */
	    for (j = 0; j < numFiles; ++j) {
	    	ParseFilename(nameList[j], filename, pathname);
		EditExistingFile(WindowList, filename, pathname, TRUE);
		fileSpecified = TRUE;
		free(nameList[j]);
	    }
	    if (nameList != NULL)
	    	free(nameList);
#else
	    ParseFilename(argv[i], filename, pathname);
	    EditExistingFile(WindowList, filename, pathname, TRUE);
	    fileSpecified = TRUE;
#endif /*VMS*/
	}
    }
    
#ifdef VMS
    VMSFileScanDone();
#endif /*VMS*/
    
    /* Load the default tags file (as long as -tags was not specified).
       Don't complain if it doesn't load, the tag file resource is
       intended to be set and forgotten.  Running nedit in a directory
       without a tags should not cause it to spew out errors. */
    if (!TagsFileLoaded() && *GetPrefTagFile() != '\0')
    	LoadTagsFile(GetPrefTagFile());
    
    /* If no file to edit was specified, open a window to edit "Untitled" */
    if (!fileSpecified)
    	EditNewFile();
    
    /* Process events. */
#ifdef CODE_CENTER_NEDIT
    ccNEditMainLoop(TheDisplay, context);
#else
    XtAppMainLoop(context);
#endif
}

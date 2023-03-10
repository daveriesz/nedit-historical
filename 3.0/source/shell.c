/*******************************************************************************
*									       *
* shell.c (formerly filter.c) -- Nirvana Editor shell command execution	       *
*									       *
* Copyright (c) 1991 Universities Research Association, Inc.		       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retaFins a paid-up,     *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warranty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* December, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)shell.c	1.3     3/8/94";
#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#ifdef IBM
#define NBBY 8
#include <sys/select.h>
#endif
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <Xm/Xm.h>
#include <X11/keysym.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/DrawingA.h>
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "nedit.h"
#include "shift.h"
#include "window.h"
#include "file.h"
#include "menu.h"
#include "preferences.h"
#include "shell.h"

#define FILTER_BUF_SIZE 256	/* size of buffers for collecting cmd output */
#define MAX_FILTER_ITEM_LEN 40	/* max length for a filter menu item */
#define MAX_FILTER_CMD_LEN 255	/* max length of a filter command */
#define MAX_FILTER_ITEMS 200	/* max number of filter commands allowed */
#define MAX_FILTER_ACC_LEN 50	/* max length of an accelerator string */
#define MAX_OUT_DIALOG_ROWS 30	/* max height of dialog for command output */
#define MAX_OUT_DIALOG_COLS 80	/* max width of dialog for command output */

/* flags for issueCommand */
#define ACCUMULATE 1
#define ERROR_DIALOGS 2

/* destinations for command output */
enum outDests {TO_SAME_WINDOW, TO_NEW_WINDOW, TO_DIALOG};

/* element of a buffer list for collecting output from filter processes */
typedef struct bufElem {
    struct bufElem *next;
    int length;
    char contents[FILTER_BUF_SIZE];
} buffer;

typedef struct {
    char name[MAX_FILTER_ITEM_LEN];
    unsigned int modifiers;
    KeySym keysym;
    char mnemonic;
    char selInput;
    char output;
    char cmd[MAX_FILTER_CMD_LEN];
} filterListRec;

/* Widgets and flags associated with the filter editing dialog */
static int DoneWithFilterDialog = False;
static Widget EditFilterDlog = NULL;
static Widget NameTextW, AccTextW, MneTextW, CmdTextW, ListW;
static Widget SelInpBtn, DlogOutBtn, WinOutBtn;
static Widget AddBtn, ChangeBtn, DeleteBtn, MoveUpBtn, MoveDownBtn;

/* The list of user programmed items for the shell menu */
static filterListRec *FilterList[MAX_FILTER_ITEMS];
static int NFilters = 0;

/* Saved copy of the state of the filter list (above) for undoing the last
   command */
static filterListRec *SavedFilterList[MAX_FILTER_ITEMS];
static int NSavedFilters = 0;
static int SavedSelection = 1;

static void dlogOutCB(Widget w, caddr_t client_data, caddr_t call_data);
static void winOutCB(Widget w, caddr_t client_data, caddr_t call_data);
static void addCB(Widget w, caddr_t client_data, caddr_t call_data);
static void changeCB(Widget w, caddr_t client_data, caddr_t call_data);
static void deleteCB(Widget w, caddr_t client_data, caddr_t call_data);
static void moveUpCB(Widget w, caddr_t client_data, caddr_t call_data);
static void moveDownCB(Widget w, caddr_t client_data, caddr_t call_data);
static void undoCB(Widget w, caddr_t client_data, caddr_t call_data);
static void dismissCB(Widget w, caddr_t client_data, caddr_t call_data);
static void accKeyCB(Widget w, XtPointer client_data, XKeyEvent *event);
static void destroyCB(Widget w, caddr_t client_data, caddr_t call_data);
static void filterMenuCB(Widget w, WindowInfo *window, caddr_t call_data);
static void listSelectionCB(Widget w, caddr_t client_data,
	XmListCallbackStruct *call_data);
static void updateFilterDialog(int selection);
static void updateFilterDlogFields(void);
static void saveFilterList(void);
static void restoreFilterList(void);
static int readFilterDialogFields(filterListRec *f);
static void disableTextW(Widget textW);
static void makeSingleLineTextW(Widget textW);
static int selectedListPosition(Widget listW);
static char *issueCommand(Widget dlogParent, char *command, char *input,
	int flags, Widget textW, int replaceLeft, int replaceRight);
static pid_t forkCommand(Widget parent, char *command, int *stdinFD,
	int *stdoutFD, int *stderrFD);
static Widget createWorkingDialog(Widget parent, int *abortFlag);
static void cancelCB(Widget w, int *abortFlag, caddr_t call_data);
static void addOutput(buffer **bufList, buffer *buf);
static char *coalesceOutput(buffer **bufList);
static void freeBufList(buffer **bufList);
static void removeTrailingNewlines(char *string);
static void generateAcceleratorString(char *text, unsigned int modifiers,
	KeySym keysym);
static void genAccelEventName(char *text, unsigned int modifiers,
	KeySym keysym);
static int parseAcceleratorString(char *string, unsigned int *modifiers,
	KeySym *keysym);
static int parseError(char *message);
static void createOutputDialog(Widget parent, char *text);
static void measureText(char *text, int wrapWidth, int *rows, int *cols);
static void truncateString(char *string, int length);

/*
** Prompt the user for a shell command to filter the current selection through.
** The selection is removed, and replaced by the output from the command.
** Failed command status and output to stderr are presented in dialog form.
*/
void FilterSelection(WindowInfo *window)
{
    int resp;
    char cmdText[DF_MAX_PROMPT_LENGTH];
    char *text;
    XmTextPosition left, right;

    /* Get the selection and the range in character positions that it
       occupies.  Beep and return if no selection */
    text = XmTextGetSelection(window->textArea);
    if (text == NULL) {
	XBell(TheDisplay, 100);
	return;
    } else if (*text == '\0') {
    	XBell(TheDisplay, 100);
    	XtFree(text);
    	return;
    }
    XmTextGetSelectionPosition(window->textArea, &left, &right);

    /* Prompt the user for the shell command to execute */
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Enter shell command for filtering selection:",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    
    /* issue the command and collect its output */
    issueCommand(window->shell, cmdText, text, ACCUMULATE | ERROR_DIALOGS,
    	    window->lastFocus, left, right);

    XtFree(text);
}

/*
** Prompt the user for a shell command to execute, and execute it, depositing
** the result at the current insert position or in the current selection if
** if the window has a selection.
*/
void ExecShellCommand(WindowInfo *window)
{
    int resp;
    char cmdText[DF_MAX_PROMPT_LENGTH];
    XmTextPosition left, right;

    /* Prompt the user for the shell command to execute */
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Enter shell command to execute:",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    
    /* get the selection or the insert position */
    if (!XmTextGetSelectionPosition(window->textArea, &left, &right))
    	left = right = XmTextGetInsertionPosition(window->lastFocus);
    
    /* issue the command and collect its output */
    issueCommand(window->shell, cmdText, "", 0, window->lastFocus, left, right);
}

/*
** Execute the line of text where the the insertion cursor is positioned
** as a shell command.
*/
void ExecCursorLine(WindowInfo *window)
{
    char *cmdText;
    XmTextPosition left, right;

    /* get all of the text on the line with the insert position */
    left = right = XmTextGetInsertionPosition(window->lastFocus);
    if (!ExtendToWholeLines(window->lastFocus, &left, &right))
    	return;
    cmdText = GetTextRange(window->textArea, left, right);
    
    /* insert a newline after the entire line */
    XmTextInsert(window->textArea, right, "\n");

    /* issue the command */
    issueCommand(window->shell, cmdText, "", 0, window->lastFocus,
    	   right+1, right+1);

    XtFree(cmdText);
}

/*
** Present a dialog for editing the user specified commands in the shell menu
*/
void EditShellMenu(WindowInfo *window)
{
    Widget form, accLabel;
    Widget nameLabel, cmdLabel, button;
    XmString s1;
    int ac;
    Arg args[20];

    /* if the dialog is already displayed, just pop it to the top and return */
    if (EditFilterDlog != NULL) {
    	XMapRaised(TheDisplay, XtWindow(EditFilterDlog));
    	return;
    }
    
    ac = 0;
    XtSetArg(args[ac], XmNautoUnmanage, False); ac++;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_NONE); ac++;
    form = XmCreateFormDialog(window->shell, "editFilters", args, ac);
    EditFilterDlog = XtParent(form);
    XtVaSetValues(EditFilterDlog, XmNtitle, "Shell Commands", 0);
    XtAddCallback(form, XmNdestroyCallback, (XtCallbackProc)destroyCB, NULL);
    AddMotifCloseCallback(XtParent(form), (XtCallbackProc)dismissCB, NULL);
 
    ac = 0;
    XtSetArg(args[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
    XtSetArg(args[ac], XmNcolumns, 28); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNtopPosition, 2); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 50); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, 99); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNbottomPosition, 67); ac++;
    ListW = XmCreateScrolledList(form, "list", args, ac);
    XtManageChild(ListW);
    XtAddCallback(ListW, XmNbrowseSelectionCallback,
    	    (XtCallbackProc)listSelectionCB, NULL);

    WinOutBtn = XtVaCreateManagedWidget("winOutBtn", xmToggleButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Output to new window"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_POSITION,
	    XmNbottomPosition, 67, 0);
    XmStringFree(s1);
    XtAddCallback(WinOutBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)winOutCB, NULL);

    DlogOutBtn = XtVaCreateManagedWidget("dlogOutBtn",
    	    xmToggleButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Output to dialog"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, WinOutBtn, 0);
    XmStringFree(s1);
    XtAddCallback(DlogOutBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)dlogOutCB, NULL);

    SelInpBtn = XtVaCreateManagedWidget("selInpBtn", xmToggleButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Use Selection as command input"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, True,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, DlogOutBtn, 0);
    XmStringFree(s1);
 
    MneTextW = XtVaCreateManagedWidget("mne", xmTextWidgetClass, form,
	    XmNcolumns, 1,
	    XmNmaxLength, 1,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 40,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, SelInpBtn, 0);
    RemapDeleteKey(MneTextW);

    AccTextW = XtVaCreateManagedWidget("acc", xmTextWidgetClass, form,
    	    XmNcolumns, 12,
    	    XmNmaxLength, MAX_FILTER_ACC_LEN-1,
    	    XmNcursorPositionVisible, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 35,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, SelInpBtn, 0);
    XtAddEventHandler(AccTextW, KeyPressMask, False,
    	    (XtEventHandler)accKeyCB, NULL);
 
    accLabel = XtVaCreateManagedWidget("accLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Accelerator"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 25,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, MneTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("mneLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Mnemonic"),
    	    XmNalignment, XmALIGNMENT_END,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 25,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 49,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, MneTextW, 0);
    XmStringFree(s1);
    
    NameTextW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNmaxLength, MAX_FILTER_ITEM_LEN-1,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, accLabel, 0);
    RemapDeleteKey(NameTextW);
 
    nameLabel = XtVaCreateManagedWidget("nameLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Menu Entry"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 49,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, NameTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
"Select a shell menu item to\n\
edit from the list at right ->\n\
or select \"New\" to add a new\n\
command to the menu"),
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, nameLabel, 0);
    XmStringFree(s1);
 
    cmdLabel = XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Shell Command to Execute"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNtopAttachment, XmATTACH_POSITION,
    	    XmNtopPosition, 67,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 99, 0);
    XmStringFree(s1);

    AddBtn = XtVaCreateManagedWidget("add", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Add"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 15,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(AddBtn, XmNactivateCallback, (XtCallbackProc)addCB, NULL);
    XmStringFree(s1);

    ChangeBtn = XtVaCreateManagedWidget("chg", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Change"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 15,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 29,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ChangeBtn, XmNactivateCallback, (XtCallbackProc)changeCB,
    	    NULL);
    XmStringFree(s1);

    DeleteBtn = XtVaCreateManagedWidget("del", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Delete"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 29,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 43,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(DeleteBtn, XmNactivateCallback, (XtCallbackProc)deleteCB,
    	    NULL);
    XmStringFree(s1);

    MoveUpBtn = XtVaCreateManagedWidget("mvUp", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Move ^"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 43,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 57,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(MoveUpBtn, XmNactivateCallback, (XtCallbackProc)moveUpCB,
    	    NULL);
    XmStringFree(s1);

    MoveDownBtn = XtVaCreateManagedWidget("mvDn", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Move v"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 57,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 71,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(MoveDownBtn, XmNactivateCallback, (XtCallbackProc)moveDownCB,
    	    NULL);
    XmStringFree(s1);

    button = XtVaCreateManagedWidget("undo", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Undo"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 71,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 85,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)undoCB,
    	    NULL);
    XmStringFree(s1);

    button = XtVaCreateManagedWidget("dismiss", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Dismiss"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 85,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 99,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)dismissCB, NULL);
    XmStringFree(s1);
    
    ac = 0;
    XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
    XtSetArg(args[ac], XmNmaxLength, MAX_FILTER_CMD_LEN-1); ac++;
    XtSetArg(args[ac], XmNscrollHorizontal, False); ac++;
    XtSetArg(args[ac], XmNwordWrap, True); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNtopWidget, cmdLabel); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 1); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, 99); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomWidget, button); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
    CmdTextW = XmCreateScrolledText(form, "name", args, ac);
    XtManageChild(CmdTextW);
    makeSingleLineTextW(CmdTextW);
    RemapDeleteKey(CmdTextW);
   
    /* Disable text input for the accelerator key widget, let the
       event handler manage it instead */
    disableTextW(AccTextW);

    /* initializs the list of menu items in the list widget */
    updateFilterDialog(1);
    
    /* Set initial default button */
    XtVaSetValues(form, XmNdefaultButton, AddBtn, 0);
    
    /* put up dialog and wait for user to press ok or cancel */
    DoneWithFilterDialog = False;
    XtManageChild(form);
    while (!DoneWithFilterDialog)
        XtAppProcessEvent (XtWidgetToApplicationContext(form), XtIMAll);
    
    XtDestroyWidget(form);
}

void UpdateFilterMenus()
{
    WindowInfo *w;

    for (w=WindowList; w!=NULL; w=w->next)
	UpdateFilterMenu(w);
}

void UpdateFilterMenu(WindowInfo *window)
{
    Widget btn;
    WidgetList items;
    int nItems, n, userData;
    XmString st1, st2;
    filterListRec *f;
    char accText[MAX_FILTER_ACC_LEN], accKeys[MAX_FILTER_ACC_LEN+5];
        
    /* Remove all of the existing filters from the menu */
    XtVaGetValues(window->filterMenuPane, XmNchildren, &items,
    	    XmNnumChildren, &nItems,0);
    for (n=0; n<nItems; n++) {
    	XtVaGetValues(items[n], XmNuserData, &userData, 0);
    	if (userData != PERMANENT_MENU_ITEM) {
    	    /* remove accel. before destroying or it will be lost forever */
    	    XtVaSetValues(items[n], XmNaccelerator, NULL, 0);
    	    /* unmanaging before destroying stops parent from displaying */
    	    XtUnmanageChild(items[n]);
    	    XtDestroyWidget(items[n]);
    	}
    }
    
    /* Add current filters from FilterList to the menu */
    for (n=0; n<NFilters; n++) {
    	f = FilterList[n];
    	generateAcceleratorString(accText, f->modifiers, f->keysym);
    	genAccelEventName(accKeys, f->modifiers, f->keysym);
    	btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
    		window->filterMenuPane, 
    		XmNlabelString, st1=XmStringCreateSimple(f->name),
    		XmNacceleratorText, st2=XmStringCreateSimple(accText),
    		XmNaccelerator, accKeys,
    		XmNmnemonic, f->mnemonic,
    		XmNuserData, n+10, NULL);
	XtAddCallback(btn, XmNactivateCallback, (XtCallbackProc)filterMenuCB,
		window);
    	XmStringFree(st1);
    	XmStringFree(st2);
    }
}

static void dlogOutCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    if (XmToggleButtonGetState(w))
    	XmToggleButtonSetState(WinOutBtn, False, False);
}

static void winOutCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    if (XmToggleButtonGetState(w))
    	XmToggleButtonSetState(DlogOutBtn, False, False);
}

static void addCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    int i, listPos;
    filterListRec *f;
    
    /* build a filter list entry the name, accelerator, and mnemonic that
       the user entered in the dialog */
    f = (filterListRec *)XtMalloc(sizeof(filterListRec));
    if (!readFilterDialogFields(f)) {
    	XtFree((char *)f);
    	return;
    }
        
    /* get the item number currently selected in the filter list */
    listPos = selectedListPosition(ListW);

    /* save the existing state of the filter list for undo */
    saveFilterList();
        
    /* add the item to the filter list */
    for (i=NFilters; i>=listPos; i--)
    	FilterList[i] = FilterList[i-1];
    FilterList[listPos-1] = f;
    NFilters++;
    
    /* update the filter menus in all windows */
    UpdateFilterMenus();
    
    /* redisplay the list widget and select the new item */
    updateFilterDialog(listPos+1);
}

static void changeCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    int listPos;
    filterListRec *f;
        
    /* get the item currently selected in the filter list */
    listPos = selectedListPosition(ListW);
    if (listPos < 2 || listPos > NFilters + 1)
    	return;					/* shouldn't happen */
    f = FilterList[listPos-2];

    /* save the existing state of the filter list for undo */
    saveFilterList();
     
    /* modify the filter list entry using the name, accelerator, and mnemonic
       text fields from the dialog */
    if (!readFilterDialogFields(f))
    	return;
    
    /* update the filter menus in all windows */
    UpdateFilterMenus();
    
    /* update the item name in the list widget */
    updateFilterDialog(listPos);
}

static void deleteCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    int i, index, listPos;
    filterListRec *f;
        
    /* get the selected list position and the item to be deleted */
    listPos = selectedListPosition(ListW);
    if (listPos < 2 || listPos > NFilters + 1)
    	return;					/* shouldn't happen */
    index = listPos-2;
    f = FilterList[index];

    /* save the existing state of the filter list for undo */
    saveFilterList();
     
    /* free the item and remove it from the list */
    XtFree((char *)f);
    for (i=index; i<NFilters-1; i++)
    	FilterList[i] = FilterList[i+1];
    NFilters--;
    
    /* update the filter menus in all windows */
    UpdateFilterMenus();
    
    /* update the list widget and move the selection to the previous item
       in the list and display the fields appropriate  for that entry */
    updateFilterDialog(index+1);
}

static void moveUpCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    int index, listPos;
    filterListRec *temp;
        
    /* get the item index currently selected in the filter list */
    listPos = selectedListPosition(ListW);
    if (listPos < 3 || listPos > NFilters + 1)
    	return;					/* shouldn't happen */
    index = listPos-2;

    /* save the existing state of the filter list for undo */
    saveFilterList();
     
    /* shuffle the item up in the filter list */
    temp = FilterList[index];
    FilterList[index] = FilterList[index-1];
    FilterList[index-1] = temp;
    
    /* update the filter menus in all windows */
    UpdateFilterMenus();
    
    /* update the list widget and keep the selection on moved item */
    updateFilterDialog(index+1);
}

static void moveDownCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    int index, listPos;
    filterListRec *temp;
        
    /* get the item index currently selected in the filter list */
    listPos = selectedListPosition(ListW);
    if (listPos < 2 || listPos > NFilters)
    	return;				/* shouldn't happen */
    index = listPos-2;

    /* save the existing state of the filter list for undo */
    saveFilterList();
     
    /* shuffle the item down in the filter list */
    temp = FilterList[index];
    FilterList[index] = FilterList[index+1];
    FilterList[index+1] = temp;
    
    /* update the filter menus in all windows */
    UpdateFilterMenus();
    
    /* update the list widget and keep the selection on moved item */
    updateFilterDialog(index+3);
}
    
static void undoCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    restoreFilterList();
}

static void dismissCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    DoneWithFilterDialog = True;
}

static void listSelectionCB(Widget w, caddr_t client_data,
	XmListCallbackStruct *call_data)
{
    if (call_data->item_position == 1) {
    	XtVaSetValues(XtParent(AddBtn), XmNdefaultButton, AddBtn, 0);
    	XtSetSensitive(ChangeBtn, False);
    	XtSetSensitive(DeleteBtn, False);
    	XtSetSensitive(MoveUpBtn, False);
    	XtSetSensitive(MoveDownBtn, False);
    } else {
    	XtVaSetValues(XtParent(ChangeBtn), XmNdefaultButton, ChangeBtn, 0);
    	XtSetSensitive(ChangeBtn, True);
    	XtSetSensitive(DeleteBtn, True);
    	XtSetSensitive(MoveUpBtn, (call_data->item_position != 2));
    	XtSetSensitive(MoveDownBtn, call_data->item_position != NFilters+1);
    }
    updateFilterDlogFields();
}

static void accKeyCB(Widget w, XtPointer client_data, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    char outStr[MAX_FILTER_ACC_LEN];
    
    /* Accept only real keys, not modifiers alone */
    if (IsModifierKey(keysym))
    	return;
    
    /* Tab key means go to the next field, don't enter */
    if (keysym == XK_Tab)
    	return;
    
    /* Beep and return if the modifiers are buttons or ones we don't support */
    if (event->state & ~(ShiftMask | LockMask | ControlMask | Mod1Mask)) {
	XBell(TheDisplay, 100);
	return;
    }
    
    /* Delete or backspace clears field */
    if (keysym == XK_Delete || keysym == XK_BackSpace) {
    	XmTextSetString(AccTextW, "");
    	return;
    }
    
    /* generate the string to use in the dialog field */
    generateAcceleratorString(outStr, event->state, keysym);

    /* Reject single character accelerators (a very simple way to eliminate
       un-modified letters and numbers)  The goal is give users a clue that
       they're supposed to type the actual keys, not the name.  This scheme
       is not rigorous and still allows accelerators like Comma. */
    if (strlen(outStr) == 1) {
    	XBell(TheDisplay, 100);
	return;
    }
    
    /* fill in the accelerator field in the dialog */
    XmTextSetString(AccTextW, outStr);
}

static void destroyCB(Widget w, caddr_t client_data, caddr_t call_data)
{
    EditFilterDlog = NULL;
}

static void filterMenuCB(Widget w, WindowInfo *window, caddr_t call_data) 
{
    int userData, index, flags = 0;
    char *text, *filteredText;
    XmTextPosition left, right;
    Widget outWidget, dialogParent = window->shell;

    /* get the index of the filter command and verify that it's in range */
    XtVaGetValues(w, XmNuserData, &userData, 0);
    index = userData - 10;
    if (index <0 || index >= NFilters)
    	return;
    	
    /* Get the selection and the range in character positions that it
       occupies.  use the insert position if no selection */
    if (FilterList[index]->selInput) {
	if (!XmTextGetSelectionPosition(window->textArea, &left, &right))
    	    left = right = XmTextGetInsertionPosition(window->textArea);
	text = GetTextRange(window->textArea, left, right);
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else
    	text = "";
    
    /* Assign the output destination and create a new window if necessary */
    if (FilterList[index]->output == TO_DIALOG) {
    	outWidget = NULL;
    	left = right = 0;
    } else if (FilterList[index]->output == TO_NEW_WINDOW) {
    	EditNewFile();
    	outWidget = WindowList->textArea;
    	dialogParent = WindowList->shell;
    	left = right = 0;
    } else {
    	outWidget = window->lastFocus;
	if (!XmTextGetSelectionPosition(window->textArea, &left, &right))
    	    left = right = XmTextGetInsertionPosition(window->textArea);
    }
    
    /* issue the command and collect its output */
    filteredText = issueCommand(dialogParent, FilterList[index]->cmd, text,
    	    flags, outWidget, left, right);

    /* if the output is to a dialog, present the dialog with the output */
    if (FilterList[index]->output == TO_DIALOG && filteredText != NULL) {
    	removeTrailingNewlines(filteredText);
	if (*filteredText != '\0')
    	    createOutputDialog(window->shell, filteredText);
    	XtFree(filteredText);
    }
    
    if (FilterList[index]->selInput)
    	XtFree(text);
}

/*
** Update the list in the shell dialog to reflect the current contents of
** the filter list, set the item that should now be highlighted, and update
** the dialog fields to correspond to the newly selected item.
*/
static void updateFilterDialog(int selection)
{
    int i;
    XmString stringTable[MAX_FILTER_ITEMS+1];
    
    /* On many systems under Motif 1.1 the list widget can't handle items
       being changed while anything is selected! */
    XmListDeselectAllItems(ListW);

    /* Fill in the list widget with the names from the filter list */
    stringTable[0] = XmStringCreateSimple("New");
    for (i=0; i<NFilters; i++)
    	stringTable[i+1] = XmStringCreateSimple(FilterList[i]->name);
    XtVaSetValues(ListW, XmNitems, stringTable, XmNitemCount, NFilters+1, 0);
    for (i=0; i<=NFilters; i++)
    	XmStringFree(stringTable[i]);

    /* select the requested item */
    XmListSelectPos(ListW, selection, True);
}

/*
** Update the name, accelerator, mnemonic, and command fields in the filter
** dialog to agree with the currently selected item in the menu item list
*/
static void updateFilterDlogFields(void)
{
    int index, listPos;
    filterListRec *f;
    char mneString[2], accString[MAX_FILTER_ACC_LEN];
        
    /* get the index of the item currently selected in the filter list */
    listPos = selectedListPosition(ListW);
    index = listPos - 2;
    if (index < -1 || index > NFilters - 1)
    	return;					/* shouldn't happen */
    
    /* fill in the name, accelerator, mnemonic, and command fields of the
       dialog for the newly selected item, or blank them if "New" is selected */
    if (index >= 0) {
    	f = FilterList[index];
	mneString[0] = f->mnemonic;
	mneString[1] = '\0';
	generateAcceleratorString(accString, f->modifiers, f->keysym);
	XmTextSetString(NameTextW, f->name);
	XmTextSetString(CmdTextW, f->cmd);
	XmTextSetString(AccTextW, accString);
	XmTextSetString(MneTextW, mneString);
	XmToggleButtonSetState(SelInpBtn, f->selInput, False);
	XmToggleButtonSetState(WinOutBtn, f->output==TO_NEW_WINDOW, False);
	XmToggleButtonSetState(DlogOutBtn, f->output==TO_DIALOG, False);
    } else {
    	XmTextSetString(NameTextW, "");
	XmTextSetString(CmdTextW, "");
	XmTextSetString(AccTextW, "");
	XmTextSetString(MneTextW, "");
	XmToggleButtonSetState(SelInpBtn, True, False);
	XmToggleButtonSetState(WinOutBtn, False, False);
	XmToggleButtonSetState(DlogOutBtn, False, False);
    }
}    

/*
** Read the name, accelerator, mnemonic, and command fields from the shell
** commands dialog into a filterListRec
*/
static int readFilterDialogFields(filterListRec *f)
{
    char *nameText, *cmdText, *mneText, *accText;

    nameText = XmTextGetString(NameTextW);
    if (nameText == NULL || *nameText == '\0') {
    	DialogF(DF_WARN, NameTextW, 1,
    		"Please specify a name\nfor the menu item", "OK");
    	if (nameText!=NULL)
    	    XtFree(nameText);
    	return False;
    }
    if (strchr(nameText, ':')) {
    	DialogF(DF_WARN, NameTextW, 1,
    		"Menu item names may not\ncontain colon (:) characters", "OK");
    	XtFree(nameText);
    	return False;
    }
    cmdText = XmTextGetString(CmdTextW);
    if (cmdText == NULL || *cmdText == '\0') {
    	DialogF(DF_WARN, CmdTextW, 1, "Please specify a shell command", "OK");
    	XtFree(nameText);
    	if (cmdText!=NULL)
    	    XtFree(cmdText);
    	return False;
    }
    strcpy(f->name, nameText);
    XtFree(nameText);
    strcpy(f->cmd, cmdText);
    XtFree(cmdText);
    if ((mneText = XmTextGetString(MneTextW)) != NULL) {
    	f->mnemonic = mneText==NULL ? '\0' : mneText[0];
    	XtFree(mneText);
    	if (f->mnemonic == ':')		/* colons mess up string parsing */
    	    f->mnemonic = '\0';
    }
    if ((accText = XmTextGetString(AccTextW)) != NULL) {
    	parseAcceleratorString(accText, &f->modifiers, &f->keysym);
    	XtFree(accText);
    }
    f->selInput = XmToggleButtonGetState(SelInpBtn);
    if (XmToggleButtonGetState(WinOutBtn))
    	f->output = TO_NEW_WINDOW;
    else if (XmToggleButtonGetState(DlogOutBtn))
    	f->output = TO_DIALOG;
    else
    	f->output = TO_SAME_WINDOW;
    return True;
}

/*
** Save (copy entirely) the current filter list to use for undo
*/
static void saveFilterList(void)
{
    int i;
    filterListRec *f, *sf;
    
    /* Free the previously saved list */
    for (i=0; i<NSavedFilters; i++)
    	XtFree((char *)SavedFilterList[i]);
    
    /* Copy the new filter list */
    for (i=0; i<NFilters; i++) {
    	sf = (filterListRec *)XtMalloc(sizeof(filterListRec));
    	f = FilterList[i];
    	strcpy(sf->name, f->name);
    	sf->modifiers = f->modifiers;
    	sf->keysym = f->keysym;
    	sf->mnemonic = f->mnemonic;
    	sf->selInput = f->selInput;
    	sf->output = f->output;
    	strcpy(sf->cmd, f->cmd);
    	SavedFilterList[i] = sf;
    }
    NSavedFilters = NFilters;
    
    /* Save the position currently currently selected in the list */
    SavedSelection = selectedListPosition(ListW);
}

/*
** Undo the last command by swapping the filter list with the saved filter list
*/
static void restoreFilterList(void)
{
    filterListRec *tempList[MAX_FILTER_ITEMS];
    int tempCount, tempSelection, i;
    
    for (i=0; i<NFilters; i++)
    	tempList[i] = FilterList[i];
    tempCount = NFilters;
    for (i=0; i<NSavedFilters; i++)
    	FilterList[i] = SavedFilterList[i];
    NFilters = NSavedFilters;
    for (i=0; i<tempCount; i++)
    	SavedFilterList[i] = tempList[i];
    NSavedFilters = tempCount;

    tempSelection = selectedListPosition(ListW);
    updateFilterDialog(SavedSelection);
    SavedSelection = tempSelection;
}

/*
** Get the position of the selection in the filter list widget
*/
static int selectedListPosition(Widget listW)
{
    int listPos;
    int *posList = NULL, posCount = 0;

    if (!XmListGetSelectedPos(listW, &posList, &posCount)) {
	fprintf(stderr, "NEdit: Internal error (nothing selected)");
    	return 0;
    }
    listPos = *posList;
    XtFree((char *)posList);
    return listPos;
}

/*
** Gut a text widget of it's ability to process input
*/
static void disableTextW(Widget textW)
{
    static XtTranslations emptyTable = NULL;
    static char *emptyTranslations = "\
    	<EnterWindow>:	enter()\n\
	<Btn1Down>:	grab-focus()\n\
	<Btn1Motion>:	extend-adjust()\n\
	<Btn1Up>:	extend-end()\n\
	Shift<Key>Tab:	prev-tab-group()\n\
	Ctrl<Key>Tab:	next-tab-group()\n\
	<Key>Tab:	next-tab-group()\n\
	<LeaveWindow>:	leave()\n\
	<FocusIn>:	focusIn()\n\
	<FocusOut>:	focusOut()\n\
	<Unmap>:	unmap()\n";

    /* replace the translation table with the slimmed down one above */
    if (emptyTable == NULL)
    	emptyTable = XtParseTranslationTable(emptyTranslations);
    XtVaSetValues(textW, XmNtranslations, emptyTable, 0);
}

/*
** Turn a multi-line editing text widget into a fake single line text area
** by disabling the translation for Return.  This is necessary since only
** MULTI_LINE_EDIT mode text widgets can do text wrapping and the command
** command text widget needs wrapping to handle long commands.
*/
static void makeSingleLineTextW(Widget textW)
{
    static XtTranslations noReturnTable = NULL;
    static char *noReturnTranslations = "<Key>Return: activate()\n";
    
    if (noReturnTable == NULL)
    	noReturnTable = XtParseTranslationTable(noReturnTranslations);
    XtOverrideTranslations(textW, noReturnTable);
}

/*
** Issue a a command, feed it the string input, and either return or
** store its output in a text widget (if textW is NULL, return the output
** from the command as a string to be freed by the caller, otherwise store the
** output between leftPos and rightPos in the text widget textW).  Flags may
** be set to ACCUMULATE, and/or ERROR_DIALOGS.  ACCUMULATE causes output
** from the command to be saved up until the command completes.  ERROR_DIALOGS
** presents stderr output separately in popup a dialog, and also reports
** failed exit status as a popup dialog including the command output.
** ERROR_DIALOGS should only be used along with ACCUMULATE.
*/
static char *issueCommand(Widget dlogParent, char *command, char *input,
	int flags, Widget textW, int replaceLeft, int replaceRight)
{
    int status, stdinFD, stdoutFD, stderrFD, maxFD;
    int len, leftPos = replaceLeft, rightPos = replaceRight;
    pid_t childPid;
    int nWritten, nRead;
    buffer *buf, *outBufs = NULL, *errBufs = NULL;
    char *outText, *errText, *inPtr = input;
    int resp, inLength = strlen(input);
    fd_set readfds, writefds;
    struct timeval timeout;
    Widget workingDlg = NULL;
    int failure, errorReport, cancel = False;
    int outEOF = False, errEOF = (flags & ERROR_DIALOGS) ? False : True;
    int abortFlag = False;
    XtAppContext context = XtDisplayToApplicationContext(TheDisplay);
    time_t startTime = time(NULL);
    time_t lastIOTime = time(NULL);
    
    /* verify consistency of input parameters */
    if ((flags & ERROR_DIALOGS) && !ACCUMULATE)
    	return NULL;
    
    /* put up a watch cursor over the waiting window */
    BeginWait(dlogParent);

    /* fork the subprocess and issue the command */
    childPid = forkCommand(dlogParent, command, &stdinFD, &stdoutFD, 
    	    (flags & ERROR_DIALOGS) ? &stderrFD: NULL);
    
    /* set the pipes connected to the process for non-blocking i/o */
    if (fcntl(stdinFD, F_SETFL, O_NONBLOCK) < 0)
    	perror("NEdit: Internal error (fcntl)");
    if (fcntl(stdoutFD, F_SETFL, O_NONBLOCK) < 0)
    	perror("NEdit: Internal error (fcntl1)");
    if (flags & ERROR_DIALOGS) {
	if (fcntl(stderrFD, F_SETFL, O_NONBLOCK) < 0)
    	    perror("NEdit: Internal error (fcntl2)");
    }
    
    /* if there's nothing to write to the process' stdin, close it now */
    if (inLength == 0)
    	close(stdinFD);
    	
    /*
    ** Loop writing input to process and reading input from it until
    ** end-of-file is reached on both stdout and stderr pipes.
    */
    while (!(outEOF && errEOF)) {
    
	/* Process all pending X events, regardless of whether
	   select says there are any. */
	while (XtAppPending(context)) {
	    XtAppProcessEvent(context, XtIMAll);
	}
   
	/* If the process is taking too long, put up a working dialog */
	if (workingDlg == NULL &&  time(NULL) >= startTime + 6)
	    workingDlg = createWorkingDialog(dlogParent, &abortFlag);
	
	/* Check the abort flag set by the working dialog when the user
	   presses the cancel button in the dialog */
	if (abortFlag) {
	    kill(-childPid, SIGTERM);
	    freeBufList(&outBufs);
	    freeBufList(&errBufs);
    	    close(stdoutFD);
    	    if (flags & ERROR_DIALOGS)
    	    	close(stderrFD);
    	    EndWait(dlogParent);
	    return NULL;
	}
	
	/* Block and wait for something to happen, but wakeup every second
	   to check abort flag and waiting dialog and output timers */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(ConnectionNumber(TheDisplay), &readfds);
	maxFD = ConnectionNumber(TheDisplay);
	if (!outEOF) {
	    FD_SET(stdoutFD, &readfds);
	    maxFD = stdoutFD > maxFD ? stdoutFD : maxFD;
	}
	if (!errEOF) {
	    FD_SET(stderrFD, &readfds);
	    maxFD = stderrFD > maxFD ? stderrFD : maxFD;
	}
	if (inLength > 0) {
	    FD_SET(stdinFD, &writefds);
	    maxFD = stdinFD > maxFD ? stdinFD : maxFD;
	}
	if (select(maxFD+1, &readfds, &writefds, NULL, &timeout) == -1) {
	    if (EINTR != errno)
		perror("NEdit: select");
	}
	
	/* Dump intermediate output to window if the process is taking a long
	   time.  If there's read data pending, hold off so that the output
	   is in bigger chunks (each is an undo operation) */
	if (!(flags & ACCUMULATE) && textW!=NULL &&
		!FD_ISSET(stdoutFD, &readfds) && time(NULL) >= lastIOTime + 3) {
    	    outText = coalesceOutput(&outBufs);
	    len = strlen(outText);
	    if (len != 0) {
	    	XmTextReplace(textW, leftPos, rightPos, outText);
		XtFree(outText);
		leftPos += len;
		rightPos = leftPos;
	    }
	}
	
	/* write input to the sub-process stdin, close stdin when finished */
	if (FD_ISSET(stdinFD, &writefds) && inLength > 0) {
	    nWritten = write(stdinFD, inPtr, inLength);
	    if (nWritten == -1) {
    		if (errno != EWOULDBLOCK) {
    		    perror("NEdit: Write to filter command failed");
    		    freeBufList(&outBufs);
	    	    freeBufList(&errBufs);
    		    if (workingDlg != NULL)
    	    		XtUnmanageChild(workingDlg);
    	    	    EndWait(dlogParent);
    		    return NULL;
    		}
	    } else {
		inPtr += nWritten;
		inLength -= nWritten;
		if (inLength <= 0)
    		    close(stdinFD);
    	    }
    	}

    	/* read the output from stdout and create a linked list of buffers */
    	if (FD_ISSET(stdoutFD, &readfds)) {
    	    buf = (buffer *)XtMalloc(sizeof(buffer));
    	    nRead = read(stdoutFD, buf->contents, FILTER_BUF_SIZE);
    	    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK) {
		    perror("NEdit: Error reading filter output");
		    XtFree((char *)buf);
		    freeBufList(&outBufs);
	    	    freeBufList(&errBufs);
		    if (workingDlg != NULL)
    	    		XtUnmanageChild(workingDlg);
    	    	    EndWait(dlogParent);
		    return NULL;
		}
    	    } else if (nRead == 0) { /* eof */
    		outEOF = True;
    		XtFree((char *)buf);
    	    } else { /* characters read */
    		buf->length = nRead;
    		addOutput(&outBufs, buf);
   	    }
    	}

    	/* read the output from stderr and create a linked list of buffers */
    	if ((flags & ERROR_DIALOGS) && FD_ISSET(stderrFD, &readfds)) {
    	    buf = (buffer *)XtMalloc(sizeof(buffer));
    	    nRead = read(stderrFD, buf->contents, FILTER_BUF_SIZE);
    	    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
		    perror("NEdit: Error reading filter error stream");
		    XtFree((char *)buf);
		    freeBufList(&outBufs);
	    	    freeBufList(&errBufs);
		    if (workingDlg != NULL)
    	    		XtUnmanageChild(workingDlg);
    	    	    EndWait(dlogParent);
		    return NULL;
		}
    	    } else if (nRead == 0) { /* eof */
    		errEOF = True;
    		XtFree((char *)buf);
    	    } else { /* chars read */
    		buf->length = nRead;
    		addOutput(&errBufs, buf);
   	    }
    	}
    }
    close(stdoutFD);
    if (flags & ERROR_DIALOGS)
    	close(stderrFD);
    
    /* assemble the output from the process' stderr and stdout streams into
       null terminated strings, and free the buffer lists used to collect it */
    outText = coalesceOutput(&outBufs);
    if (flags & ERROR_DIALOGS)
    	errText = coalesceOutput(&errBufs);

    /* pop down the working dialog if it's up */
    if (workingDlg != NULL)
    	XtUnmanageChild(workingDlg);
    
    /* Wait for the child process to complete and get its return status */
    waitpid(childPid, &status, 0);
    
    EndWait(dlogParent);
    
    /* Present error and stderr-information dialogs.  If a command returned
       error output, or if the process' exit status indicated failure,
       present the information to the user. */
    if (flags & ERROR_DIALOGS) {
	failure = WIFEXITED(status) && WEXITSTATUS(status) != 0;
	errorReport = *errText != '\0';
	if (failure && errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_WARN, dlogParent, 2, "%s",
    	    	    "Cancel", "Proceed", errText);
    	    cancel = resp == 1;
	} else if (failure) {
    	    truncateString(outText, DF_MAX_MSG_LENGTH-70);
    	    resp = DialogF(DF_WARN, dlogParent, 2,
    	       "Command reported failed exit status.\nOutput from command:\n%s",
    		    "Cancel", "Proceed", outText);
    	    cancel = resp == 1;
	} else if (errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_INF, dlogParent, 2, "%s",
    	    	    "Proceed", "Cancel", errText);
    	    cancel = resp == 2;
	}
	XtFree(errText);
	if (cancel) {
	    XtFree(outText);
    	    return NULL;
	}
    }
    
    /* insert the remaining output, and move the insert point to the end */
    if (textW != NULL) {
	XmTextReplace(textW, leftPos, rightPos, outText);
	XmTextSetInsertionPosition(textW, leftPos + strlen(outText));
	XtFree(outText);
	return NULL;
    } else
    	return outText;
}

/*
** Fork a subprocess to execute a command, return file descriptors for pipes
** connected to the subprocess' stdin, stdout, and stderr streams.  If
** stderrFD is passed as NULL, the pipe represented by stdoutFD is connected
** to both stdin and stderr.  The function value returns the pid of the new
** subprocess, or -1 if an error occured.
*/
static pid_t forkCommand(Widget parent, char *command, int *stdinFD,
	int *stdoutFD, int *stderrFD)
{
    int childStdoutFD, childStdinFD, childStderrFD, pipeFDs[2];
    int dupFD;
    pid_t childPid;
    
    /* Create pipes to communicate with the sub process.  One end of each is
       returned to the caller, the other half is spliced to stdin, stdout
       and stderr in the child process */
    if (pipe(pipeFDs) != 0) {
    	perror("Nedit: Internal error (opening stdout pipe)");
        return -1;
    }
    *stdoutFD = pipeFDs[0];
    childStdoutFD = pipeFDs[1];
    if (pipe(pipeFDs) != 0) {
    	perror("Nedit: Internal error (opening stdin pipe)");
        return -1;
    }
    *stdinFD = pipeFDs[1];
    childStdinFD = pipeFDs[0];
    if (stderrFD == NULL)
    	childStderrFD = childStdoutFD;
    else {
	if (pipe(pipeFDs) != 0) {
    	    perror("Nedit: Internal error (opening stdin pipe)");
            return -1;
        }
	*stderrFD = pipeFDs[0];
	childStderrFD = pipeFDs[1];
    }
    
    /* Fork the filter process to run the command */
    childPid = fork();
    
    /*
    ** Child process context (fork returned 0), clean up the
    ** child ends of the pipes and execute the command
    */
    if (0 == childPid) {

	/* close the parent end of the pipes in the child process   */
	close(*stdinFD);
	close(*stdoutFD);
	if (stderrFD != NULL)
	    close(*stderrFD);

	/* close current stdin, stdout, and stderr file descriptors before
	   substituting pipes */
	close(fileno(stdin));
	close(fileno(stdout));
	close(fileno(stderr));

	/* duplicate the child ends of the pipes to have the same numbers
	   as stdout & stderr, so it can substitute for stdout & stderr */
 	dupFD = dup2(childStdinFD, fileno(stdin));
	if (dupFD == -1)
	    perror("dup of stdin failed");
 	dupFD = dup2(childStdoutFD, fileno(stdout));
	if (dupFD == -1)
	    perror("dup of stdout failed");
 	dupFD = dup2(childStderrFD, fileno(stderr));
	if (dupFD == -1)
	    perror("dup of stderr failed");
	
	/* make this process the leader of a new process group, so the sub
	   processes can be killed, if necessary, with a killpg call */
	setsid();
	
	/* execute the command using the shell specified by preferences */
	execl(GetPrefShell(), GetPrefShell(), "-c", command, 0);

	/* if we reach here, execl failed */
	fprintf(stderr, "Error starting shell: %s\n", GetPrefShell());
	exit();
    }
    
    /* Parent process context, check if fork succeeded */
    if (childPid == -1)
    	DialogF(DF_ERR, parent, 1,
		"Error starting filter process\n(fork failed)",
		"Acknowledged");

    /* close the child ends of the pipes */
    close(childStdinFD);
    close(childStdoutFD);
    if (stderrFD != NULL)
    	close(childStderrFD);

    return childPid;
}    

/*
** Add a buffer full of output to a buffer list
*/
static void addOutput(buffer **bufList, buffer *buf)
{
    buf->next = *bufList;
    *bufList = buf;
}

/*
** coalesce the contents of a list of buffers into a contiguous memory block,
** freeing the memory occupied by the buffer list.
*/
static char *coalesceOutput(buffer **bufList)
{
    buffer *buf, *rBufList = NULL;
    char *outBuf, *outPtr, *p;
    int i, length = 0;
    
    /* find the total length of data read */
    for (buf=*bufList; buf!=NULL; buf=buf->next)
    	length += buf->length;
    
    /* allocate contiguous memory for returning data */
    outBuf = XtMalloc(length+1);
    
    /* reverse the buffer list */
    while (*bufList != NULL) {
    	buf = *bufList;
    	*bufList = buf->next;
    	buf->next = rBufList;
    	rBufList = buf;
    }
    
    /* copy the buffers into the output buffer */
    outPtr = outBuf;
    for (buf=rBufList; buf!=NULL; buf=buf->next) {
    	p = buf->contents;
    	for (i=0; i<buf->length; i++)
    	    *outPtr++ = *p++;
    }
    
    /* terminate with a null */
    *outPtr++ = '\0';

    /* free the buffer list */
    freeBufList(&rBufList);
    
    return outBuf;
}

static void freeBufList(buffer **bufList)
{
    buffer *buf;
    
    while (*bufList != NULL) {
    	buf = *bufList;
    	*bufList = buf->next;
    	XtFree((char *)buf);
    }
}

/*
** Remove trailing newlines from a string by substituting nulls
*/
static void removeTrailingNewlines(char *string)
{
    char *endPtr = &string[strlen(string)-1];
    
    while (endPtr >= string && *endPtr == '\n')
    	*endPtr-- = '\0';
}

static Widget createWorkingDialog(Widget parent, int *abortFlag)
{
    Widget dlg;
    XmString st;
    
    dlg = XmCreateWorkingDialog(parent, "working", NULL, 0);
    XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_OK_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
    XtAddCallback(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON),
    	    XmNactivateCallback, (XtCallbackProc)cancelCB, abortFlag);
    XtVaSetValues(XmMessageBoxGetChild(dlg, XmDIALOG_MESSAGE_LABEL),
    	    XmNlabelString,
    	    st=XmStringCreateSimple("Waiting for command to complete"), 0);
    XtVaSetValues(XtParent(dlg), XmNtitle, " ", 0);
    XmStringFree(st);
    XtManageChild(dlg);
    
    return dlg;
}

static void cancelCB(Widget w, int *abortFlag, caddr_t call_data)
{
    *abortFlag = True;
}

/*
** Generate a text string for the preferences file describing the contents
** of the filter list.  This string is not exactly of the form that it can
** be read by LoadFilterString, rather, it is what needs to be written to
** a resource file such that it will read back in that form.
*/
char *WriteFilterListString(void)
{
    char *outStr, *outPtr, accStr[MAX_FILTER_ACC_LEN];
    filterListRec *f;
    int i, length;
    
    /* determine the amount of memory needed for the returned string */
    length = 0;
    for (i=0; i<NFilters; i++) {
    	f = FilterList[i];
    	generateAcceleratorString(accStr, f->modifiers, f->keysym);
    	length += strlen(f->name);
    	length += strlen(accStr);
    	length += strlen(f->cmd);
    	length += 21;			/* number of characters added below */
    }
    length++;				/* terminating null */
    
    /* allocate the string */
    outStr = XtMalloc(length);
    
    /* write the string */
    outPtr = outStr;
    *outPtr++ = '\\';
    *outPtr++ = '\n';
    for (i=0; i<NFilters; i++) {
    	f = FilterList[i];
    	generateAcceleratorString(accStr, f->modifiers, f->keysym);
    	*outPtr++ = '\t';
    	strcpy(outPtr, f->name);
    	outPtr += strlen(f->name);
    	*outPtr++ = ':';
    	strcpy(outPtr, accStr);
    	outPtr += strlen(accStr);
    	*outPtr++ = ':';
    	if (f->mnemonic != '\0')
    	    *outPtr++ = f->mnemonic;
    	*outPtr++ = ':';
    	if (f->selInput)
   	    *outPtr++ = 'I';
    	if (f->output == TO_DIALOG)
    	    *outPtr++ = 'D';
    	else if (f->output == TO_NEW_WINDOW)
    	    *outPtr++ = 'W';
    	*outPtr++ = ':';
    	*outPtr++ = '\\';
    	*outPtr++ = 'n';
    	*outPtr++ = '\\';
    	*outPtr++ = '\n';
    	*outPtr++ = '\t';
    	strcpy(outPtr, f->cmd);
    	outPtr += strlen(f->cmd);
    	*outPtr++ = '\\';
    	*outPtr++ = 'n';
    	*outPtr++ = '\\';
    	*outPtr++ = '\n';
    }
    --outPtr;
    *--outPtr = '\0';
    return outStr;
}

int LoadFilterListString(char *inString)
{
    filterListRec *f;
    char *inPtr = inString;
    char nameStr[MAX_FILTER_ITEM_LEN], accStr[MAX_FILTER_ACC_LEN], mneChar;
    char cmdStr[MAX_FILTER_CMD_LEN];
    KeySym keysym;
    unsigned int modifiers;
    int selInput;
    int output;
    int nameLen, accLen, mneLen, cmdLen;
    
    for (;;) {
   	
   	/* remove leading whitespace */
   	while (*inPtr == ' ' || *inPtr == '\t')
   	    inPtr++;
   	
   	/* read name field */
   	nameLen = strcspn(inPtr, ":");
	if (nameLen == 0)
    	    return parseError("no name field");
    	else if (nameLen >= MAX_FILTER_ITEM_LEN)
    	    return parseError("name field too long");
    	strncpy(nameStr, inPtr, nameLen);
    	nameStr[nameLen] = '\0';
    	inPtr += nameLen;
	if (*inPtr == '\0')
	    return parseError("end not expected");
	inPtr++;
	
	/* read accelerator field */
	accLen = strcspn(inPtr, ":");
	if (accLen >= MAX_FILTER_ACC_LEN)
	    return parseError("accelerator field too long");
    	strncpy(accStr, inPtr, accLen);
    	accStr[accLen] = '\0';
    	inPtr += accLen;
	if (*inPtr == '\0')
	    return parseError("end not expected");
    	inPtr++;
    	
    	/* read menemonic field */
    	mneLen = strcspn(inPtr, ":");
    	if (mneLen > 1)
    	    return parseError("mnemonic field too long");
    	if (mneLen == 1)
    	    mneChar = *inPtr++;
    	else
    	    mneChar = '\0';
    	inPtr++;
    	if (*inPtr == '\0')
	    return parseError("end not expected");
	
	/* read flags field */
	selInput = False;
	output = TO_SAME_WINDOW;
	for (; *inPtr != ':'; inPtr++) {
	    if (*inPtr == 'I')
	    	selInput = True;
	    else if (*inPtr == 'W')
	    	output = TO_NEW_WINDOW;
	    else if (*inPtr == 'D')
	    	output = TO_DIALOG;
	    else
	    	return parseError("unreadable flag field");
	}
	inPtr++;
	if (*inPtr++ != '\n')
	    return parseError("command must begin with newline");
	
   	/* remove leading whitespace */
   	while (*inPtr == ' ' || *inPtr == '\t')
   	    inPtr++;
   	
	/* read command field */
	cmdLen = strcspn(inPtr, "\n");
	if (cmdLen == 0)
    	    return parseError("command field is empty");
    	else if (cmdLen >= MAX_FILTER_CMD_LEN)
    	    return parseError("command field too long");
    	strncpy(cmdStr, inPtr, cmdLen);
    	cmdStr[cmdLen] = '\0';
    	inPtr += cmdLen;
	if (*inPtr == '\0')
	    return parseError("command not terminated with newline");
	inPtr++;

    	/* parse the accelerator field */
    	if (!parseAcceleratorString(accStr, &modifiers, &keysym))
    	    return parseError("couldn't read accelerator field");
    	
    	/* create a filter list record and add it to the list */
    	f = (filterListRec *)XtMalloc(sizeof(filterListRec));
	strcpy(f->name, nameStr);
	strcpy(f->cmd, cmdStr);
	f->mnemonic = mneChar;
	f->modifiers = modifiers;
	f->selInput = selInput;
	f->output = output;
	f->keysym = keysym;
    	FilterList[NFilters++] = f;
    	
    	/* end of string in proper place */
    	if (*inPtr == '\0')
    	    return True;
    }
}

static int parseError(char *message)
{
    fprintf(stderr, "NEdit: Parse error in shell menu item, %s\n", message);
    return False;
}

/*
** Create a text string representing an accelerator for the filter dialog,
** the filter resource, and for the menu item.
*/
static void generateAcceleratorString(char *text, unsigned int modifiers,
	KeySym keysym)
{
    char *shiftStr = "", *lockStr = "", *ctrlStr = "", *altStr = "";
    char keyName[20];

    /* if there's no accelerator, generate an empty string */
    if (keysym == NoSymbol) {
    	*text = '\0';
    	return;
    }

    /* translate the modifiers into strings */
    if (modifiers & ShiftMask)
    	shiftStr = "Shift+";
    if (modifiers & LockMask)
    	lockStr = "Lock+";
    if (modifiers & ControlMask)
    	ctrlStr = "Ctrl+";
    if (modifiers & Mod1Mask)
    	altStr = "Alt+";
    
    /* for a consistent look to the accelerator names in the menus,
       capitalize the first letter of the keysym */
    strcpy(keyName, XKeysymToString(keysym));
    *keyName = toupper(*keyName);
    
    /* concatenate the strings together */
    sprintf(text, "%s%s%s%s%s", shiftStr, lockStr, ctrlStr, altStr, keyName);
}

/*
** Create a translation table event description string for the menu
** XmNaccelerator resource.
*/
static void genAccelEventName(char *text, unsigned int modifiers,
	KeySym keysym)
{
    char *shiftStr = "", *lockStr = "", *ctrlStr = "", *altStr = "";

    /* if there's no accelerator, generate an empty string */
    if (keysym == NoSymbol) {
    	*text = '\0';
    	return;
    }
    
    /* translate the modifiers into strings */
    if (modifiers & ShiftMask)
    	shiftStr = "Shift ";
    if (modifiers & LockMask)
    	lockStr = "Lock ";
    if (modifiers & ControlMask)
    	ctrlStr = "Ctrl ";
    if (modifiers & Mod1Mask)
    	altStr = "Alt ";
    
    /* put the modifiers together with the key name */
    sprintf(text, "%s%s%s%s<Key>%s", shiftStr, lockStr, ctrlStr, altStr,
    	    XKeysymToString(keysym));
}

/*
** Read an accelerator name and put it into the form of a modifier mask
** and a KeySym code.  Returns false if string can't be read
** ... does not handle whitespace in string (look at scanf)
*/
static int parseAcceleratorString(char *string, unsigned int *modifiers,
	KeySym *keysym)
{
    int i, nFields, inputLength = strlen(string);
    char fields[6][MAX_FILTER_ACC_LEN];
    
    /* a blank field means no accelerator */
    if (inputLength == 0) {
    	*modifiers = 0;
    	*keysym = NoSymbol;
    	return True;
    }
    
    /* limit the string length so no field strings will overflow */
    if (inputLength > MAX_FILTER_ACC_LEN)
    	return False;
    
    /* divide the input into '+' separated fields */
    nFields = sscanf(string, "%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]",
    	    fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]);
    if (nFields == 0)
    	return False;
    
    /* get the key name from the last field and translate it to a keysym.
       If the name is capitalized, try it lowercase as well, since some
       of the keysyms are "prettied up" by generateAcceleratorString */
    *keysym = XStringToKeysym(fields[nFields-1]);
    if (*keysym == NoSymbol) {
    	*fields[nFields-1] = tolower(*fields[nFields-1]);
    	*keysym = XStringToKeysym(fields[nFields-1]);
    	if (*keysym == NoSymbol)
    	    return False;
    }
    	
    /* parse the modifier names from the rest of the fields */
    *modifiers = 0;
    for (i=0; i<nFields-1; i++) {
    	if (!strcmp(fields[i], "Shift"))
    	    *modifiers |= ShiftMask;
    	else if (!strcmp(fields[i], "Lock"))
    	    *modifiers |= LockMask;
    	else if (!strcmp(fields[i], "Ctrl"))
    	    *modifiers |= ControlMask;
    	else if (!strcmp(fields[i], "Alt"))
    	    *modifiers |= Mod1Mask;
    	else
    	    return False;
    }
    
    /* all fields successfully parsed */
    return True;
}

static void createOutputDialog(Widget parent, char *text)
{
    Arg al[50];
    int ac, rows, cols, hasScrollBar;
    Widget form, textW, button;
    XmString st1;

    /* measure the width and height of the text to determine size for dialog */
    measureText(text, MAX_OUT_DIALOG_COLS, &rows, &cols);
    if (rows > MAX_OUT_DIALOG_ROWS) {
    	rows = MAX_OUT_DIALOG_ROWS;
    	hasScrollBar = True;
    } else
    	hasScrollBar = False;
    if (cols > MAX_OUT_DIALOG_COLS)
    	cols = MAX_OUT_DIALOG_COLS;
    if (cols == 0)
    	cols = 1;
    
    ac = 0;
    form = XmCreateFormDialog(parent, "helpForm", al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("Dismiss")); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
    XtSetArg (al[ac], XmNtopAttachment, XmATTACH_NONE);  ac++;
    button = XmCreatePushButtonGadget(form, "dismiss", al, ac);
    XtManageChild(button);
    XtVaSetValues(form, XmNdefaultButton, button, 0);
    XmStringFree(st1);
    
    ac = 0;
    XtSetArg(al[ac], XmNrows, rows);  ac++;
    XtSetArg(al[ac], XmNcolumns, cols);  ac++;
    XtSetArg(al[ac], XmNresizeHeight, False);  ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNwordWrap, True);  ac++;
    XtSetArg(al[ac], XmNscrollHorizontal, False);  ac++;
    XtSetArg(al[ac], XmNscrollVertical, hasScrollBar);  ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNspacing, 0);  ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);  ac++;
    XtSetArg(al[ac], XmNeditable, False);  ac++;
    XtSetArg(al[ac], XmNvalue, text);  ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET);  ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomWidget, button);  ac++;
    textW = XmCreateScrolledText(form, "outText", al, ac);
    XtManageChild(textW);
    
    XtVaSetValues(XtParent(form), XmNtitle, "Output from Command", 0);
    XtManageChild(form);
}

/*
** Measure the width and height of a string of text.  Assumes 8 character
** tabs.  wrapWidth specifies a number of columns at which text wraps.
*/
static void measureText(char *text, int wrapWidth, int *rows, int *cols)
{
    int maxCols = 0, line = 1, col = 0;
    char *c;
    
    for (c=text; *c!='\0'; c++) {
    	if (*c=='\n' || col > wrapWidth) {
    	    line++;
    	    col = 0;
    	} else {
    	    if (*c == '\t')
    		col += 8 - (col % 8);
    	    else
    		col++;
    	    if (col > maxCols)
    	    	maxCols = col;
    	}
    }
    *rows = line;
    *cols = maxCols;
}

/*
** truncates a string to a maximum of length characters.  If it
** shortens the string, it appends "..." to show that it has been shortened.
** It assumes that the string that it is passed is writeable.
*/
static void truncateString(char *string, int length)
{
    if (strlen(string) > length)
	memcpy(&string[length-3], "...", 4);
}

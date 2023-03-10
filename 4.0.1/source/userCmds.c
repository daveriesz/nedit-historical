/*******************************************************************************
*									       *
* userCmds.c -- Nirvana Editor shell and macro command dialogs 		       *
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
* November, 1995							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)userCmds.c	1.10     10/31/94";
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <X11/keysym.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/RowColumn.h>
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "menu.h"
#include "shell.h"
#include "macro.h"
#include "file.h"

#define MAX_MENU_ITEM_LEN 40	/* max length for a shell/macro menu item */
#define MAX_ITEMS_PER_MENU 200	/* max number of shell/macro commands allowed */
#define MAX_ACCEL_LEN 50	/* max length of an accelerator string */

/* major divisions (in position units) in User Commands dialogs */
#define LIST_LEFT 57
#define SHELL_CMD_TOP 70
#define MACRO_CMD_TOP 30

/* types of current dialog and/or menu */
enum dialogTypes {SHELL_CMDS, MACRO_CMDS};

/* Structure representing a menu item for both the shell and macro menus */
typedef struct {
    char name[MAX_MENU_ITEM_LEN];
    unsigned int modifiers;
    KeySym keysym;
    char mnemonic;
    char input;
    char output;
    char repInput;
    char saveFirst;
    char loadAfter;
    char *cmd;
} menuItemRec;

/* Structure for widgets and flags associated with both shell command
   and macro command editing dialogs */
typedef struct {
    int dialogType;
    WindowInfo *window;
    Widget nameTextW, accTextW, mneTextW, cmdTextW, listW, saveFirstBtn;
    Widget loadAfterBtn, selInpBtn, winInpBtn, eitherInpBtn, noInpBtn;
    Widget repInpBtn, sameOutBtn, dlogOutBtn, winOutBtn, dlogShell;
    Widget addBtn, changeBtn, deleteBtn, moveUpBtn, moveDownBtn;
    menuItemRec **menuItemList;
    int *nMenuItems;
    menuItemRec *savedMenuItemList[MAX_ITEMS_PER_MENU];
    int nSavedMenuItems;
    int savedSelection;
} userCmdDialog;

/* The list of user programmed items for the shell menu */
static menuItemRec *ShellMenuItems[MAX_ITEMS_PER_MENU];
static int NShellMenuItems = 0;

/* The list of user programmed items for the macro menu */
static menuItemRec *MacroMenuItems[MAX_ITEMS_PER_MENU];
static int NMacroMenuItems = 0;

static void updateMenus(userCmdDialog *ucd);
static void updateMenu(WindowInfo *window, int menuType);
static void addCB(Widget w, XtPointer clientData, XtPointer callData);
static void changeCB(Widget w, XtPointer clientData, XtPointer callData);
static void deleteCB(Widget w, XtPointer clientData, XtPointer callData);
static void moveUpCB(Widget w, XtPointer clientData, XtPointer callData);
static void moveDownCB(Widget w, XtPointer clientData, XtPointer callData);
static void undoCB(Widget w, XtPointer clientData, XtPointer callData);
static void dismissCB(Widget w, XtPointer clientData, XtPointer callData);
static void pasteReplayCB(Widget w, XtPointer clientData, XtPointer callData);
static void destroyCB(Widget w, XtPointer clientData, XtPointer callData);
static void accKeyCB(Widget w, XtPointer clientData, XKeyEvent *event);
static void sameOutCB(Widget w, XtPointer clientData, XtPointer callData);
static void shellMenuCB(Widget w, WindowInfo *window, XtPointer callData);
static void macroMenuCB(Widget w, WindowInfo *window, XtPointer callData);
static void listSelectionCB(Widget w, XtPointer clientData,
	XmListCallbackStruct *callData);
static void accFocusCB(Widget w, XtPointer clientData, XtPointer callData);
static void accLoseFocusCB(Widget w, XtPointer clientData,
	XtPointer callData);
static void updateDialog(userCmdDialog *ucd, int selection);
static void updateDialogFields(userCmdDialog *ucd);
static void saveMenuItemList(userCmdDialog *ucd);
static void restoreMenuItemList(userCmdDialog *ucd);
static menuItemRec *readDialogFields(userCmdDialog *ucd, menuItemRec *f);
static void disableTextW(Widget textW);
static void makeSingleLineTextW(Widget textW);
static char *writeMenuItemString(menuItemRec **menuItems, int nItems,
	int listType);
static int loadMenuItemString(char *inString, menuItemRec **menuItems,
	int *nItems, int listType);
static int selectedListPosition(Widget listW);
static void generateAcceleratorString(char *text, unsigned int modifiers,
	KeySym keysym);
static void genAccelEventName(char *text, unsigned int modifiers,
	KeySym keysym);
static int parseAcceleratorString(char *string, unsigned int *modifiers,
	KeySym *keysym);
static int parseError(char *message);
static char *parseMacro(char **inPtr);

/*
** Present a dialog for editing the user specified commands in the shell menu
*/
void EditShellMenu(WindowInfo *window)
{
    Widget form, accLabel, inpLabel, inpBox, outBox, outLabel;
    Widget nameLabel, cmdLabel, dismissBtn, button;
    userCmdDialog *ucd;
    XmString s1;
    int ac;
    Arg args[20];

    /* if the dialog is already displayed, just pop it to the top and return */
    if (window->shellCmdDialog != NULL) {
    	XMapRaised(TheDisplay, XtWindow(window->shellCmdDialog));
    	return;
    }

    /* Create a structure for keeping track of dialog state */
    ucd = (userCmdDialog *)XtMalloc(sizeof(userCmdDialog));
    ucd->window = window;
    
    /* Set the dialog to operate on the Shell menu */
    ucd->menuItemList = ShellMenuItems;
    ucd->nMenuItems = &NShellMenuItems;
    ucd->dialogType = SHELL_CMDS;
    ucd->nSavedMenuItems = 0;
    ucd->savedSelection = 1;

    /* Initialize the undo information */
    saveMenuItemList(ucd);
    
    ac = 0;
    XtSetArg(args[ac], XmNautoUnmanage, False); ac++;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_NONE); ac++;
    form = XmCreateFormDialog(window->shell, "editShellCommands", args, ac);
    ucd->dlogShell = XtParent(form);
    window->shellCmdDialog = ucd->dlogShell;
    XtVaSetValues(ucd->dlogShell, XmNtitle, "Shell Commands", 0);
    XtAddCallback(form, XmNdestroyCallback, destroyCB, ucd);
    AddMotifCloseCallback(XtParent(form), dismissCB, ucd);
 
    ac = 0;
    XtSetArg(args[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
    XtSetArg(args[ac], XmNcolumns, 28); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNtopPosition, 2); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, LIST_LEFT+1); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, 99); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNbottomPosition, SHELL_CMD_TOP); ac++;
    ucd->listW = XmCreateScrolledList(form, "list", args, ac);
    XtManageChild(ucd->listW);
    XtAddCallback(ucd->listW, XmNbrowseSelectionCallback,
    	    (XtCallbackProc)listSelectionCB, ucd);

    ucd->loadAfterBtn = XtVaCreateManagedWidget("loadAfterBtn",
    	    xmToggleButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Re-load file after executing command"),
    	    XmNmnemonic, 'R',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_POSITION,
	    XmNbottomPosition, SHELL_CMD_TOP, 0);
    XmStringFree(s1);
    ucd->saveFirstBtn = XtVaCreateManagedWidget("saveFirstBtn",
    	    xmToggleButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Save file before executing command"),
    	    XmNmnemonic, 'f',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, ucd->loadAfterBtn, 0);
    XmStringFree(s1);
    ucd->repInpBtn = XtVaCreateManagedWidget("repInpBtn",
    	    xmToggleButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Output replaces input"),
    	    XmNmnemonic, 'f',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNset, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, ucd->saveFirstBtn, 0);
    XmStringFree(s1);
    outBox = XtVaCreateManagedWidget("outBox", xmRowColumnWidgetClass, form,
	    XmNpacking, XmPACK_TIGHT,
	    XmNorientation, XmHORIZONTAL,
	    XmNradioBehavior, True,
	    XmNradioAlwaysOne, True,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 3,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, ucd->repInpBtn,
	    XmNbottomOffset, 4, 0);
    ucd->sameOutBtn = XtVaCreateManagedWidget("sameOutBtn",
    	    xmToggleButtonWidgetClass, outBox,
    	    XmNlabelString, s1=MKSTRING("same window"),
    	    XmNmnemonic, 'a',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, True, 0);
    XmStringFree(s1);
    XtAddCallback(ucd->sameOutBtn, XmNvalueChangedCallback, sameOutCB, ucd);
    ucd->dlogOutBtn = XtVaCreateManagedWidget("dlogOutBtn",
    	    xmToggleButtonWidgetClass, outBox,
    	    XmNlabelString, s1=MKSTRING("dialog"),
    	    XmNmnemonic, 'g',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, False, 0);
    XmStringFree(s1);
    ucd->winOutBtn = XtVaCreateManagedWidget("winOutBtn", xmToggleButtonWidgetClass,
    	    outBox,
    	    XmNlabelString, s1=MKSTRING("new window"),
    	    XmNmnemonic, 'n',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, False, 0);
    XmStringFree(s1);
    outLabel = XtVaCreateManagedWidget("outLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Command Output:"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, outBox, 0);
    XmStringFree(s1);

    inpBox = XtVaCreateManagedWidget("inpBox", xmRowColumnWidgetClass, form,
	    XmNpacking, XmPACK_TIGHT,
	    XmNorientation, XmHORIZONTAL,
	    XmNradioBehavior, True,
	    XmNradioAlwaysOne, True,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 3,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, outLabel, 0);
    ucd->selInpBtn = XtVaCreateManagedWidget("selInpBtn", xmToggleButtonWidgetClass,
    	    inpBox,
    	    XmNlabelString, s1=MKSTRING("selection"),
    	    XmNmnemonic, 's',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, True, 0);
    XmStringFree(s1);
    ucd->winInpBtn = XtVaCreateManagedWidget("winInpBtn",
    	    xmToggleButtonWidgetClass, inpBox,
    	    XmNlabelString, s1=MKSTRING("window"),
    	    XmNmnemonic, 'w',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, False, 0);
    XmStringFree(s1);
    ucd->eitherInpBtn = XtVaCreateManagedWidget("eitherInpBtn",
    	    xmToggleButtonWidgetClass, inpBox,
    	    XmNlabelString, s1=MKSTRING("either"),
    	    XmNmnemonic, 't',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, False, 0);
    XmStringFree(s1);
    ucd->noInpBtn = XtVaCreateManagedWidget("noInpBtn",
    	    xmToggleButtonWidgetClass, inpBox,
    	    XmNlabelString, s1=MKSTRING("none"),
    	    XmNmnemonic, 'o',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginHeight, 0,
    	    XmNset, False, 0);
    XmStringFree(s1);
    inpLabel = XtVaCreateManagedWidget("inpLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Command Input:"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, inpBox, 0);
    XmStringFree(s1);
 
    ucd->mneTextW = XtVaCreateManagedWidget("mne", xmTextWidgetClass, form,
	    XmNcolumns, 1,
	    XmNmaxLength, 1,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_LEFT-10,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, inpLabel, 0);
    RemapDeleteKey(ucd->mneTextW);

    ucd->accTextW = XtVaCreateManagedWidget("acc", xmTextWidgetClass, form,
    	    XmNcolumns, 12,
    	    XmNmaxLength, MAX_ACCEL_LEN-1,
    	    XmNcursorPositionVisible, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT-15,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, inpLabel, 0);
    XtAddEventHandler(ucd->accTextW, KeyPressMask, False,
    	    (XtEventHandler)accKeyCB, ucd);
    XtAddCallback(ucd->accTextW, XmNfocusCallback, accFocusCB, ucd);
    XtAddCallback(ucd->accTextW, XmNlosingFocusCallback, accLoseFocusCB, ucd);
    accLabel = XtVaCreateManagedWidget("accLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Accelerator"),
    	    XmNmnemonic, 'l',
    	    XmNuserData, ucd->accTextW,
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 25,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->mneTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("mneLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Mnemonic"),
    	    XmNmnemonic, 'i',
    	    XmNuserData, ucd->mneTextW,
    	    XmNalignment, XmALIGNMENT_END,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 25,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, LIST_LEFT,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->mneTextW, 0);
    XmStringFree(s1);
    
    ucd->nameTextW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNmaxLength, MAX_MENU_ITEM_LEN-1,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, accLabel, 0);
    RemapDeleteKey(ucd->nameTextW);
 
    nameLabel = XtVaCreateManagedWidget("nameLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Menu Entry"),
    	    XmNmnemonic, 'y',
    	    XmNuserData, ucd->nameTextW,
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, LIST_LEFT,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->nameTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
"Select a shell menu item from the list at right ->\n\
or \"New\" to add a new command to the menu"),
    	    XmNmnemonic, 'm',
    	    XmNuserData, ucd->listW,
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, nameLabel, 0);
    XmStringFree(s1);
 
    cmdLabel = XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Shell Command to Execute"),
    	    XmNmnemonic, 'x',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNtopAttachment, XmATTACH_POSITION,
    	    XmNtopPosition, SHELL_CMD_TOP,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1, 0);
    XmStringFree(s1);
    XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("(% expands to current filename)"),
    	    XmNalignment, XmALIGNMENT_END,
    	    XmNmarginTop, 5,
    	    XmNtopAttachment, XmATTACH_POSITION,
    	    XmNtopPosition, SHELL_CMD_TOP,
    	    XmNleftAttachment, XmATTACH_WIDGET,
    	    XmNleftWidget, cmdLabel,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 99, 0);
    XmStringFree(s1);

    ucd->addBtn = XtVaCreateManagedWidget("add", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Add"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 15,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->addBtn, XmNactivateCallback, addCB, ucd);
    XmStringFree(s1);

    ucd->changeBtn = XtVaCreateManagedWidget("chg", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Change"),
    	    XmNmnemonic, 'C',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 15,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 29,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->changeBtn, XmNactivateCallback, changeCB, ucd);
    XmStringFree(s1);

    ucd->deleteBtn = XtVaCreateManagedWidget("del", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Delete"),
    	    XmNmnemonic, 'D',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 29,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 43,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->deleteBtn, XmNactivateCallback, deleteCB,
    	    ucd);
    XmStringFree(s1);

    ucd->moveUpBtn = XtVaCreateManagedWidget("mvUp", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Move ^"),
    	    XmNmnemonic, 'e',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 43,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 57,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->moveUpBtn, XmNactivateCallback, moveUpCB,
    	    ucd);
    XmStringFree(s1);

    ucd->moveDownBtn = XtVaCreateManagedWidget("mvDn", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Move v"),
    	    XmNmnemonic, 'v',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 57,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 71,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->moveDownBtn, XmNactivateCallback, moveDownCB, ucd);
    XmStringFree(s1);

    button = XtVaCreateManagedWidget("undo", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Undo"),
    	    XmNmnemonic, 'U',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 71,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 85,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(button, XmNactivateCallback, undoCB, ucd);
    XmStringFree(s1);

    dismissBtn = XtVaCreateManagedWidget("dismiss",xmPushButtonWidgetClass,form,
    	    XmNlabelString, s1=MKSTRING("Dismiss"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 85,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 99,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(dismissBtn, XmNactivateCallback, dismissCB, ucd);
    XmStringFree(s1);
    
    ac = 0;
    XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
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
    ucd->cmdTextW = XmCreateScrolledText(form, "name", args, ac);
    XtManageChild(ucd->cmdTextW);
    makeSingleLineTextW(ucd->cmdTextW);
    RemapDeleteKey(ucd->cmdTextW);
    XtVaSetValues(cmdLabel, XmNuserData, ucd->cmdTextW, 0); /* for mnemonic */
   
    /* Disable text input for the accelerator key widget, let the
       event handler manage it instead */
    disableTextW(ucd->accTextW);

    /* initializs the list of menu items in the list widget */
    updateDialog(ucd, 1);
    
    /* Set initial default button */
    XtVaSetValues(form, XmNdefaultButton, ucd->addBtn, 0);
    XtVaSetValues(form, XmNcancelButton, dismissBtn, 0);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);
    
    /* put up dialog */
    ManageDialogCenteredOnPointer(form);
}

/*
** Present a dialog for editing the user specified commands in the Macro menu
*/
void EditMacroMenu(WindowInfo *window)
{
    Widget form, accLabel;
    Widget nameLabel, cmdLabel, dismissBtn, button;
    userCmdDialog *ucd;
    XmString s1;
    int ac;
    Arg args[20];

    /* if the dialog is already displayed, just pop it to the top and return */
    if (window->macroCmdDialog != NULL) {
    	XMapRaised(TheDisplay, XtWindow(window->macroCmdDialog));
    	return;
    }

    /* Create a structure for keeping track of dialog state */
    ucd = (userCmdDialog *)XtMalloc(sizeof(userCmdDialog));
    ucd->window = window;

    /* Set the dialog to operate on the Macro menu */
    ucd->menuItemList = MacroMenuItems;
    ucd->nMenuItems = &NMacroMenuItems;
    ucd->dialogType = MACRO_CMDS;
    ucd->nSavedMenuItems = 0;
    ucd->savedSelection = 1;

    /* Initialize the undo information */
    saveMenuItemList(ucd);
    
    ac = 0;
    XtSetArg(args[ac], XmNautoUnmanage, False); ac++;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_NONE); ac++;
    form = XmCreateFormDialog(window->shell, "editMacroCommands", args, ac);
    ucd->dlogShell = XtParent(form);
    window->macroCmdDialog = ucd->dlogShell;
    XtVaSetValues(ucd->dlogShell, XmNtitle, "Macro Commands", 0);
    XtAddCallback(form, XmNdestroyCallback, destroyCB, ucd);
    AddMotifCloseCallback(XtParent(form), dismissCB, ucd);
 
    ac = 0;
    XtSetArg(args[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
    XtSetArg(args[ac], XmNcolumns, 28); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNtopPosition, 2); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, LIST_LEFT+1); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, 99); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNbottomPosition, MACRO_CMD_TOP); ac++;
    ucd->listW = XmCreateScrolledList(form, "list", args, ac);
    XtManageChild(ucd->listW);
    XtAddCallback(ucd->listW, XmNbrowseSelectionCallback,
    	    (XtCallbackProc)listSelectionCB, ucd);
 
    ucd->mneTextW = XtVaCreateManagedWidget("mne", xmTextWidgetClass, form,
	    XmNcolumns, 1,
	    XmNmaxLength, 1,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_LEFT-20-6,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT-20,
	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, MACRO_CMD_TOP, 0);
    RemapDeleteKey(ucd->mneTextW);

    ucd->accTextW = XtVaCreateManagedWidget("acc", xmTextWidgetClass, form,
    	    XmNcolumns, 12,
    	    XmNmaxLength, MAX_ACCEL_LEN-1,
    	    XmNcursorPositionVisible, False,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT-20-10,
	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, MACRO_CMD_TOP, 0);
    XtAddEventHandler(ucd->accTextW, KeyPressMask, False,
    	    (XtEventHandler)accKeyCB, ucd);
    XtAddCallback(ucd->accTextW, XmNfocusCallback, accFocusCB, ucd);
    XtAddCallback(ucd->accTextW, XmNlosingFocusCallback, accLoseFocusCB, ucd);
 
    accLabel = XtVaCreateManagedWidget("accLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Accelerator"),
    	    XmNmnemonic, 'l',
    	    XmNuserData, ucd->accTextW,
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 25,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->mneTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("mneLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Mnemonic"),
    	    XmNmnemonic, 'i',
    	    XmNuserData, ucd->mneTextW,
    	    XmNalignment, XmALIGNMENT_END,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 25,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, LIST_LEFT-20,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->mneTextW, 0);
    XmStringFree(s1);
    
    window->macroPasteReplayBtn = XtVaCreateManagedWidget("pasteReplay",
    	    xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Paste Learn/\nReplay Macro"),
    	    XmNsensitive, GetReplayMacro() != NULL,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LIST_LEFT-20,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, LIST_LEFT,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
    	    XmNbottomWidget, ucd->accTextW,
    	    XmNbottomOffset, -7, 0);
    XtAddCallback(window->macroPasteReplayBtn, XmNactivateCallback,
    	    pasteReplayCB, ucd);
    XmStringFree(s1);
    
    ucd->nameTextW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNmaxLength, MAX_MENU_ITEM_LEN-1,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, accLabel, 0);
    RemapDeleteKey(ucd->nameTextW);
 
    nameLabel = XtVaCreateManagedWidget("nameLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Menu Entry"),
    	    XmNmnemonic, 'y',
    	    XmNuserData, ucd->nameTextW,
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, LIST_LEFT,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, ucd->nameTextW, 0);
    XmStringFree(s1);

    XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
"Select a Macro menu item from the list at right ->\n\
or \"New\" to add a new command to the menu"),
    	    XmNmnemonic, 'm',
    	    XmNuserData, ucd->listW,
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, LIST_LEFT,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, nameLabel, 0);
    XmStringFree(s1);
 
    cmdLabel = XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Macro Command to Execute"),
    	    XmNmnemonic, 'x',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmarginTop, 5,
    	    XmNtopAttachment, XmATTACH_POSITION,
    	    XmNtopPosition, MACRO_CMD_TOP,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1, 0);
    XmStringFree(s1);

    ucd->addBtn = XtVaCreateManagedWidget("add", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Add"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 1,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 15,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->addBtn, XmNactivateCallback, addCB, ucd);
    XmStringFree(s1);

    ucd->changeBtn = XtVaCreateManagedWidget("chg", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Change"),
    	    XmNmnemonic, 'C',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 15,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 29,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->changeBtn, XmNactivateCallback, changeCB, ucd);
    XmStringFree(s1);

    ucd->deleteBtn = XtVaCreateManagedWidget("del", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Delete"),
    	    XmNmnemonic, 'D',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 29,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 43,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->deleteBtn, XmNactivateCallback, deleteCB, ucd);
    XmStringFree(s1);

    ucd->moveUpBtn = XtVaCreateManagedWidget("mvUp", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Move ^"),
    	    XmNmnemonic, 'e',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 43,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 57,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->moveUpBtn, XmNactivateCallback, moveUpCB, ucd);
    XmStringFree(s1);

    ucd->moveDownBtn = XtVaCreateManagedWidget("mvDn", xmPushButtonWidgetClass,
    	    form,
    	    XmNlabelString, s1=MKSTRING("Move v"),
    	    XmNmnemonic, 'v',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 57,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 71,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(ucd->moveDownBtn, XmNactivateCallback, moveDownCB, ucd);
    XmStringFree(s1);

    button = XtVaCreateManagedWidget("undo", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=MKSTRING("Undo"),
    	    XmNmnemonic, 'U',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 71,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 85,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(button, XmNactivateCallback, undoCB, ucd);
    XmStringFree(s1);

    dismissBtn = XtVaCreateManagedWidget("dismiss",xmPushButtonWidgetClass,form,
    	    XmNlabelString, s1=MKSTRING("Dismiss"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 85,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 99,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, 0);
    XtAddCallback(dismissBtn, XmNactivateCallback, dismissCB, ucd);
    XmStringFree(s1);
    
    ac = 0;
    XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNtopWidget, cmdLabel); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 1); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, 99); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomWidget, button); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
    ucd->cmdTextW = XmCreateScrolledText(form, "name", args, ac);
    XtManageChild(ucd->cmdTextW);
    RemapDeleteKey(ucd->cmdTextW);
    XtVaSetValues(cmdLabel, XmNuserData, ucd->cmdTextW, 0); /* for mnemonic */
   
    /* Disable text input for the accelerator key widget, let the
       event handler manage it instead */
    disableTextW(ucd->accTextW);

    /* initializs the list of menu items in the list widget */
    updateDialog(ucd, 1);
    
    /* Set initial default button */
    XtVaSetValues(form, XmNdefaultButton, ucd->addBtn, 0);
    XtVaSetValues(form, XmNcancelButton, dismissBtn, 0);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);
    
    /* put up dialog */
    ManageDialogCenteredOnPointer(form);
}

/*
** Update the Shell menu of window "window" from the currently loaded shell
** commands
*/
void UpdateShellMenu(WindowInfo *window)
{
    updateMenu(window, SHELL_CMDS);
}

/*
** Update the Macro menu of window "window" from the currently loaded macro
** commands
*/
void UpdateMacroMenu(WindowInfo *window)
{
    updateMenu(window, MACRO_CMDS);
}

/*
** Generate a text string for the preferences file describing the contents
** of the shell cmd list.  This string is not exactly of the form that it
** can be read by LoadShellCmdsString, rather, it is what needs to be written
** to a resource file such that it will read back in that form.
*/
char *WriteShellCmdsString(void)
{
    return writeMenuItemString(ShellMenuItems, NShellMenuItems,
    	    SHELL_CMDS);
}

/*
** Generate a text string for the preferences file describing the contents
** of the macro cmd list.  This string is not exactly of the form that it
** can be read by LoadMacroCmdsString, rather, it is what needs to be written
** to a resource file such that it will read back in that form.
*/
char *WriteMacroCmdsString(void)
{
    return writeMenuItemString(MacroMenuItems, NMacroMenuItems, MACRO_CMDS);
}

/*
** Read a string representing shell command menu items and add them to the
** internal list used for constructing shell menus
*/
int LoadShellCmdsString(char *inString)
{
    return loadMenuItemString(inString, ShellMenuItems, &NShellMenuItems,
    	    SHELL_CMDS);
}

/*
** Read a string representing macro command menu items and add them to the
** internal list used for constructing macro menus
*/
int LoadMacroCmdsString(char *inString)
{
    return loadMenuItemString(inString, MacroMenuItems, &NMacroMenuItems,
    	    MACRO_CMDS);
}

/*
** Search through the shell menu and execute the first command with menu item
** name "itemName".  Returns True on successs and False on failure.
*/
#ifndef VMS
int DoNamedShellMenuCmd(WindowInfo *window, char *itemName)
{
    int i;
    
    for (i=0; i<NShellMenuItems; i++) {
    	if (!strcmp(ShellMenuItems[i]->name, itemName)) {
    	    if (ShellMenuItems[i]->output == TO_SAME_WINDOW &&
    	    	    CheckReadOnly(window))
    	    	return False;
    	    DoShellMenuCmd(window, ShellMenuItems[i]->cmd,
    		ShellMenuItems[i]->input, ShellMenuItems[i]->output,
    		ShellMenuItems[i]->repInput,
    		ShellMenuItems[i]->saveFirst, ShellMenuItems[i]->loadAfter);
    	    return True;
    	}
    }
    return False;
}
#endif /*VMS*/

/*
** Search through the Macro menu and execute the first command with menu item
** name "itemName".  Returns True on successs and False on failure.
*/
int DoNamedMacroMenuCmd(WindowInfo *window, char *itemName)
{
    int i;
    
    for (i=0; i<NMacroMenuItems; i++) {
    	if (!strcmp(MacroMenuItems[i]->name, itemName)) {
    	    DoMacro(window, MacroMenuItems[i]->cmd);
    	    return True;
    	}
    }
    return False;
}

/*
** Update all of the Shell or Macro menus of all editor windows, depending
** on the type of user-commands dialog currently up.
*/
static void updateMenus(userCmdDialog *ucd)
{
    WindowInfo *w;

    for (w=WindowList; w!=NULL; w=w->next)
	updateMenu(w, ucd->dialogType);
}

/*
** Updates either the Shell menu or the Macro menu of "window", depending on
** value of "menuType"
*/
static void updateMenu(WindowInfo *window, int menuType)
{
    Widget btn, menuPane;
    WidgetList items;
    int nItems, nListItems, n, userData;
    XmString st1, st2;
    menuItemRec *f, **itemList;
    char accText[MAX_ACCEL_LEN], accKeys[MAX_ACCEL_LEN+5];
    
    /* Fetch the appropriate menu pane and item list for this menu type */
    if (menuType == SHELL_CMDS) {
    	menuPane = window->shellMenuPane;
    	itemList = ShellMenuItems;
    	nListItems = NShellMenuItems;
    } else {
    	menuPane = window->macroMenuPane;
    	itemList = MacroMenuItems;
    	nListItems = NMacroMenuItems;
    }
    
    /* Remove all of the existing user commands from the menu */
    XtVaGetValues(menuPane, XmNchildren, &items,
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
    
    /* Add current (shell or macro) cmds from MenuItemList to the menu */
    for (n=0; n<nListItems; n++) {
    	f = itemList[n];
    	generateAcceleratorString(accText, f->modifiers, f->keysym);
    	genAccelEventName(accKeys, f->modifiers, f->keysym);
    	btn = XtVaCreateManagedWidget("cmd", xmPushButtonWidgetClass,
    		menuPane, 
    		XmNlabelString, st1=XmStringCreateSimple(f->name),
    		XmNacceleratorText, st2=XmStringCreateSimple(accText),
    		XmNaccelerator, accKeys,
    		XmNmnemonic, f->mnemonic,
    		XmNuserData, n+10, NULL);
	XtAddCallback(btn, XmNactivateCallback, menuType == SHELL_CMDS ?
		(XtCallbackProc)shellMenuCB : (XtCallbackProc)macroMenuCB,
		window);
    	XmStringFree(st1);
    	XmStringFree(st2);
    }
}

static void addCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    int i, listPos;
    menuItemRec *f;
    
    /* build a menu item record, by reading the name, accelerator, and
       mnemonic that the user entered in the dialog */
    f = readDialogFields(ucd, NULL);
    if (f == NULL)
    	return;
        
    /* get the item number currently selected in the menu item list */
    listPos = selectedListPosition(ucd->listW);

    /* save the existing state of the menu item list for undo */
    saveMenuItemList(ucd);
    ucd->savedSelection = selectedListPosition(ucd->listW);
        
    /* add the item to the menu item list */
    for (i= *ucd->nMenuItems; i>=listPos; i--)
    	ucd->menuItemList[i] = ucd->menuItemList[i-1];
    ucd->menuItemList[listPos-1] = f;
    (*ucd->nMenuItems)++;
    
    /* update the appropriate shell or macro menus in all windows */
    updateMenus(ucd);
    
    /* redisplay the list widget and select the new item */
    updateDialog(ucd, listPos+1);
}

static void changeCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    int listPos;
    menuItemRec *f;
        
    /* get the item currently selected in the menu item list */
    listPos = selectedListPosition(ucd->listW);
    if (listPos < 2 || listPos > *ucd->nMenuItems + 1)
    	return;					/* shouldn't happen */
    f = ucd->menuItemList[listPos-2];

    /* save the existing state of the shell cmd list for undo */
    saveMenuItemList(ucd);
    ucd->savedSelection = selectedListPosition(ucd->listW);
     
    /* modify the menu item list entry using the name, accelerator, and
       mnemonic text fields from the dialog */
    if (readDialogFields(ucd, f) == NULL)
    	return;
    
    /* update the appropriate shell or macro menus in all windows */
    updateMenus(ucd);
    
    /* update the item name in the list widget */
    updateDialog(ucd, listPos);
}

static void deleteCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    int i, index, listPos;
    menuItemRec *f;
        
    /* get the selected list position and the item to be deleted */
    listPos = selectedListPosition(ucd->listW);
    if (listPos < 2 || listPos > *ucd->nMenuItems + 1)
    	return;					/* shouldn't happen */
    index = listPos-2;
    f = ucd->menuItemList[index];

    /* save the existing state of the menu item list for undo */
    saveMenuItemList(ucd);
    ucd->savedSelection = selectedListPosition(ucd->listW);
     
    /* free the item and remove it from the list */
    XtFree(f->cmd);
    XtFree((char *)f);
    for (i=index; i<*ucd->nMenuItems-1; i++)
    	ucd->menuItemList[i] = ucd->menuItemList[i+1];
    (*ucd->nMenuItems)--;
    
    /* update the appropriate shell or macro menus in all windows */
    updateMenus(ucd);
    
    /* update the list widget and move the selection to the previous item
       in the list and display the fields appropriate  for that entry */
    updateDialog(ucd, index+1);
}

static void moveUpCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    int index, listPos;
    menuItemRec *temp;
        
    /* get the item index currently selected in the menu item list */
    listPos = selectedListPosition(ucd->listW);
    if (listPos < 3 || listPos > *ucd->nMenuItems + 1)
    	return;					/* shouldn't happen */
    index = listPos-2;

    /* save the existing state of the menu item list for undo */
    saveMenuItemList(ucd);
    ucd->savedSelection = selectedListPosition(ucd->listW);
     
    /* shuffle the item up in the menu item list */
    temp = ucd->menuItemList[index];
    ucd->menuItemList[index] = ucd->menuItemList[index-1];
    ucd->menuItemList[index-1] = temp;
    
    /* update the appropriate shell or macro menus in all windows */
    updateMenus(ucd);
    
    /* update the list widget and keep the selection on moved item */
    updateDialog(ucd, index+1);
}

static void moveDownCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    int index, listPos;
    menuItemRec *temp;
        
    /* get the item index currently selected in the menu item list */
    listPos = selectedListPosition(ucd->listW);
    if (listPos < 2 || listPos > *ucd->nMenuItems)
    	return;				/* shouldn't happen */
    index = listPos-2;

    /* save the existing state of the menu item list for undo */
    saveMenuItemList(ucd);
    ucd->savedSelection = selectedListPosition(ucd->listW);
     
    /* shuffle the item down in the menu item list */
    temp = ucd->menuItemList[index];
    ucd->menuItemList[index] = ucd->menuItemList[index+1];
    ucd->menuItemList[index+1] = temp;
    
    /* update the appropriate shell or macro menus in all windows */
    updateMenus(ucd);
    
    /* update the list widget and keep the selection on moved item */
    updateDialog(ucd, index+3);
}
    
static void undoCB(Widget w, XtPointer clientData, XtPointer callData)
{
    restoreMenuItemList((userCmdDialog *)clientData);
}

static void dismissCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    
    /* mark the window data structure to show that there's no longer a
       (macro or shell) dialog up */
    if (ucd->dialogType == SHELL_CMDS)
    	ucd->window->shellCmdDialog = NULL;
    else
    	ucd->window->macroCmdDialog = NULL;

    /* pop down and destroy the dialog (memory for ucd is freed in the
       destroy callback) */
    XtDestroyWidget(ucd->dlogShell);
}

static void pasteReplayCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    
    if (GetReplayMacro() == NULL)
    	return;
    
    XmTextInsert(ucd->cmdTextW, XmTextGetInsertionPosition(ucd->cmdTextW),
    	    GetReplayMacro());
}

static void destroyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    
    XtFree((char *)ucd);
}

static void listSelectionCB(Widget w, XtPointer clientData,
	XmListCallbackStruct *callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    
    if (callData->item_position == 1) {
    	XtVaSetValues(XtParent(ucd->addBtn), XmNdefaultButton, ucd->addBtn, 0);
    	XtSetSensitive(ucd->changeBtn, False);
    	XtSetSensitive(ucd->deleteBtn, False);
    	XtSetSensitive(ucd->moveUpBtn, False);
    	XtSetSensitive(ucd->moveDownBtn, False);
    } else {
    	XtVaSetValues(XtParent(ucd->changeBtn), XmNdefaultButton,
    		ucd->changeBtn, 0);
    	XtSetSensitive(ucd->changeBtn, True);
    	XtSetSensitive(ucd->deleteBtn, True);
    	XtSetSensitive(ucd->moveUpBtn, (callData->item_position != 2));
    	XtSetSensitive(ucd->moveDownBtn,
    		callData->item_position != *ucd->nMenuItems+1);
    }
    updateDialogFields(ucd);
}

static void accFocusCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;

    RemoveDialogMnemonicHandler(XtParent(ucd->accTextW));
}

static void accLoseFocusCB(Widget w, XtPointer clientData, XtPointer callData)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;

    AddDialogMnemonicHandler(XtParent(ucd->accTextW));
}

static void accKeyCB(Widget w, XtPointer clientData, XKeyEvent *event)
{
    userCmdDialog *ucd = (userCmdDialog *)clientData;
    KeySym keysym = XLookupKeysym(event, 0);
    char outStr[MAX_ACCEL_LEN];
    
    /* Accept only real keys, not modifiers alone */
    if (IsModifierKey(keysym))
    	return;
    
    /* Tab key means go to the next field, don't enter */
    if (keysym == XK_Tab)
    	return;
    
    /* Beep and return if the modifiers are buttons or ones we don't support */
    if (event->state & ~(ShiftMask | LockMask | ControlMask | Mod1Mask)) {
	XBell(TheDisplay, 0);
	return;
    }
    
    /* Delete or backspace clears field */
    if (keysym == XK_Delete || keysym == XK_BackSpace) {
    	XmTextSetString(ucd->accTextW, "");
    	return;
    }
    
    /* generate the string to use in the dialog field */
    generateAcceleratorString(outStr, event->state, keysym);

    /* Reject single character accelerators (a very simple way to eliminate
       un-modified letters and numbers)  The goal is give users a clue that
       they're supposed to type the actual keys, not the name.  This scheme
       is not rigorous and still allows accelerators like Comma. */
    if (strlen(outStr) == 1) {
    	XBell(TheDisplay, 0);
	return;
    }
    
    /* fill in the accelerator field in the dialog */
    XmTextSetString(ucd->accTextW, outStr);
}

static void sameOutCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtSetSensitive(((userCmdDialog *)clientData)->repInpBtn,
    	    XmToggleButtonGetState(w));
}

static void shellMenuCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    int userData, index;
    char *params[1];

    /* get the index of the shell command and verify that it's in range */
    XtVaGetValues(w, XmNuserData, &userData, 0);
    index = userData - 10;
    if (index <0 || index >= NShellMenuItems)
    	return;
    
    params[0] = ShellMenuItems[index]->name;
    XtCallActionProc(window->lastFocus, "shell-menu-command",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

static void macroMenuCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    int userData, index;
    char *params[1];

    /* get the index of the macro command and verify that it's in range */
    XtVaGetValues(w, XmNuserData, &userData, 0);
    index = userData - 10;
    if (index <0 || index >= NMacroMenuItems)
    	return;
    
    params[0] = MacroMenuItems[index]->name;
    XtCallActionProc(window->lastFocus, "macro-menu-command",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

/*
** Update the list in the shell or macro dialog to reflect the current contents
** of the menu item list, set the item that should now be highlighted, and
** update the dialog fields to correspond to the newly selected item.
*/
static void updateDialog(userCmdDialog *ucd, int selection)
{
    int i;
    XmString stringTable[MAX_ITEMS_PER_MENU+1];
    
    /* On many systems under Motif 1.1 the list widget can't handle items
       being changed while anything is selected! */
    XmListDeselectAllItems(ucd->listW);

    /* Fill in the list widget with the names from the menu item list */
    stringTable[0] = XmStringCreateSimple("New");
    for (i=0; i<*ucd->nMenuItems; i++)
    	stringTable[i+1] = XmStringCreateSimple(ucd->menuItemList[i]->name);
    XtVaSetValues(ucd->listW, XmNitems, stringTable,
    	    XmNitemCount, *ucd->nMenuItems+1, 0);
    for (i=0; i<=*ucd->nMenuItems; i++)
    	XmStringFree(stringTable[i]);

    /* Select the requested item (indirectly filling in the dialog fields) */
    XmListSelectPos(ucd->listW, selection, True);
}

/*
** Update the name, accelerator, mnemonic, and command fields in the shell
** command or macro dialog to agree with the currently selected item in the
** menu item list.
*/
static void updateDialogFields(userCmdDialog *ucd)
{
    int index, listPos;
    menuItemRec *f;
    char mneString[2], accString[MAX_ACCEL_LEN];
        
    /* get the index of the item currently selected in the meun item list */
    listPos = selectedListPosition(ucd->listW);
    index = listPos - 2;
    if (index < -1 || index > *ucd->nMenuItems - 1)
    	return;					/* shouldn't happen */
    
    /* fill in the name, accelerator, mnemonic, and command fields of the
       dialog for the newly selected item, or blank them if "New" is selected */
    if (index >= 0) {
    	f = ucd->menuItemList[index];
	mneString[0] = f->mnemonic;
	mneString[1] = '\0';
	generateAcceleratorString(accString, f->modifiers, f->keysym);
	XmTextSetString(ucd->nameTextW, f->name);
	XmTextSetString(ucd->cmdTextW, f->cmd);
	XmTextSetString(ucd->accTextW, accString);
	XmTextSetString(ucd->mneTextW, mneString);
	if (ucd->dialogType == SHELL_CMDS) {
	    XmToggleButtonSetState(ucd->selInpBtn, f->input == FROM_SELECTION,
	    	    False);
	    XmToggleButtonSetState(ucd->winInpBtn, f->input == FROM_WINDOW,
	    	    False);
	    XmToggleButtonSetState(ucd->eitherInpBtn, f->input == FROM_EITHER,
	    	    False);
	    XmToggleButtonSetState(ucd->noInpBtn, f->input == FROM_NONE,
	    	    False);
	    XmToggleButtonSetState(ucd->sameOutBtn, f->output==TO_SAME_WINDOW,
	    	    False);
	    XmToggleButtonSetState(ucd->winOutBtn, f->output==TO_NEW_WINDOW,
	    	    False);
	    XmToggleButtonSetState(ucd->dlogOutBtn, f->output==TO_DIALOG,
	    	    False);
	    XmToggleButtonSetState(ucd->repInpBtn, f->repInput, False);
	    XtSetSensitive(ucd->repInpBtn, f->output==TO_SAME_WINDOW);
	    XmToggleButtonSetState(ucd->saveFirstBtn, f->saveFirst, False);
	    XmToggleButtonSetState(ucd->loadAfterBtn, f->loadAfter, False);
	}
    } else {
    	XmTextSetString(ucd->nameTextW, "");
	XmTextSetString(ucd->cmdTextW, "");
	XmTextSetString(ucd->accTextW, "");
	XmTextSetString(ucd->mneTextW, "");
	if (ucd->dialogType == SHELL_CMDS) {
	    XmToggleButtonSetState(ucd->selInpBtn, True, True);
	    XmToggleButtonSetState(ucd->sameOutBtn, True, True);
	    XmToggleButtonSetState(ucd->repInpBtn, False, False);
	    XtSetSensitive(ucd->repInpBtn, True);
	    XmToggleButtonSetState(ucd->saveFirstBtn, False, False);
	    XmToggleButtonSetState(ucd->loadAfterBtn, False, False);
	}
    }
}    

/*
** Read the name, accelerator, mnemonic, and command fields from the shell
** or macro commands dialog into menuItemRec "f".  If "f" is NULL, allocates a
** new menuItemRec.  Returns a pointer to the new or existing menuItemRec
** as the function value, or NULL on failure.
*/
static menuItemRec *readDialogFields(userCmdDialog *ucd, menuItemRec *f)
{
    char *nameText, *cmdText, *mneText, *accText;

    nameText = XmTextGetString(ucd->nameTextW);
    if (nameText == NULL || *nameText == '\0') {
    	DialogF(DF_WARN, ucd->dlogShell, 1,
    		"Please specify a name\nfor the menu item", "Dismiss");
    	if (nameText!=NULL)
    	    XtFree(nameText);
    	return NULL;
    }
    if (strchr(nameText, ':')) {
    	DialogF(DF_WARN, ucd->dlogShell, 1,
    		"Menu item names may not\ncontain colon (:) characters",
    		"Dismiss");
    	XtFree(nameText);
    	return NULL;
    }
    cmdText = XmTextGetString(ucd->cmdTextW);
    if (cmdText == NULL || *cmdText == '\0') {
    	DialogF(DF_WARN, ucd->dlogShell, 1, "Please specify a %s", "Dismiss",
    		ucd->dialogType == SHELL_CMDS ? "shell command" : "macro");
    	XtFree(nameText);
    	if (cmdText!=NULL)
    	    XtFree(cmdText);
    	return NULL;
    }
    if (f == NULL)
    	f = (menuItemRec *)XtMalloc(sizeof(menuItemRec));
    else
    	XtFree(f->cmd);
    strcpy(f->name, nameText);
    XtFree(nameText);
    f->cmd = cmdText;
    if ((mneText = XmTextGetString(ucd->mneTextW)) != NULL) {
    	f->mnemonic = mneText==NULL ? '\0' : mneText[0];
    	XtFree(mneText);
    	if (f->mnemonic == ':')		/* colons mess up string parsing */
    	    f->mnemonic = '\0';
    }
    if ((accText = XmTextGetString(ucd->accTextW)) != NULL) {
    	parseAcceleratorString(accText, &f->modifiers, &f->keysym);
    	XtFree(accText);
    }
    if (ucd->dialogType == SHELL_CMDS) {
	if (XmToggleButtonGetState(ucd->selInpBtn))
    	    f->input = FROM_SELECTION;
	else if (XmToggleButtonGetState(ucd->winInpBtn))
    	    f->input = FROM_WINDOW;
	else if (XmToggleButtonGetState(ucd->eitherInpBtn))
    	    f->input = FROM_EITHER;
	else
    	    f->input = FROM_NONE;
	if (XmToggleButtonGetState(ucd->winOutBtn))
    	    f->output = TO_NEW_WINDOW;
	else if (XmToggleButtonGetState(ucd->dlogOutBtn))
    	    f->output = TO_DIALOG;
	else
    	    f->output = TO_SAME_WINDOW;
	f->repInput = XmToggleButtonGetState(ucd->repInpBtn);
	f->saveFirst = XmToggleButtonGetState(ucd->saveFirstBtn);
	f->loadAfter = XmToggleButtonGetState(ucd->loadAfterBtn);
    } else {
    	f->input = FROM_SELECTION;
    	f->output = TO_SAME_WINDOW;
    	f->repInput = False;
    	f->saveFirst = False;
    	f->loadAfter = False;
    }
    return f;
}

/*
** Save (copy entirely) the current menu item list to use for undo
*/
static void saveMenuItemList(userCmdDialog *ucd)
{
    int i;
    menuItemRec *f, *sf;
    
    /* Free the previously saved list */
    for (i=0; i<ucd->nSavedMenuItems; i++) {
    	XtFree(ucd->savedMenuItemList[i]->cmd);
    	XtFree((char *)ucd->savedMenuItemList[i]);
    }
    
    /* Copy the new menu item list */
    for (i=0; i<*ucd->nMenuItems; i++) {
    	sf = (menuItemRec *)XtMalloc(sizeof(menuItemRec));
    	f = ucd->menuItemList[i];
    	strcpy(sf->name, f->name);
    	sf->modifiers = f->modifiers;
    	sf->keysym = f->keysym;
    	sf->mnemonic = f->mnemonic;
    	sf->input = f->input;
    	sf->output = f->output;
    	sf->repInput = f->repInput;
    	sf->saveFirst = f->saveFirst;
    	sf->loadAfter = f->loadAfter;
    	sf->cmd = XtMalloc(strlen(f->cmd)+1);
    	strcpy(sf->cmd, f->cmd);
    	ucd->savedMenuItemList[i] = sf;
    }
    ucd->nSavedMenuItems = *ucd->nMenuItems;
}

/*
** Undo the last command by swapping the menu item list with the saved
** version of the list
*/
static void restoreMenuItemList(userCmdDialog *ucd)
{
    menuItemRec *tempList[MAX_ITEMS_PER_MENU];
    int tempCount, tempSelection, i;
    
    for (i=0; i<*ucd->nMenuItems; i++)
    	tempList[i] = ucd->menuItemList[i];
    tempCount = *ucd->nMenuItems;
    for (i=0; i<ucd->nSavedMenuItems; i++)
    	ucd->menuItemList[i] = ucd->savedMenuItemList[i];
    *ucd->nMenuItems = ucd->nSavedMenuItems;
    for (i=0; i<tempCount; i++)
    	ucd->savedMenuItemList[i] = tempList[i];
    ucd->nSavedMenuItems = tempCount;

    tempSelection = selectedListPosition(ucd->listW);
    updateDialog(ucd, ucd->savedSelection);
    ucd->savedSelection = tempSelection;
}

/*
** Get the position of the selection in the menu item list widget
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

static char *writeMenuItemString(menuItemRec **menuItems, int nItems,
	int listType)
{
    char *outStr, *outPtr, *c, accStr[MAX_ACCEL_LEN];
    menuItemRec *f;
    int i, length;
    
    /* determine the max. amount of memory needed for the returned string
       and allocate a buffer for composing the string */
    length = 0;
    for (i=0; i<nItems; i++) {
    	f = menuItems[i];
    	generateAcceleratorString(accStr, f->modifiers, f->keysym);
    	length += strlen(f->name);
    	length += strlen(accStr);
    	length += strlen(f->cmd) * 4;	/* allow for \n & \\ expansions */
    	length += 21;			/* number of characters added below */
    }
    length++;				/* terminating null */
    outStr = XtMalloc(length);
    
    /* write the string */
    outPtr = outStr;
    *outPtr++ = '\\';
    *outPtr++ = '\n';
    for (i=0; i<nItems; i++) {
    	f = menuItems[i];
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
    	if (listType == SHELL_CMDS) {
    	    if (f->input == FROM_SELECTION)
   		*outPtr++ = 'I';
    	    else if (f->input == FROM_WINDOW)
   		*outPtr++ = 'A';
    	    else if (f->input == FROM_EITHER)
   		*outPtr++ = 'E';
    	    if (f->output == TO_DIALOG)
    		*outPtr++ = 'D';
    	    else if (f->output == TO_NEW_WINDOW)
    		*outPtr++ = 'W';
    	    if (f->repInput)
    		*outPtr++ = 'X';
    	    if (f->saveFirst)
    		*outPtr++ = 'S';
    	    if (f->loadAfter)
    		*outPtr++ = 'L';
    	    *outPtr++ = ':';
    	} else {
    	    *outPtr++ = ':';
    	    *outPtr++ = ' ';
    	    *outPtr++ = '{';
    	}
    	*outPtr++ = '\\';
    	*outPtr++ = 'n';
    	*outPtr++ = '\\';
    	*outPtr++ = '\n';
    	*outPtr++ = '\t';
    	*outPtr++ = '\t';
    	for (c=f->cmd; *c!='\0'; c++) { /* Copy the command string, changing */
    	    if (*c == '\\') {	    	/* backslashes to double backslashes */
    	    	*outPtr++ = '\\';	/* and newlines to backslash-n's,    */
 	    	*outPtr++ = '\\';	/* followed by real newlines and tab */
 	    } else if (*c == '\n') {
 	    	*outPtr++ = '\\';
 	    	*outPtr++ = 'n';
 	    	*outPtr++ = '\\';
 	    	*outPtr++ = '\n';
 	    	*outPtr++ = '\t';
 	    	*outPtr++ = '\t';
 	    } else
 	    	*outPtr++ = *c;
    	}
    	if (listType == MACRO_CMDS) {
    	    if (*(outPtr-1) == '\t') outPtr--;
    	    *outPtr++ = '}';
    	}
    	*outPtr++ = '\\';
    	*outPtr++ = 'n';
    	*outPtr++ = '\\';
    	*outPtr++ = '\n';
    }
    --outPtr;
    *--outPtr = '\0';
    return outStr;
}

static int loadMenuItemString(char *inString, menuItemRec **menuItems,
	int *nItems, int listType)
{
    menuItemRec *f;
    char *cmdStr, *inPtr = inString;
    char nameStr[MAX_MENU_ITEM_LEN], accStr[MAX_ACCEL_LEN], mneChar;
    KeySym keysym;
    unsigned int modifiers;
    int input;
    int output;
    int saveFirst, loadAfter, repInput;
    int nameLen, accLen, mneLen, cmdLen;
    
    for (;;) {
   	
   	/* remove leading whitespace */
   	while (*inPtr == ' ' || *inPtr == '\t')
   	    inPtr++;
   	
   	/* read name field */
   	nameLen = strcspn(inPtr, ":");
	if (nameLen == 0)
    	    return parseError("no name field");
    	else if (nameLen >= MAX_MENU_ITEM_LEN)
    	    return parseError("name field too long");
    	strncpy(nameStr, inPtr, nameLen);
    	nameStr[nameLen] = '\0';
    	inPtr += nameLen;
	if (*inPtr == '\0')
	    return parseError("end not expected");
	inPtr++;
	
	/* read accelerator field */
	accLen = strcspn(inPtr, ":");
	if (accLen >= MAX_ACCEL_LEN)
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
	input = FROM_NONE;
	output = TO_SAME_WINDOW;
	repInput = False;
	saveFirst = False;
	loadAfter = False;
	for (; *inPtr != ':'; inPtr++) {
	    if (*inPtr == 'I')
	    	input = FROM_SELECTION;
	    else if (*inPtr == 'A')
	    	input = FROM_WINDOW;
	    else if (*inPtr == 'E')
	    	input = FROM_EITHER;
	    else if (*inPtr == 'W')
	    	output = TO_NEW_WINDOW;
	    else if (*inPtr == 'D')
	    	output = TO_DIALOG;
	    else if (*inPtr == 'X')
	    	repInput = True;
	    else if (*inPtr == 'S')
	    	saveFirst = True;
	    else if (*inPtr == 'L')
	    	loadAfter = True;
	    else
	    	return parseError("unreadable flag field");
	}
	inPtr++;
	
	/* read command field */
	if (listType == SHELL_CMDS) {
	    if (*inPtr++ != '\n')
		return parseError("command must begin with newline");
   	    while (*inPtr == ' ' || *inPtr == '\t') /* leading whitespace */
   	    	inPtr++;
	    cmdLen = strcspn(inPtr, "\n");
	    if (cmdLen == 0)
    		return parseError("shell command field is empty");
    	    cmdStr = XtMalloc(cmdLen+1);
    	    strncpy(cmdStr, inPtr, cmdLen);
    	    cmdStr[cmdLen] = '\0';
    	    inPtr += cmdLen;
	} else {
	    cmdStr = parseMacro(&inPtr);
	    if (cmdStr == NULL)
	    	return False;
	}
   	while (*inPtr == ' ' || *inPtr == '\t' || *inPtr == '\n')
   	    inPtr++; /* skip trailing whitespace & newline */

    	/* parse the accelerator field */
    	if (!parseAcceleratorString(accStr, &modifiers, &keysym))
    	    return parseError("couldn't read accelerator field");
    	
    	/* create a menu item record and add it to the list */
    	f = (menuItemRec *)XtMalloc(sizeof(menuItemRec));
	strcpy(f->name, nameStr);
	f->cmd = cmdStr;
	f->mnemonic = mneChar;
	f->modifiers = modifiers;
	f->input = input;
	f->output = output;
	f->repInput = repInput;
	f->saveFirst = saveFirst;
	f->loadAfter = loadAfter;
	f->keysym = keysym;
    	menuItems[(*nItems)++] = f;
    	
    	/* end of string in proper place */
    	if (*inPtr == '\0')
    	    return True;
    }
}

static int parseError(char *message)
{
    fprintf(stderr, "NEdit: Parse error in user defined menu item, %s\n",
    	    message);
    return False;
}

/*
** Create a text string representing an accelerator for the dialog,
** the shellCommands or macroCommands resource, and for the menu item.
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
    char fields[6][MAX_ACCEL_LEN];
    
    /* a blank field means no accelerator */
    if (inputLength == 0) {
    	*modifiers = 0;
    	*keysym = NoSymbol;
    	return True;
    }
    
    /* limit the string length so no field strings will overflow */
    if (inputLength > MAX_ACCEL_LEN)
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

/*
** Scan text from "*inPtr" to the end of macro input (matching brace),
** advancing inPtr, and return macro text as function return value.
**
** Works by parsing then re-generating the string using ParseActionList and
** ActionListToString.  (The only real advantage to storing macros in un-parsed
** text form is preserving comments, and this removes them, but this code is
** temporary until NEdit has a more complete macro language).
*/
static char *parseMacro(char **inPtr)
{
    actionList *actionL;
    char *retStr, *errMsg;
    
    /* Parse the input */
    actionL = ParseActionList(inPtr, &errMsg);
    if (actionL == NULL) {
    	fprintf(stderr, "NEdit: Parse error in macro menu item, %s\n",
    		errMsg);
    	return NULL;
    }
    
    /* Generate the output string */
    retStr = ActionListToString(actionL, "");
    FreeActionList(actionL);
    return retStr;
}

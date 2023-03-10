/*******************************************************************************
*									       *
* misc.c -- Miscelaneous Motif convenience functions			       *
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
* July 28, 1992								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)misc.c	1.26	3/8/94";
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#ifdef VMS
#include <sys/types.h>
#include <unixio.h>
#include <file.h>
#endif /*VMS*/
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Text.h>
#include "DialogF.h"
#include "misc.h"

/* math.h on Sun mysteriously excludes strtod and other functions when
   POSIX compliance is turned on */
extern double strtod();

/* flag used to disable delete key remapping */
static int RemapDeleteEnabled = True;

/* bitmap and mask for waiting (wrist-watch) cursor */
#define watch_x_hot 7
#define watch_y_hot 7
#define watch_width 16
#define watch_height 16
static unsigned char watch_bits[] = {
   0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0x10, 0x08, 0x08, 0x11,
   0x04, 0x21, 0x04, 0x21, 0xe4, 0x21, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08,
   0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07
};
#define watch_mask_width 16
#define watch_mask_height 16
static unsigned char watch_mask_bits[] = {
   0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f,
   0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f,
   0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f
};

static void removeWhiteSpace(char *string);
static void warnHandlerCB(String message);
static void passwdCB(Widget w, caddr_t clientData, XmTextVerifyCallbackStruct
	*txtVerStr);

/*
** Set up closeCB to be called when the user selects close from the
** window menu.  The close menu item usually activates f.kill which
** sends a WM_DELETE_WINDOW protocol request for the window.
*/
void AddMotifCloseCallback(Widget shell, XtCallbackProc closeCB, void *arg)
{
    static Atom wmpAtom, dwAtom = NULL;
    Display *display = XtDisplay(shell);

    /* deactivate the built in delete response of killing the application */
    XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, 0);

    /* add a delete window protocol callback instead */
    if (dwAtom == NULL) {
    	wmpAtom = XmInternAtom(display, "WM_PROTOCOLS", TRUE);
    	dwAtom = XmInternAtom(display, "WM_DELETE_WINDOW", TRUE);
    }
    XmAddProtocolCallback(shell, wmpAtom, dwAtom, closeCB, arg);
}

/*
** Motif still generates spurrious passive grab warnings on both IBM and SGI
** This routine suppresses them
*/
void SuppressPassiveGrabWarnings(void)
{
    XtSetWarningHandler(warnHandlerCB);
}

/*
** This routine kludges around the problem of backspace not being mapped
** correctly when Motif is used between a server with a delete key in
** the traditional typewriter backspace position and a client that
** expects a backspace key in that position.  Though there are three
** distinct levels of key re-mapping in effect when a user is running
** a Motif application, none of these is really appropriate or effective
** for eliminating the delete v.s. backspace problem.  Our solution is,
** sadly, to eliminate the forward delete functionality of the delete key
** in favor of backwards delete for both keys.  So as not to prevent the
** user or the application from applying other translation table re-mapping,
** we apply re-map the key as a post-processing step, applied after widget
** creation.  As a result, the re-mapping necessarily becomes embedded
** throughout an application (wherever text widgets are created), and
** within library routines, including the Nirvana utility library.  To
** make this remapping optional, the SetDeleteRemap function provides a
** way for an application to turn this functionality on and off.  It is
** recommended that applications that use this routine provide an
** application resource called remapDeleteKey so savvy users can get
** their forward delete functionality back.
*/
void RemapDeleteKey(Widget w)
{
    static XtTranslations table = NULL;
    static char *translations = "<Key>osfDelete: delete-previous-character()\n";

    if (RemapDeleteEnabled) {
    	if (table == NULL)
    	    table = XtParseTranslationTable(translations);
    	XtOverrideTranslations(w, table);
    }
}

/*
** PopDownBugPatch
**
** Under some circumstances, popping down a dialog and its parent in
** rapid succession causes a crash.  This routine delays and
** processs events until receiving a ReparentNotify event.
** (I have no idea why a ReparentNotify event occurs at all, but it does
** mark the point where it is safe to destroy or pop down the parent, and
** it might have something to do with the bug.)  There is a failsafe in
** the form of a ~1.5 second timeout in case no ReparentNotify arrives.
** Use this sparingly, only when real crashes are observed, and periodically
** check to make sure that it is still necessary.
*/
void PopDownBugPatch(Widget w)
{
    time_t stopTime;

    stopTime = time(NULL) + 1;
    while (time(NULL) <= stopTime) {
    	XEvent event;
    	XtAppContext context = XtWidgetToApplicationContext(w);
    	XtAppPeekEvent(context, &event);
    	if (event.xany.type == ReparentNotify)
    	    return;
    	XtAppProcessEvent(context, XtIMAll);
    }
}

void SetDeleteRemap(int state)
{
    RemapDeleteEnabled = state;
}
   
/*
** Create a string table suitable for passing to XmList widgets
*/
XmString* StringTable(int count, ... )
{
    va_list ap;
    XmString *array;
    int i;
    char *str;

    va_start(ap, count);
    array = (XmString*)XtMalloc((count+1) * sizeof(XmString));
    for(i = 0;  i < count; i++ ) {
    	str = va_arg(ap, char *);
	array[i] = XmStringCreateSimple(str);
    }
    array[i] = (XmString)0;
    va_end(ap);
    return(array);
}

void FreeStringTable(XmString *table)
{
    int i;

    for(i = 0; table[i] != 0; i++)
	XmStringFree(table[i]);
    XtFree((char *)table);
}

/*
** Simulate a button press.  The purpose of this routine is show users what
** is happening when they take an action with a non-obvious side effect,
** such as when a user double clicks on a list item.  The argument is an
** XmPushButton widget to "press"
*/ 
void SimulateButtonPress(Widget widget)
{
    XKeyPressedEvent keyEvent;
    
    memset((char *)&keyEvent, 0, sizeof(XKeyPressedEvent));
    keyEvent.type = KeyPress;
    keyEvent.serial = 1;
    keyEvent.send_event = True;
    keyEvent.display = XtDisplay(widget);
    keyEvent.window = XtWindow(widget);
    XtCallActionProc(widget, "ArmAndActivate", (XEvent *)&keyEvent, NULL, 0);
}

/*
** Add an item to an already established pull-down or pop-up menu, including
** mnemonics, accelerators and callbacks.
*/
Widget AddMenuItem(Widget parent, char *name, char *label,
			  char mnemonic, char *acc, char *accText,
			  XtCallbackProc callback, void *cbArg)
{
    Widget button;
    XmString st1, st2;
    
    button = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent, 
    	XmNlabelString, st1=XmStringCreateSimple(label),
    	XmNmnemonic, mnemonic,
    	XmNacceleratorText, st2=XmStringCreateSimple(accText),
    	XmNaccelerator, acc, NULL);
    XtAddCallback(button, XmNactivateCallback, callback, cbArg);
    XmStringFree(st1);
    XmStringFree(st2);
    return button;
}

/*
** Add a toggle button item to an already established pull-down or pop-up
** menu, including mnemonics, accelerators and callbacks.
*/
Widget AddMenuToggle(Widget parent, char *name, char *label,
		 	    char mnemonic, char *acc, char *accText,
		  	    XtCallbackProc callback, void *cbArg, int set)
{
    Widget button;
    XmString st1, st2;
    
    button = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent, 
    	XmNlabelString, st1=XmStringCreateSimple(label),
    	XmNmnemonic, mnemonic,
    	XmNacceleratorText, st2=XmStringCreateSimple(accText),
    	XmNaccelerator, acc,
    	XmNset, set, NULL);
    XtAddCallback(button, XmNvalueChangedCallback, callback, cbArg);
    XmStringFree(st1);
    XmStringFree(st2);
    return button;
}

/*
** Add a separator line to a menu
*/
Widget AddMenuSeparator(Widget parent, char *name)
{
    Widget button;
    
    button = XmCreateSeparator(parent, name, NULL, 0);
    XtManageChild(button);
    return button;
}

/*
** Add a sub-menu to an established pull-down or pop-up menu, including
** mnemonics, accelerators and callbacks.  Returns the menu pane of the
** new sub menu.
*/
Widget AddSubMenu(Widget parent, char *name, char *label, char mnemonic)
{
    Widget menu;
    XmString st1;
    
    menu = XmCreatePulldownMenu(parent, name, NULL, 0);
    XtVaCreateManagedWidget(name, xmCascadeButtonWidgetClass, parent, 
    	XmNlabelString, st1=XmStringCreateSimple(label),
    	XmNmnemonic, mnemonic,
    	XmNsubMenuId, menu, 0);
    XmStringFree(st1);
    return menu;
}

/*
** SetIntLabel, SetFloatLabel, SetIntText, SetFloatText
**
** Set the text of a motif label or text widget to show an integer or
** floating number.
*/
void SetIntLabel(Widget label, int value)
{
    char labelString[20];
    XmString s1;
    
    sprintf(labelString, "%d", value);
    s1=XmStringCreateSimple(labelString);
    XtVaSetValues(label, XmNlabelString, s1, 0);
    XmStringFree(s1);
}
void SetFloatLabel(Widget label, double value)
{
    char labelString[20];
    XmString s1;
    
    sprintf(labelString, "%g", value);
    s1=XmStringCreateSimple(labelString);
    XtVaSetValues(label, XmNlabelString, s1, 0);
    XmStringFree(s1);
}
void SetIntText(Widget text, int value)
{
    char labelString[20];
    
    sprintf(labelString, "%d", value);
    XmTextSetString(text, labelString);
}
void SetFloatText(Widget text, double value)
{
    char labelString[20];
    
    sprintf(labelString, "%g", value);
    XmTextSetString(text, labelString);
}

/*
** GetIntText, GetFloatText, GetIntTextWarn, GetFloatTextWarn
**
** Get the text of a motif text widget as an integer or floating point number.
** The functions will return TEXT_READ_OK of the value was read correctly.
** If not, they will return either TEXT_IS_BLANK, or TEXT_NOT_NUMBER.  The
** GetIntTextWarn and GetFloatTextWarn will display a dialog warning the
** user that the value could not be read.  The argument fieldName is used
** in the dialog to help the user identify where the problem is.  Set
** warnBlank to true if a blank field is also considered an error.
*/
int GetFloatText(Widget text, double *value)
{
    char *strValue, *endPtr, endChar;
    int retVal;

    strValue = XmTextGetString(text);	/* Get Value */
    removeWhiteSpace(strValue);		/* Remove blanks and tabs */
    *value = strtod(strValue, &endPtr);	/* Convert string to double */
    if (strlen(strValue) == 0)		/* String is empty */
	retVal = TEXT_IS_BLANK;
    else if (*endPtr != '\0')		/* Whole string not parsed */
    	retVal = TEXT_NOT_NUMBER;
    else
	retVal = TEXT_READ_OK;
    XtFree(strValue);
    return retVal;
}
int GetIntText(Widget text, int *value)
{
    char *strValue, *endPtr;
    int retVal;

    strValue = XmTextGetString(text);		/* Get Value */
    removeWhiteSpace(strValue);			/* Remove blanks and tabs */
    *value = strtol(strValue, &endPtr, 10);	/* Convert string to long */
    if (strlen(strValue) == 0)			/* String is empty */
	retVal = TEXT_IS_BLANK;
    else if (*endPtr != '\0')			/* Whole string not parsed */
    	retVal = TEXT_NOT_NUMBER;
    else
	retVal = TEXT_READ_OK;
    XtFree(strValue);
    return retVal;
}
int GetFloatTextWarn(Widget text, double *value, char *fieldName, int warnBlank)
{
    int result;
    char *valueStr;
    
    result = GetFloatText(text, value);
    if (result == TEXT_READ_OK || (result == TEXT_IS_BLANK && !warnBlank))
    	return result;
    valueStr = XmTextGetString(text);
    if (result == TEXT_IS_BLANK)
    	DialogF (DF_ERR, text, 1, "Please supply %s value",
    		 "Acknowledged", fieldName);
    else /* TEXT_NOT_NUMBER */
    	DialogF (DF_ERR, text, 1,
    		 "Can't read %s value: \"%s\"",
    		 "Acknowledged", fieldName, valueStr);
    XtFree(valueStr);
    return result;
}
int GetIntTextWarn(Widget text, int *value, char *fieldName, int warnBlank)
{
    int result;
    char *valueStr;
    
    result = GetIntText(text, value);
    if (result == TEXT_READ_OK || (result == TEXT_IS_BLANK && !warnBlank))
    	return result;
    valueStr = XmTextGetString(text);
    if (result == TEXT_IS_BLANK)
    	DialogF (DF_ERR, text, 1, "Please supply a value for %s",
    		 "Acknowledged", fieldName);
    else /* TEXT_NOT_NUMBER */
    	DialogF (DF_ERR, text, 1,
    		 "Can't read integer value \"%s\" in %s",
    		 "Acknowledged", valueStr, fieldName);
    XtFree(valueStr);
    return result;
}

/*
 * PasswordText - routine to add a callback to any text widget so that all
 * 		  text typed by the user is echoed with asterisks, allowing
 *		  a password to be typed in without being seen.
 *
 * parameters: w       - text widget to add the callback to
 *             passTxt - pointer to a string created by caller of this routine.
 *		         **NOTE** The length of this string should be one 
 *			 greater than the maximum specified by XmNmaxLength.
 *			 This string is set to empty just before the callback
 *			 is added.
 */

void PasswordText(Widget w, char *passTxt)
{
    passTxt[0] = '\0';
    XtAddCallback(w, XmNmodifyVerifyCallback, (XtCallbackProc)passwdCB,passTxt);
}

/*
** BeginWait/EndWait
**
** Display/Remove a watch cursor over topCursorWidget and its descendents
*/
void BeginWait(Widget topCursorWidget)
{
    Display *display = XtDisplay(topCursorWidget);
    Pixmap pixmap;
    Pixmap maskPixmap;
    XColor xcolors[2];
    int scr;
    static Cursor  waitCursor = NULL;
    
    /* if the watch cursor hasn't been created yet, create it */
    if (!waitCursor) {
	pixmap = XCreateBitmapFromData(display, DefaultRootWindow(display),
		(char *)watch_bits, watch_width, watch_height);

	maskPixmap = XCreateBitmapFromData(display, DefaultRootWindow(display),
		(char *)watch_mask_bits, watch_width, watch_height);

	xcolors[0].pixel = BlackPixelOfScreen(DefaultScreenOfDisplay(display));
	xcolors[1].pixel = WhitePixelOfScreen(DefaultScreenOfDisplay(display));

	XQueryColors(display, DefaultColormapOfScreen(
		DefaultScreenOfDisplay(display)), xcolors, 2);
	waitCursor = XCreatePixmapCursor(display, pixmap, maskPixmap,
		&xcolors[0], &xcolors[1], watch_x_hot, watch_y_hot);
	XFreePixmap(display, pixmap);
	XFreePixmap(display, maskPixmap);
    }

    /* display the cursor */
    XDefineCursor(display, XtWindow(topCursorWidget), waitCursor);
}

void EndWait(Widget topCursorWidget)
{
    XUndefineCursor(XtDisplay(topCursorWidget), XtWindow(topCursorWidget));
}

/*									     */
/* passwdCB: callback routine added by PasswordText routine.  This routine   */
/* 	     echoes each character typed as an asterisk (*) and a few other  */
/* 	     necessary things so that the password typed in is not visible   */
/*									     */

static void passwdCB(Widget w, char * passTxt, XmTextVerifyCallbackStruct
	*txtVerStr)

/* XmTextVerifyCallbackStruct:						      */
/*   int reason;	should be XmCR_MODIFYING_TEXT_VALUE 		      */
/*   XEvent  *event;	points to XEvent that triggered the callback	      */
/*   Boolean doit;	indicates whether action should be performed; setting */
/*			this to false negates the action		      */
/*   long currInsert, 	current position of insert cursor		      */
/*	  newInsert;	position user attempts to position the insert cursor  */
/*   long startPos,	starting position of the text to modify		      */
/*	  endPos;	ending position of the text to modify		      */
/*   XmTextBlock text;							      */

/* XmTextBlock (used to pass text around): 		    		      */
/*   char *ptr;                   points to text to be inserted 	      */
/*   int length;		  Number of bytes (length)		      */
/*   XmTextFormat format;         Representations format		      */

/* XmTextFormat: either FMT8BIT or FMT16BIT */

{
    int numCharsTyped, i, j, pos;

    /* ensure text format is 8-bit characters */
    if (txtVerStr->text->format != FMT8BIT)
	return;

    /* verify assumptions */
/*    if (txtVerStr->endPos < txtVerStr->startPos)
	fprintf(stderr, "User password callback error: endPos < startPos\n");
    if (strlen(passTxt) == 0 && txtVerStr->endPos != 0)
	fprintf(stderr, "User password callback error: no txt, but end != 0\n");

    printf("passTxt = %s, startPos = %d, endPos = %d, txtBlkAddr = %d\n",
	 passTxt, txtVerStr->startPos, txtVerStr->endPos, txtVerStr->text);
    if (txtVerStr->text != NULL && txtVerStr->text->ptr != NULL)
	printf("       string typed = %s, length = %d\n", txtVerStr->text->ptr,
		txtVerStr->text->length);
*/
    /* If necessary, expand/compress passTxt and insert any new text */
    if (txtVerStr->text != NULL && txtVerStr->text->ptr != NULL)
	numCharsTyped = txtVerStr->text->length;
    else
	numCharsTyped = 0;
    /* numCharsTyped = # chars to insert (that user typed) */
    /* j = # chars to expand (+) or compress (-) the password string */
    j = numCharsTyped - (txtVerStr->endPos - txtVerStr->startPos);
    if (j > 0) 				/* expand case: start at ending null  */
	for (pos = strlen(passTxt) + 1; pos >= txtVerStr->endPos; --pos)
	    passTxt[pos+j] = passTxt[pos];
    if (j < 0)				/* compress case */
	for (pos = txtVerStr->startPos + numCharsTyped; 
			     pos <= strlen(passTxt)+1; ++pos)
	    passTxt[pos] = passTxt[pos-j];
    /* then copy text to be inserted into passTxt */
    for (pos = txtVerStr->startPos, i = 0; i < numCharsTyped; ++i) {
	passTxt[pos+i] = *(txtVerStr->text->ptr + i);
	/* Replace text typed by user with asterisks (*) */
	*(txtVerStr->text->ptr + i) = '*';
    }
/*    printf("  Password string now = %s\n", passTxt);  */
}

/*
** Remove the white space (blanks and tabs) from a string
*/
static void removeWhiteSpace(char *string)
{
    char *outPtr = string;
    
    while (TRUE) {
    	if (*string != ' ' && *string != '\t')
	    *(outPtr++) = *(string++);
	else
	    string++;
    	if (*string == 0) {
	    *outPtr = 0;
	    return;
	}
    }
}

static void warnHandlerCB(String message)
{
    if (strstr(message, "XtRemoveGrab"))
    	return;
    if (strstr(message, "Attempt to remove non-existant passive grab"))
    	return;
    fprintf(stderr, message);
}

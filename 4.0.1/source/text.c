/*******************************************************************************
*									       *
* text.c - Text Editing Widget						       *
*									       *
* Copyright (c) 1995 Universities Research Association, Inc.		       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retains a paid-up,      *
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
* June 15, 1995								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#if XmVersion >= 1002
#include <Xm/PrimitiveP.h>
#endif
#include "textBuf.h"
#include "textDisp.h"
#include "textP.h"
#include "textSel.h"
#include "textDrag.h"

/* Number of pixels of motion from the initial (grab-focus) button press
   required to begin recognizing a mouse drag for the purpose of making a
   selection */
#define SELECT_THRESHOLD 5

/* Length of delay in milliseconds for vertical autoscrolling */
#define VERTICAL_SCROLL_DELAY 50

static void initialize(TextWidget request, TextWidget new);
static void redisplay(TextWidget w, XEvent *event, Region region);
static void destroy(TextWidget w);
static void resize(TextWidget w);
static Boolean setValues(TextWidget current, TextWidget request,
	TextWidget new);
static void realize(Widget w, XtValueMask *valueMask,
	XSetWindowAttributes *attributes);
static XtGeometryResult queryGeometry(Widget w, XtWidgetGeometry *proposed,
	XtWidgetGeometry *answer);
static void grabFocusAP(Widget w, XEvent *event, String *args,
	Cardinal *n_args);
static void moveDestinationAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void extendAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void extendStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void extendEndAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void processCancelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void secondaryStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void secondaryOrDragStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void secondaryAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void secondaryOrDragAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void copyToAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void copyToOrEndDragAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void copyPrimaryAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void cutPrimaryAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void moveToAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void moveToOrEndDragAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void exchangeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void pasteClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void copyClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void cutClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void insertStringAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void selfInsertAP(Widget w, XEvent *event, String *args,
	Cardinal *n_args);
static void newlineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void newlineAndIndentAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void newlineNoIndentAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void processTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void endOfLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void beginningOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deleteSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deletePreviousCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deleteNextCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deletePreviousWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deleteNextWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deleteToStartOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deleteToEndOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void forwardCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void backwardCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void forwardWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void backwardWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void forwardParagraphAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void backwardParagraphAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void keySelectAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void processUpAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void processShiftUpAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void processDownAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void processShiftDownAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void beginningOfFileAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void endOfFileAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void nextPageAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void previousPageAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void pageLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void pageRightAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void toggleOverstrikeAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void scrollUpAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void scrollDownAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void scrollToLineAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void selectAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void deselectAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void focusInAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void focusOutAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void checkMoveSelectionChange(Widget w, XEvent *event, int startPos,
	String *args, Cardinal *nArgs);
static void keyMoveExtendSelection(Widget w, XEvent *event, int startPos,
	int rectangular);
static void checkAutoShowInsertPos(Widget w);
static int checkReadOnly(Widget w);
static void insertAtCursor(Widget w, char *chars, XEvent *event);
static int pendingSelection(Widget w);
static int deletePendingSelection(Widget w, XEvent *event);
static int deleteEmulatedTab(Widget w, XEvent *event);
static void selectWord(Widget w);
static void selectLine(Widget w);
static int startOfWord(TextWidget w, int pos);
static int endOfWord(TextWidget w, int pos);
static void checkAutoScroll(TextWidget w, int x, int y);
static void endDrag(Widget w);
static void cancelDrag(Widget w);
static void callCursorMovementCBs(Widget w, XEvent *event);
static int checkAutoWrap(Widget w, char *chars, XEvent *event);
static void adjustSelection(TextWidget tw, int x, int y);
static void adjustSecondarySelection(TextWidget tw, int x, int y);
static void autoScrollTimerProc(XtPointer clientData, XtIntervalId *id);
static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId *id);
static int hasKey(char *key, String *args, Cardinal *nArgs);
static int max(int i1, int i2);
static int min(int i1, int i2);
static int strCaseCmp(char *str1, char *str2);

static char defaultTranslations[] = 
    "Ctrl<KeyPress>osfBackSpace: delete-previous-word()\n\
     <KeyPress>osfBackSpace: delete-previous-character()\n\
     Alt Shift Ctrl<KeyPress>osfDelete: cut-primary(\"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfDelete: cut-primary(\"rect\")\n\
     Shift Ctrl<KeyPress>osfDelete: cut-primary()\n\
     Ctrl<KeyPress>osfDelete: delete-to-end-of-line()\n\
     Shift<KeyPress>osfDelete: cut-clipboard()\n\
     <KeyPress>osfDelete: delete-next-character()\n\
     Alt Shift Ctrl<KeyPress>osfInsert: copy-primary(\"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfInsert: copy-primary(\"rect\")\n\
     Shift Ctrl<KeyPress>osfInsert: copy-primary()\n\
     Shift<KeyPress>osfInsert: paste-clipboard()\n\
     Ctrl<KeyPress>osfInsert: copy-clipboard()\n\
     Shift Ctrl<KeyPress>osfCut: cut-primary()\n\
     <KeyPress>osfCut: cut-clipboard()\n\
     <KeyPress>osfCopy: copy-clipboard()\n\
     <KeyPress>osfPaste: paste-clipboard()\n\
     <KeyPress>osfPrimaryPaste: copy-primary()\n\
     Alt Shift Ctrl<KeyPress>osfBeginLine: beginning-of-file(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfBeginLine: beginning-of-file(\"extend\" \"rect\")\n\
     Alt Shift<KeyPress>osfBeginLine: beginning-of-line(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfBeginLine: beginning-of-line(\"extend\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfBeginLine: beginning-of-file(\"extend\")\n\
     Ctrl<KeyPress>osfBeginLine: beginning-of-file()\n\
     Shift<KeyPress>osfBeginLine: beginning-of-line(\"extend\")\n\
     <KeyPress>osfBeginLine: beginning-of-line()\n\
     Alt Shift Ctrl<KeyPress>osfEndLine: end-of-file(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfEndLine: end-of-file(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfEndLine: end-of-line(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfEndLine: end-of-line(\"extend\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfEndLine: end-of-file(\"extend\")\n\
     Ctrl<KeyPress>osfEndLine: end-of-file()\n\
     Shift<KeyPress>osfEndLine: end-of-line(\"extend\")\n\
     <KeyPress>osfEndLine: end-of-line()\n\
     Alt Shift Ctrl<KeyPress>osfLeft: backward-word(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfLeft: backward-word(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfLeft: key-select(\"left\", \"rect\")\n\
     Meta Shift<KeyPress>osfLeft: key-select(\"left\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfLeft: backward-word(\"extend\")\n\
     Ctrl<KeyPress>osfLeft: backward-word()\n\
     Shift<KeyPress>osfLeft: key-select(\"left\")\n\
     <KeyPress>osfLeft: backward-character()\n\
     Alt Shift Ctrl<KeyPress>osfRight: forward-word(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfRight: forward-word(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfRight: key-select(\"right\", \"rect\")\n\
     Meta Shift<KeyPress>osfRight: key-select(\"right\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfRight: forward-word(\"extend\")\n\
     Ctrl<KeyPress>osfRight: forward-word()\n\
     Shift<KeyPress>osfRight: key-select(\"right\")\n\
     <KeyPress>osfRight: forward-character()\n\
     Alt Shift Ctrl<KeyPress>osfUp: backward-paragraph(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfUp: backward-paragraph(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfUp: process-shift-up(\"rect\")\n\
     Meta Shift<KeyPress>osfUp: process-shift-up(\"rect\")\n\
     Shift Ctrl<KeyPress>osfUp: backward-paragraph(\"extend\")\n\
     Ctrl<KeyPress>osfUp: backward-paragraph()\n\
     Shift<KeyPress>osfUp: process-shift-up()\n\
     <KeyPress>osfUp: process-up()\n\
     Alt Shift Ctrl<KeyPress>osfDown: forward-paragraph(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfDown: forward-paragraph(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfDown: process-shift-down(\"rect\")\n\
     Meta Shift<KeyPress>osfDown: process-shift-down(\"rect\")\n\
     Shift Ctrl<KeyPress>osfDown: forward-paragraph(\"extend\")\n\
     Ctrl<KeyPress>osfDown: forward-paragraph()\n\
     Shift<KeyPress>osfDown: process-shift-down()\n\
     <KeyPress>osfDown: process-down()\n\
     Alt Shift<KeyPress>osfPageLeft: page-left(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfPageLeft: page-left(\"extend\", \"rect\")\n\
     Shift<KeyPress>osfPageLeft: page-left(\"extend\")\n\
     <KeyPress>osfPageLeft: page-left()\n\
     Alt Shift<KeyPress>osfPageRight: page-right(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfPageRight: page-right(\"extend\", \"rect\")\n\
     Shift<KeyPress>osfPageRight: page-right(\"extend\")\n\
     <KeyPress>osfPageRight: page-right()\n\
     Alt Shift Ctrl<KeyPress>osfPageUp: page-left(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfPageUp: page-left(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfPageUp: previous-page(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfPageUp: previous-page(\"extend\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfPageUp: page-left(\"extend\")\n\
     Ctrl<KeyPress>osfPageUp: page-left()\n\
     Shift<KeyPress>osfPageUp: previous-page(\"extend\")\n\
     <KeyPress>osfPageUp: previous-page()\n\
     Alt Shift Ctrl<KeyPress>osfPageDown: page-right(\"extend\", \"rect\")\n\
     Meta Shift Ctrl<KeyPress>osfPageDown: page-right(\"extend\", \"rect\")\n\
     Alt Shift<KeyPress>osfPageDown: next-page(\"extend\", \"rect\")\n\
     Meta Shift<KeyPress>osfPageDown: next-page(\"extend\", \"rect\")\n\
     Shift Ctrl<KeyPress>osfPageDown: page-right(\"extend\")\n\
     Ctrl<KeyPress>osfPageDown: page-right()\n\
     Shift<KeyPress>osfPageDown: next-page(\"extend\")\n\
     <KeyPress>osfPageDown: next-page()\n\
     Shift<KeyPress>osfSelect: key-select()\n\
     <KeyPress>osfCancel: process-cancel()\n\
     Ctrl~Alt~Meta<KeyPress>v: paste-clipboard()\n\
     Ctrl~Alt~Meta<KeyPress>c: copy-clipboard()\n\
     Ctrl~Alt~Meta<KeyPress>x: cut-clipboard()\n\
     Ctrl~Alt~Meta<KeyPress>u: delete-to-start-of-line()\n\
     Ctrl<KeyPress>Return: newline-and-indent()\n\
     Shift<KeyPress>Return: newline-no-indent()\n\
     <KeyPress>Return: newline()\n\
     Ctrl<KeyPress>Tab: self-insert()\n\
     <KeyPress>Tab: process-tab()\n\
     Alt Shift Ctrl<KeyPress>space: key-select(\"rect\")\n\
     Meta Shift Ctrl<KeyPress>space: key-select(\"rect\")\n\
     Shift Ctrl~Meta~Alt<KeyPress>space: key-select()\n\
     Ctrl~Meta~Alt<KeyPress>slash: select-all()\n\
     Ctrl~Meta~Alt<KeyPress>backslash: deselect-all()\n\
     <KeyPress>: self-insert()\n\
     Alt Ctrl<Btn1Down>: move-destination()\n\
     Meta Ctrl<Btn1Down>: move-destination()\n\
     Shift Ctrl<Btn1Down>: extend-start(\"rect\")\n\
     Shift<Btn1Down>: extend-start()\n\
     <Btn1Down>: grab-focus()\n\
     Button1 Ctrl<MotionNotify>: extend-adjust(\"rect\")\n\
     Button1~Ctrl<MotionNotify>: extend-adjust()\n\
     <Btn1Up>: extend-end()\n\
     <Btn2Down>: secondary-or-drag-start()\n\
     Shift Ctrl Button2<MotionNotify>: secondary-or-drag-adjust(\"rect\", \
\"copy\", \"overlay\")\n\
     Shift Button2<MotionNotify>: secondary-or-drag-adjust(\"copy\")\n\
     Ctrl Button2<MotionNotify>: secondary-or-drag-adjust(\"rect\", \
\"overlay\")\n\
     Button2<MotionNotify>: secondary-or-drag-adjust()\n\
     Shift Ctrl<Btn2Up>: move-to-or-end-drag(\"copy\", \"overlay\")\n\
     Shift <Btn2Up>: move-to-or-end-drag(\"copy\")\n\
     Alt<Btn2Up>: exchange()\n\
     Meta<Btn2Up>: exchange()\n\
     Ctrl<Btn2Up>: copy-to-or-end-drag(\"overlay\")\n\
     <Btn2Up>: copy-to-or-end-drag()\n\
     <Btn3Down>: secondary-or-drag-start()\n\
     Shift Ctrl Button3<MotionNotify>: secondary-or-drag-adjust(\"rect\", \
\"copy\", \"overlay\")\n\
     Shift Button3<MotionNotify>: secondary-or-drag-adjust(\"copy\")\n\
     Ctrl Button3<MotionNotify>: secondary-or-drag-adjust(\"rect\", \
\"overlay\")\n\
     Button3<MotionNotify>: secondary-or-drag-adjust()\n\
     Shift Ctrl<Btn3Up>: move-to-or-end-drag(\"copy\", \"overlay\")\n\
     Shift <Btn3Up>: move-to-or-end-drag(\"copy\")\n\
     Alt<Btn3Up>: exchange()\n\
     Meta<Btn3Up>: exchange()\n\
     Ctrl<Btn3Up>: copy-to-or-end-drag(\"overlay\")\n\
     <Btn3Up>: copy-to-or-end-drag()\n\
     <FocusIn>: focusIn()\n\
     <FocusOut>: focusOut()\n";
     /* some of the translations from the Motif text widget were not picked up:
	  :<KeyPress>osfSelect: set-anchor()\n\
	  :<KeyPress>osfActivate: activate()\n\
	 ~Shift Ctrl~Meta~Alt<KeyPress>Return: activate()\n\
	  ~Shift Ctrl~Meta~Alt<KeyPress>space: set-anchor()\n\
	   :<KeyPress>osfClear: clear-selection()\n\
	  ~Shift~Ctrl~Meta~Alt<KeyPress>Return: process-return()\n\
	  Shift~Meta~Alt<KeyPress>Tab: prev-tab-group()\n\
	  Ctrl~Meta~Alt<KeyPress>Tab: next-tab-group()\n\
	  <UnmapNotify>: unmap()\n\
	  <EnterNotify>: enter()\n\
	  <LeaveNotify>: leave()\n
     */


static XtActionsRec actionsList[] = {
    {"self-insert", selfInsertAP},
    {"grab-focus", grabFocusAP},
    {"extend-adjust", extendAdjustAP},
    {"extend-start", extendStartAP},
    {"extend-end", extendEndAP},
    {"secondary-adjust", secondaryAdjustAP},
    {"secondary-or-drag-adjust", secondaryOrDragAdjustAP},
    {"secondary-start", secondaryStartAP},
    {"secondary-or-drag-start", secondaryOrDragStartAP},
    {"process-bdrag", secondaryOrDragStartAP},
    {"move-destination", moveDestinationAP},
    {"move-to", moveToAP},
    {"move-to-or-end-drag", moveToOrEndDragAP},
    {"copy-to", copyToAP},
    {"copy-to-or-end-drag", copyToOrEndDragAP},
    {"exchange", exchangeAP},
    {"process-cancel", processCancelAP},
    {"paste-clipboard", pasteClipboardAP},
    {"copy-clipboard", copyClipboardAP},
    {"cut-clipboard", cutClipboardAP},
    {"copy-primary", copyPrimaryAP},
    {"cut-primary", cutPrimaryAP},
    {"newline", newlineAP},
    {"newline-and-indent", newlineAndIndentAP},
    {"newline-no-indent", newlineNoIndentAP},
    {"delete-selection", deleteSelectionAP},
    {"delete-previous-character", deletePreviousCharacterAP},
    {"delete-next-character", deleteNextCharacterAP},
    {"delete-previous-word", deletePreviousWordAP},
    {"delete-next-word", deleteNextWordAP},
    {"delete-to-start-of-line", deleteToStartOfLineAP},
    {"delete-to-end-of-line", deleteToEndOfLineAP},
    {"forward-character", forwardCharacterAP},
    {"backward-character", backwardCharacterAP},
    {"key-select", keySelectAP},
    {"process-up", processUpAP},
    {"process-down", processDownAP},
    {"process-shift-up", processShiftUpAP},
    {"process-shift-down", processShiftDownAP},
    {"process-home", beginningOfLineAP},
    {"forward-word", forwardWordAP},
    {"backward-word", backwardWordAP},
    {"forward-paragraph", forwardParagraphAP},
    {"backward-paragraph", backwardParagraphAP},
    {"beginning-of-line", beginningOfLineAP},
    {"end-of-line", endOfLineAP},
    {"beginning-of-file", beginningOfFileAP},
    {"end-of-file", endOfFileAP},
    {"next-page", nextPageAP},
    {"previous-page", previousPageAP},
    {"page-left", pageLeftAP},
    {"page-right", pageRightAP},
    {"toggle-overstrike", toggleOverstrikeAP},
    {"scroll-up", scrollUpAP},
    {"scroll-down", scrollDownAP},
    {"scroll-to-line", scrollToLineAP},
    {"select-all", selectAllAP},
    {"deselect-all", deselectAllAP},
    {"focusIn", focusInAP},
    {"focusOut", focusOutAP},
    {"process-return", selfInsertAP},
    {"process-tab", processTabAP},
    {"insert-string", insertStringAP},
};

/* The motif text widget defined a bunch of actions which the nedit text
   widget as-of-yet does not support:
   
     Actions which were not bound to keys (for emacs emulation, some of
     them should probably be supported:
       
	kill-next-character()
	kill-next-word()
	kill-previous-character()
	kill-previous-word()
	kill-selection()
	kill-to-end-of-line()
	kill-to-start-of-line()
	unkill()
	next-line()
	newline-and-backup()
	beep()
	redraw-display()
	scroll-one-line-down()
	scroll-one-line-up()
	set-insertion-point()

    Actions which are not particularly useful:
    
	set-anchor()
	activate()
	clear-selection() -> this is a wierd one
	do-quick-action() -> don't think this ever worked
	Help()
	next-tab-group()
	select-adjust()
	select-start()
	select-end()
*/

static XtResource resources[] = {
    {XmNhighlightThickness, XmCHighlightThickness, XmRDimension,
      sizeof(Dimension), XtOffset(TextWidget, primitive.highlight_thickness),
      XmRInt, 0},
    {XmNshadowThickness, XmCShadowThickness, XmRDimension, sizeof(Dimension),
      XtOffset(TextWidget, primitive.shadow_thickness), XmRInt, 0},
    {textNfont, textCFont, XmRFontStruct, sizeof(XFontStruct *),
      XtOffset(TextWidget, text.fontStruct), XmRString, "fixed"},
    {textNselectForeground, textCSelectForeground, XmRPixel, sizeof(Pixel),
      XtOffset(TextWidget, text.selectFGPixel), XmRString, "black"},
    {textNselectBackground, textCSelectBackground, XmRPixel, sizeof(Pixel),
      XtOffset(TextWidget, text.selectBGPixel), XmRString, "#cccccc"},
    {textNhighlightForeground, textCHighlightForeground, XmRPixel,sizeof(Pixel),
      XtOffset(TextWidget, text.highlightFGPixel), XmRString, "white"},
    {textNhighlightBackground, textCHighlightBackground, XmRPixel,sizeof(Pixel),
      XtOffset(TextWidget, text.highlightBGPixel), XmRString, "red"},
    {textNcursorForeground, textCCursorForeground, XmRPixel,sizeof(Pixel),
      XtOffset(TextWidget, text.cursorFGPixel), XmRString, "black"},
    {textNrows, textCRows, XmRInt,sizeof(int),
      XtOffset(TextWidget, text.rows), XmRString, "24"},
    {textNcolumns, textCColumns, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.columns), XmRString, "80"},
    {textNmarginWidth, textCMarginWidth, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.marginWidth), XmRString, "5"},
    {textNmarginHeight, textCMarginHeight, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.marginHeight), XmRString, "5"},
    {textNpendingDelete, textCPendingDelete, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.pendingDelete), XmRString, "True"},
    {textNautoWrap, textCAutoWrap, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.autoWrap), XmRString, "True"},
    {textNautoIndent, textCAutoIndent, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.autoIndent), XmRString, "True"},
    {textNoverstrike, textCOverstrike, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.overstrike), XmRString, "False"},
    {textNheavyCursor, textCHeavyCursor, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.heavyCursor), XmRString, "False"},
    {textNreadOnly, textCReadOnly, XmRBoolean, sizeof(Boolean),
      XtOffset(TextWidget, text.readOnly), XmRString, "False"},
    {textNwrapMargin, textCWrapMargin, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.wrapMargin), XmRString, "0"},
    {textNhScrollBar, textCHScrollBar, XmRWidget, sizeof(Widget),
      XtOffset(TextWidget, text.hScrollBar), XmRString, ""},
    {textNvScrollBar, textCVScrollBar, XmRWidget, sizeof(Widget),
      XtOffset(TextWidget, text.vScrollBar), XmRString, ""},
    {textNautoShowInsertPos, textCAutoShowInsertPos, XmRBoolean,
      sizeof(Boolean), XtOffset(TextWidget, text.autoShowInsertPos),
      XmRString, "True"},
    {textNwordDelimiters, textCWordDelimiters, XmRString, sizeof(char *),
      XtOffset(TextWidget, text.delimiters), XmRString,
      ".,/\\`'!@#%^&*()-=+{}[]\":;<>?"},
    {textNblinkRate, textCBlinkRate, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.cursorBlinkRate), XmRString, "500"},
    {textNemulateTabs, textCEmulateTabs, XmRInt, sizeof(int),
      XtOffset(TextWidget, text.emulateTabs), XmRString, "0"},
    {textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t),
      XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL},
    {textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback,
      sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL},
    {textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback,
      sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL},
    {textNdragStartCallback, textCDragStartCallback, XmRCallback,
      sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback,
      NULL},
    {textNdragEndCallback, textCDragEndCallback, XmRCallback,
      sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL},
};

TextClassRec textClassRec = {
     /* CoreClassPart */
  {
    (WidgetClass) &xmPrimitiveClassRec,  /* superclass       */
    "Text",                         /* class_name            */
    sizeof(TextRec),                /* widget_size           */
    NULL,                           /* class_initialize      */
    NULL,                           /* class_part_initialize */
    FALSE,                          /* class_inited          */
    (XtInitProc)initialize,         /* initialize            */
    NULL,                           /* initialize_hook       */
    realize,   		            /* realize               */
    actionsList,                    /* actions               */
    XtNumber(actionsList),          /* num_actions           */
    resources,                      /* resources             */
    XtNumber(resources),            /* num_resources         */
    NULLQUARK,                      /* xrm_class             */
    TRUE,                           /* compress_motion       */
    TRUE,                           /* compress_exposure     */
    TRUE,                           /* compress_enterleave   */
    FALSE,                          /* visible_interest      */
    (XtWidgetProc)destroy,          /* destroy               */
    (XtWidgetProc)resize,           /* resize                */
    (XtExposeProc)redisplay,        /* expose                */
    (XtSetValuesFunc)setValues,     /* set_values            */
    NULL,                           /* set_values_hook       */
    XtInheritSetValuesAlmost,       /* set_values_almost     */
    NULL,                           /* get_values_hook       */
    NULL,                           /* accept_focus          */
    XtVersion,                      /* version               */
    NULL,                           /* callback private      */
    defaultTranslations,            /* tm_table              */
    queryGeometry,                  /* query_geometry        */
    NULL,                           /* display_accelerator   */
    NULL,                           /* extension             */
  },
  /* Motif primitive class fields */
  {
     (XtWidgetProc)_XtInherit,   	/* Primitive border_highlight   */
     (XtWidgetProc)_XtInherit,   	/* Primitive border_unhighlight */
     NULL, /*XtInheritTranslations,*/	/* translations                 */
     NULL,				/* arm_and_activate             */
     NULL,				/* get resources      		*/
     0,					/* num get_resources  		*/
     NULL,         			/* extension                    */
  },
  /* Text class part */
  {
    0,                              	/* ignored	                */
  }
};

WidgetClass textWidgetClass = (WidgetClass)&textClassRec;

/*
** Widget initialize method
*/
static void initialize(TextWidget request, TextWidget new)
{
    XFontStruct *fs = new->text.fontStruct;
    char *delimiters;
    textBuffer *buf;
    Pixel white, black;
   
    /* Set the initial window size based on the rows and columns resources */
    if (request->core.width == 0)
    	new->core.width = fs->max_bounds.width * new->text.columns +
    		new->text.marginWidth * 2;
    if (request->core.height == 0)
   	new->core.height = (fs->ascent + fs->descent) * new->text.rows +
   		new->text.marginHeight * 2;
    
    /* The default colors work for B&W as well as color, except for
       selectFGPixel and selectBGPixel, where color highlighting looks
       much better without reverse video, so if we get here, and the
       selection is totally unreadable because of the bad default colors,
       swap the colors and make the selection reverse video */
    white = WhitePixelOfScreen(XtScreen((Widget)new));
    black = BlackPixelOfScreen(XtScreen((Widget)new));
    if (    new->text.selectBGPixel == white &&
    	    new->core.background_pixel == white &&
    	    new->text.selectFGPixel == black &&
    	    new->primitive.foreground == black) {
    	new->text.selectBGPixel = black;
    	new->text.selectFGPixel = white;
    }
    
    /* Create the initial text buffer for the widget to display (which can
       be replaced later with TextSetBuffer) */
    buf = BufCreate();
    
    /* Create and initialize the text-display part of the widget */
    new->text.textD = TextDCreate((Widget)new, new->text.hScrollBar,
	    new->text.vScrollBar, new->text.marginWidth, new->text.marginHeight,
	    new->core.width - new->text.marginWidth * 2,
	    new->core.height - new->text.marginHeight * 2,
	    buf, new->text.fontStruct, new->core.background_pixel,
	    new->primitive.foreground, new->text.selectFGPixel,
	    new->text.selectBGPixel, new->text.highlightFGPixel,
	    new->text.highlightBGPixel, new->text.cursorFGPixel);

    /* Add mandatory delimiters blank, tab, and newline to the list of
       delimiters (... is memory lost here?) */
    delimiters =  XtMalloc(strlen(new->text.delimiters) + 4);
    sprintf(delimiters, "%s%s", " \t\n", new->text.delimiters);
    new->text.delimiters = delimiters;

    /* Start with the cursor blanked (widgets don't have focus on creation,
       the initial FocusIn event will unblank it and get blinking started) */
    new->text.textD->cursorOn = False;
    
    /* Initialize the widget variables */
    new->text.autoScrollProcID = 0;
    new->text.cursorBlinkProcID = 0;
    new->text.dragState = NOT_CLICKED;
    new->text.multiClickState = NO_CLICKS;
    new->text.lastBtnDown = 0;
    new->text.selectionOwner = False;
    new->text.motifDestOwner = False;
    new->text.emTabsBeforeCursor = 0;
}

/*
** Widget destroy method
*/
static void destroy(TextWidget w)
{
    textBuffer *buf;
    
    /* Free the text display and possibly the attached buffer.  The buffer
       is freed only if after removing all of the modify procs (by calling
       StopHandlingXSelections and TextDFree) there are no modify procs
       left */
    StopHandlingXSelections((Widget)w);
    buf = w->text.textD->buffer;
    TextDFree(w->text.textD);
    if (buf->nModifyProcs == 0)
    	BufFree(buf);

    if (w->text.cursorBlinkProcID != 0)
    	XtRemoveTimeOut(w->text.cursorBlinkProcID);
    XtFree(w->text.delimiters);
    XtRemoveAllCallbacks((Widget)w, textNfocusCallback);
    XtRemoveAllCallbacks((Widget)w, textNlosingFocusCallback);
    XtRemoveAllCallbacks((Widget)w, textNcursorMovementCallback);
    XtRemoveAllCallbacks((Widget)w, textNdragStartCallback);
    XtRemoveAllCallbacks((Widget)w, textNdragEndCallback);
}

/*
** Widget resize method.  Called when the size of the widget changes
*/
static void resize(TextWidget w)
{
    XFontStruct *fs = w->text.fontStruct;
    int height = w->core.height, width = w->core.width;
    int marginWidth = w->text.marginWidth, marginHeight = w->text.marginHeight;

    TextDResize(w->text.textD, width - marginWidth*2, height - marginHeight*2);
    w->text.columns = (width - marginWidth*2) / fs->max_bounds.width;
    w->text.rows = (height - marginHeight*2) / (fs->ascent + fs->descent);
    
    /* if the window became shorter or narrower, there may be text left
       in the bottom or right margin area, which must be cleaned up */
    if (XtIsRealized((Widget)w)) {
	XClearArea(XtDisplay(w), XtWindow(w), 0, height-marginHeight,
    		width, marginHeight, False);
	XClearArea(XtDisplay(w), XtWindow(w), width-marginWidth, 0,
    		marginWidth, height, False);
    }
}

/*
** Widget redisplay method
*/
static void redisplay(TextWidget w, XEvent *event, Region region)
{
    XExposeEvent *e = (XExposeEvent *)event;
    
    TextDRedisplayRect(w->text.textD, e->x, e->y, e->width, e->height);
}

/*
** Widget setValues method
*/
static Boolean setValues(TextWidget current, TextWidget request,
	TextWidget new)
{
    Boolean redraw = False;
    
    if (new->text.overstrike != current->text.overstrike) {
    	if (current->text.textD->cursorStyle == BLOCK_CURSOR)
    	    TextDSetCursorStyle(current->text.textD,
    	    	    current->text.heavyCursor ? HEAVY_CURSOR : NORMAL_CURSOR);
    	else if (current->text.textD->cursorStyle == NORMAL_CURSOR ||
    		current->text.textD->cursorStyle == HEAVY_CURSOR)
    	    TextDSetCursorStyle(current->text.textD, BLOCK_CURSOR);
    }
    
    return redraw;
} 

/*
** Widget realize method
*/
static void realize(Widget w, XtValueMask *valueMask,
	XSetWindowAttributes *attributes)
{
    /* Set bit gravity window attribute.  This saves a full blank and redraw
       on window resizing */
    *valueMask |= CWBitGravity;
    attributes->bit_gravity = NorthWestGravity;
        
    /* Continue with realize method from superclass */
    (xmPrimitiveClassRec.core_class.realize)(w, valueMask, attributes);
}

/*
** Widget query geometry method ... experimental, not fully tested, does
** nothing in NEdit since paned window ignores children's suggested geometry
*/
static XtGeometryResult queryGeometry(Widget w, XtWidgetGeometry *proposed,
	XtWidgetGeometry *answer)
{
    int curHeight = ((TextWidget)w)->core.height;
    int curWidth = ((TextWidget)w)->core.width;
    XFontStruct *fs = ((TextWidget)w)->text.textD->fontStruct;
    int fontWidth = fs->max_bounds.width;
    int fontHeight = fs->ascent + fs->descent;
    int marginHeight = ((TextWidget)w)->text.marginHeight;
    int propWidth = (proposed->request_mode & CWWidth) ? proposed->width : 0;
    int propHeight = (proposed->request_mode & CWHeight) ? proposed->height : 0;
    
    /* suggest a height that is an exact multiple of the line height
       and at least one line high and 10 chars wide */
    answer->request_mode = CWHeight | CWWidth;
    answer->width = max(fontWidth * 10, propWidth);
    answer->height = max(1, ((propHeight - 2*marginHeight) / fontHeight)) *
    	    fontHeight + 2*marginHeight;
    /* printf("propWidth %d, propHeight %d, ansWidth %d, ansHeight %d\n",
    	    propWidth, propHeight, answer->width, answer->height); */
    if (propWidth == answer->width && propHeight == answer->height)
    	return XtGeometryYes;
    else if (answer->width == curWidth && answer->height == curHeight)
    	return XtGeometryNo;
    else
    	return XtGeometryAlmost;
}
    
/*
** Set the text buffer which this widget will display and interact with.
** The currently attached buffer is automatically freed, ONLY if it has
** no additional modify procs attached (as it would if it were being
** displayed by another text widget).
*/
void TextSetBuffer(Widget w, textBuffer *buffer)
{
    textBuffer *oldBuf = ((TextWidget)w)->text.textD->buffer;
    
    StopHandlingXSelections(w);
    TextDSetBuffer(((TextWidget)w)->text.textD, buffer);
    if (oldBuf->nModifyProcs == 0)
    	BufFree(oldBuf);
}

/*
** Get the buffer associated with this text widget.  Note that attaching
** additional modify callbacks to the buffer will prevent it from being
** automatically freed when the widget is destroyed.
*/
textBuffer *TextGetBuffer(Widget w)
{
    return ((TextWidget)w)->text.textD->buffer;
}

/*
** Translate a position into a line number (if the position is visible,
** if it's not, return False
*/
int TextPosToLineNum(Widget w, int pos, int *lineNum)
{
    return TextDPosToLineNum(((TextWidget)w)->text.textD, pos, lineNum);
}

/*
** Translate a buffer text position to the XY location where the center
** of the cursor would be positioned to point to that character.  Returns
** False if the position is not displayed because it is VERTICALLY out
** of view.  If the position is horizontally out of view, returns the
** x coordinate where the position would be if it were visible.
*/
int TextPosToXY(Widget w, int pos, int *x, int *y)
{
    return TextDPositionToXY(((TextWidget)w)->text.textD, pos, x, y);
}

/*
** Return the cursor position
*/
int TextGetCursorPos(Widget w)
{
    return TextDGetInsertPosition(((TextWidget)w)->text.textD);
}

/*
** Set the cursor position
*/
void TextSetCursorPos(Widget w, int pos)
{
    TextDSetInsertPosition(((TextWidget)w)->text.textD, pos);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, NULL);

}

/*
** Return the horizontal and vertical scroll positions of the widget
*/
void TextGetScroll(Widget w, int *topLineNum, int *horizOffset)
{
    TextDGetScroll(((TextWidget)w)->text.textD, topLineNum, horizOffset);
}

/*
** Set the horizontal and vertical scroll positions of the widget
*/
void TextSetScroll(Widget w, int topLineNum, int horizOffset)
{
    TextDSetScroll(((TextWidget)w)->text.textD, topLineNum, horizOffset);
}

/*
** Set this widget to be the owner of selections made in it's attached
** buffer (text buffers may be shared among several text widgets).
*/
void TextHandleXSelections(Widget w)
{
    HandleXSelections(w);
}

void TextStopHandlingSelections(Widget w)
{
    StopHandlingXSelections(w);
}

void TextPasteClipboard(Widget w, Time time)
{
    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, time);
    InsertClipboard(w, time, False);
    callCursorMovementCBs(w, NULL);
}

void TextColPasteClipboard(Widget w, Time time)
{
    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, time);
    InsertClipboard(w, time, True);
    callCursorMovementCBs(w, NULL);
}

void TextCopyClipboard(Widget w, Time time)
{
    cancelDrag(w);
    if (!((TextWidget)w)->text.textD->buffer->primary.selected) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    CopyToClipboard(w, time);
}

void TextCutClipboard(Widget w, Time time)
{
    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    if (!((TextWidget)w)->text.textD->buffer->primary.selected) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    TakeMotifDestination(w, time);
    CopyToClipboard (w, time);
    BufRemoveSelected(((TextWidget)w)->text.textD->buffer);
    checkAutoShowInsertPos(w);
}

int TextFirstVisiblePos(Widget w)
{
    return ((TextWidget)w)->text.textD->firstChar;
}

int TextLastVisiblePos(Widget w)
{
    return ((TextWidget)w)->text.textD->lastChar;
}

static void grabFocusAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    TextWidget tw = (TextWidget)w;
    textDisp *textD = tw->text.textD;
    int lastBtnDown = tw->text.lastBtnDown;
    int row, column;
    
    /* Indicate state for future events, PRIMARY_CLICKED indicates that
       the proper initialization has been done for primary dragging and/or
       multi-clicking.  Also record the timestamp for multi-click processing */
    tw->text.dragState = PRIMARY_CLICKED;
    tw->text.lastBtnDown = e->time;

    /* Become owner of the MOTIF_DESTINATION selection, making this widget
       the designated recipient of secondary quick actions in Motif XmText
       widgets and in other NEdit text widgets */
    TakeMotifDestination(w, e->time);
    
    /* Check for possible multi-click sequence in progress */
    if (tw->text.multiClickState != NO_CLICKS) {
	if (e->time < lastBtnDown + XtGetMultiClickTime(XtDisplay(w))) {
    	    if (tw->text.multiClickState == ONE_CLICK) {
    		selectWord(w);
    		TextDSetInsertPosition(textD, endOfWord(tw,
    			TextDGetInsertPosition(textD)));
    		callCursorMovementCBs(w, event);
		return;
    	    } else if (tw->text.multiClickState == TWO_CLICKS) {
    		selectLine(w);
    		TextDSetInsertPosition(textD, BufEndOfLine(textD->buffer,
    			TextDGetInsertPosition(textD)));
    		callCursorMovementCBs(w, event);
    		return;
    	    } else if (tw->text.multiClickState >= THREE_CLICKS)
    		tw->text.multiClickState = NO_CLICKS;
	} else
    	    tw->text.multiClickState = NO_CLICKS;
    }
    
    /* Clear any existing selections */
    BufUnselect(textD->buffer);
    
    /* Move the cursor to the pointer location */
    moveDestinationAP(w, event, args, nArgs);
    
    /* Record the site of the initial button press and the initial character
       position so subsequent motion events and clicking can decide when and
       where to begin a primary selection */
    tw->text.btnDownX = e->x;
    tw->text.btnDownY = e->y;
    tw->text.anchor = TextDGetInsertPosition(textD);
    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
    tw->text.rectAnchor = column;
}

static void moveDestinationAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
 
    /* Get input focus */
    XmProcessTraversal(w, XmTRAVERSE_CURRENT);

    /* Move the cursor */
    TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void extendAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    TextWidget tw = (TextWidget)w;
    XMotionEvent *e = (XMotionEvent *)event;
    int dragState = tw->text.dragState;
    int rectDrag = hasKey("rect", args, nArgs);

    /* Make sure the proper initialization was done on mouse down */
    if (dragState != PRIMARY_DRAG && dragState != PRIMARY_CLICKED &&
    	    dragState != PRIMARY_RECT_DRAG)
    	return;
    	
    /* If the selection hasn't begun, decide whether the mouse has moved
       far enough from the initial mouse down to be considered a drag */
    if (tw->text.dragState == PRIMARY_CLICKED) {
    	if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
    	    	abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
    	    tw->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;
    	else
    	    return;
    }
    
    /* If "rect" argument has appeared or disappeared, keep dragState up
       to date about which type of drag this is */
    tw->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;
    
    /* Record the new position for the autoscrolling timer routine, and
       engage or disengage the timer if the mouse is in/out of the window */
    checkAutoScroll(tw, e->x, e->y);
    
    /* Adjust the selection and move the cursor */
    adjustSelection(tw, e->x, e->y);
}

static void extendStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{    
    XMotionEvent *e = (XMotionEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *sel = &buf->primary;
    int anchor, rectAnchor, anchorLineStart, newPos, row, column;

    /* Find the new anchor point for the rest of this drag operation */
    newPos = TextDXYToPosition(textD, e->x, e->y);
    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
    if (sel->selected) {
    	if (sel->rectangular) {
    	    rectAnchor = column < (sel->rectEnd + sel->rectStart) / 2 ?
    	    	    sel->rectEnd : sel->rectStart;
    	    anchorLineStart = BufStartOfLine(buf, newPos <
    	    	    (sel->end + sel->start) / 2 ? sel->end : sel->start);
    	    anchor = BufCountForwardDispChars(buf, anchorLineStart, rectAnchor);
    	} else {
    	    if (abs(newPos - sel->start) < abs(newPos - sel->end))
    		anchor = sel->end;
    	    else
    		anchor = sel->start;
    	    anchorLineStart = BufStartOfLine(buf, anchor);
    	    rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
    	}
    } else {
    	anchor = TextDGetInsertPosition(textD);
    	anchorLineStart = BufStartOfLine(buf, anchor);
    	rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
    }
    ((TextWidget)w)->text.anchor = anchor;
    ((TextWidget)w)->text.rectAnchor = rectAnchor;

    /* Make the new selection */
    if (hasKey("rect", args, nArgs))
	BufRectSelect(buf, BufStartOfLine(buf, min(anchor, newPos)),
		BufEndOfLine(buf, max(anchor, newPos)),
		min(rectAnchor, column), max(rectAnchor, column));
    else
    	BufSelect(buf, min(anchor, newPos), max(anchor, newPos));
    
    /* Never mind the motion threshold, go right to dragging since
       extend-start is unambiguously the start of a selection */
    ((TextWidget)w)->text.dragState = PRIMARY_DRAG;
    
    /* Don't do by-word or by-line adjustment, just by character */
    ((TextWidget)w)->text.multiClickState = NO_CLICKS;

    /* Move the cursor */
    TextDSetInsertPosition(textD, newPos);
    callCursorMovementCBs(w, event);
}

static void extendEndAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    TextWidget tw = (TextWidget)w;
    
    if (tw->text.dragState == PRIMARY_CLICKED &&
    	    tw->text.lastBtnDown <= e->time + XtGetMultiClickTime(XtDisplay(w)))
    	tw->text.multiClickState++;
    endDrag(w);
}

static void processCancelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int dragState = ((TextWidget)w)->text.dragState;
    textBuffer *buf = ((TextWidget)w)->text.textD->buffer;
    
    if (dragState == PRIMARY_DRAG || dragState == PRIMARY_RECT_DRAG)
    	BufUnselect(buf);
    cancelDrag(w);
}

static void secondaryStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{    
    XMotionEvent *e = (XMotionEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *sel = &buf->secondary;
    int anchor, pos, row, column;

    /* Find the new anchor point and make the new selection */
    pos = TextDXYToPosition(textD, e->x, e->y);
    if (sel->selected) {
    	if (abs(pos - sel->start) < abs(pos - sel->end))
    	    anchor = sel->end;
    	else
    	    anchor = sel->start;
    	BufSecondarySelect(buf, anchor, pos);
    } else
    	anchor = pos;

    /* Record the site of the initial button press and the initial character
       position so subsequent motion events can decide when to begin a
       selection, (and where the selection began) */
    ((TextWidget)w)->text.btnDownX = e->x;
    ((TextWidget)w)->text.btnDownY = e->y;
    ((TextWidget)w)->text.anchor = pos;
    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
    ((TextWidget)w)->text.rectAnchor = column;
    ((TextWidget)w)->text.dragState = SECONDARY_CLICKED;
}

static void secondaryOrDragStartAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{    
    XMotionEvent *e = (XMotionEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;

    /* If the click was outside of the primary selection, this is not
       a drag, start a secondary selection */
    if (!buf->primary.selected || !TextDInSelection(textD, e->x, e->y)) {
    	secondaryStartAP(w, event, args, nArgs);
    	return;
    }

    if (checkReadOnly(w))
	return;

    /* Record the site of the initial button press and the initial character
       position so subsequent motion events can decide when to begin a
       drag, and where to drag to */
    ((TextWidget)w)->text.btnDownX = e->x;
    ((TextWidget)w)->text.btnDownY = e->y;
    ((TextWidget)w)->text.dragState = CLICKED_IN_SELECTION;
}

static void secondaryAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    TextWidget tw = (TextWidget)w;
    XMotionEvent *e = (XMotionEvent *)event;
    int dragState = tw->text.dragState;
    int rectDrag = hasKey("rect", args, nArgs);

    /* Make sure the proper initialization was done on mouse down */
    if (dragState != SECONDARY_DRAG && dragState != SECONDARY_RECT_DRAG &&
    	    dragState != SECONDARY_CLICKED)
    	return;
    	
    /* If the selection hasn't begun, decide whether the mouse has moved
       far enough from the initial mouse down to be considered a drag */
    if (tw->text.dragState == SECONDARY_CLICKED) {
    	if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
    	    	abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
    	    tw->text.dragState = rectDrag ? SECONDARY_RECT_DRAG: SECONDARY_DRAG;
    	else
    	    return;
    }
    
    /* If "rect" argument has appeared or disappeared, keep dragState up
       to date about which type of drag this is */
    tw->text.dragState = rectDrag ? SECONDARY_RECT_DRAG : SECONDARY_DRAG;
    
    /* Record the new position for the autoscrolling timer routine, and
       engage or disengage the timer if the mouse is in/out of the window */
    checkAutoScroll(tw, e->x, e->y);
    
    /* Adjust the selection */
    adjustSecondarySelection(tw, e->x, e->y);
}

static void secondaryOrDragAdjustAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    TextWidget tw = (TextWidget)w;
    XMotionEvent *e = (XMotionEvent *)event;
    int dragState = tw->text.dragState;

    /* Only dragging of blocks of text is handled in this action proc.
       Otherwise, defer to secondaryAdjust to handle the rest */
    if (dragState != CLICKED_IN_SELECTION && dragState != PRIMARY_BLOCK_DRAG) {
     	secondaryAdjustAP(w, event, args, nArgs);
    	return;
    }
    
    /* Decide whether the mouse has moved far enough from the
       initial mouse down to be considered a drag */
    if (tw->text.dragState == CLICKED_IN_SELECTION) {
    	if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
    	    	abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
    	    BeginBlockDrag(tw);
    	else
    	    return;
    }
    
    /* Record the new position for the autoscrolling timer routine, and
       engage or disengage the timer if the mouse is in/out of the window */
    checkAutoScroll(tw, e->x, e->y);
    
    /* Adjust the selection */
    BlockDragSelection(tw, e->x, e->y, hasKey("overlay", args, nArgs) ?
    	(hasKey("copy", args, nArgs) ? DRAG_OVERLAY_COPY : DRAG_OVERLAY_MOVE) :
    	(hasKey("copy", args, nArgs) ? DRAG_COPY : DRAG_MOVE));
}

static void copyToAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int dragState = ((TextWidget)w)->text.dragState;
    textBuffer *buf = textD->buffer;
    selection *secondary = &buf->secondary, *primary = &buf->primary;
    int rectangular = secondary->rectangular;
    char *textToCopy;
    int insertPos, lineStart, column, row;

    endDrag(w);
    if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
    	    dragState == SECONDARY_RECT_DRAG && secondary->selected ||
    	    dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
        return;
    if (!(secondary->selected && !((TextWidget)w)->text.motifDestOwner)) {
	if (checkReadOnly(w)) {
	    BufSecondaryUnselect(buf);
	    return;
	}
    }
    if (secondary->selected) {
    	if (((TextWidget)w)->text.motifDestOwner) {
	    TextDBlankCursor(textD);
	    textToCopy = BufGetSecSelectText(buf);
	    if (primary->selected && rectangular) {
    		insertPos = TextDGetInsertPosition(textD);
    		BufReplaceSelected(buf, textToCopy);
    		TextDSetInsertPosition(textD, buf->cursorPosHint);
	    } else if (rectangular) {
    		insertPos = TextDGetInsertPosition(textD);
    		lineStart = BufStartOfLine(buf, insertPos);
    		column = BufCountDispChars(buf, lineStart, insertPos);
    		BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
    		TextDSetInsertPosition(textD, buf->cursorPosHint);
	    } else
	    	insertAtCursor(w, textToCopy, event);
	    XtFree(textToCopy);
	    BufSecondaryUnselect(buf);
	    TextDUnblankCursor(textD);
	} else
	    SendSecondarySelection(w, e->time, False);
    } else if (primary->selected) {
	textToCopy = BufGetSelectionText(buf);
	insertPos = TextDXYToPosition(textD, e->x, e->y);
	BufInsert(buf, insertPos, textToCopy);
	TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
	XtFree(textToCopy);
	checkAutoShowInsertPos(w);
    } else {
    	TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
    	InsertPrimarySelection(w, e->time, False);
    }
}

static void copyToOrEndDragAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    int dragState = ((TextWidget)w)->text.dragState;

    if (dragState != PRIMARY_BLOCK_DRAG) {
    	copyToAP(w, event, args, nArgs);
    	return;
    }
    
    FinishBlockDrag((TextWidget)w);
}

static void moveToAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int dragState = ((TextWidget)w)->text.dragState;
    textBuffer *buf = textD->buffer;
    selection *secondary = &buf->secondary, *primary = &buf->primary;
    int insertPos, rectangular = secondary->rectangular;
    int column, row, lineStart;
    char *textToCopy;

    endDrag(w);
    if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
    	    dragState == SECONDARY_RECT_DRAG && secondary->selected ||
    	    dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
        return;
    if (checkReadOnly(w)) {
	BufSecondaryUnselect(buf);
	return;
    }
    
    if (secondary->selected) {
	if (((TextWidget)w)->text.motifDestOwner) {
	    textToCopy = BufGetSecSelectText(buf);
	    if (primary->selected && rectangular) {
    		insertPos = TextDGetInsertPosition(textD);
    		BufReplaceSelected(buf, textToCopy);
    		TextDSetInsertPosition(textD, buf->cursorPosHint);
	    } else if (rectangular) {
    		insertPos = TextDGetInsertPosition(textD);
    		lineStart = BufStartOfLine(buf, insertPos);
    		column = BufCountDispChars(buf, lineStart, insertPos);
    		BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
    		TextDSetInsertPosition(textD, buf->cursorPosHint);
	    } else
	    	insertAtCursor(w, textToCopy, event);
	    XtFree(textToCopy);
	    BufRemoveSecSelect(buf);
	    BufSecondaryUnselect(buf);
	} else
	    SendSecondarySelection(w, e->time, True);
    } else if (primary->selected) {
        textToCopy = BufGetRange(buf, primary->start, primary->end);
	insertPos = TextDXYToPosition(textD, e->x, e->y);
	BufInsert(buf, insertPos, textToCopy);
	TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
	XtFree(textToCopy);
	BufRemoveSelected(buf);
	BufUnselect(buf);
    } else {
    	TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
    	MovePrimarySelection(w, e->time, False);
    } 
}

static void moveToOrEndDragAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    int dragState = ((TextWidget)w)->text.dragState;

    if (dragState != PRIMARY_BLOCK_DRAG) {
    	moveToAP(w, event, args, nArgs);
    	return;
    }
    
    FinishBlockDrag((TextWidget)w);
}

static void exchangeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    XButtonEvent *e = (XButtonEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *sec = &buf->secondary, *primary = &buf->primary;
    char *primaryText, *secText;
    int newPrimaryStart, newPrimaryEnd, secWasRect;

    endDrag(w);
    if (checkReadOnly(w))
	return;

    /* If there's no secondary selection here, or the primary and secondary
       selection overlap, just beep and return */
    if (!sec->selected || (primary->selected &&
    	    ((primary->start <= sec->start && primary->end > sec->start) ||
    	     (sec->start <= primary->start && sec->end > primary->start)))) {
    	BufSecondaryUnselect(buf);
    	XBell(XtDisplay(w), 0);
    	return;
    }
    
    /* if the primary selection is in another widget, use selection routines */
    if (!primary->selected) {
    	ExchangeSelections(w, e->time);
    	return;
    }
    
    /* Both primary and secondary are in this widget, do the exchange here */
    primaryText = BufGetSelectionText(buf);
    secText = BufGetSecSelectText(buf);
    secWasRect = sec->rectangular;
    BufReplaceSecSelect(buf, primaryText);
    newPrimaryStart = primary->start;
    BufReplaceSelected(buf, secText);
    newPrimaryEnd = newPrimaryStart + strlen(secText);
    XtFree(primaryText);
    XtFree(secText);
    BufSecondaryUnselect(buf);
    if (secWasRect) {
    	TextDSetInsertPosition(textD, buf->cursorPosHint);
    } else {
	BufSelect(buf, newPrimaryStart, newPrimaryEnd);
	TextDSetInsertPosition(textD, newPrimaryEnd);
    }
    checkAutoShowInsertPos(w);
}

static void copyPrimaryAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    TextWidget tw = (TextWidget)w;
    textDisp *textD = tw->text.textD;
    textBuffer *buf = textD->buffer;
    selection *primary = &buf->primary;
    int rectangular = hasKey("rect", args, nArgs);
    char *textToCopy;
    int insertPos, col;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    if (primary->selected && rectangular) {
	textToCopy = BufGetSelectionText(buf);
	insertPos = TextDGetInsertPosition(textD);
	col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
	BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
	TextDSetInsertPosition(textD, buf->cursorPosHint);
	XtFree(textToCopy);
	checkAutoShowInsertPos(w);
    } else if (primary->selected) {
	textToCopy = BufGetSelectionText(buf);
	insertPos = TextDGetInsertPosition(textD);
	BufInsert(buf, insertPos, textToCopy);
	TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
	XtFree(textToCopy);
	checkAutoShowInsertPos(w);
    } else if (rectangular) {
    	if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
    		&tw->text.btnDownX, &tw->text.btnDownY))
    	    return; /* shouldn't happen */
    	InsertPrimarySelection(w, e->time, True);
    } else
    	InsertPrimarySelection(w, e->time, False);
}

static void cutPrimaryAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *primary = &buf->primary;
    char *textToCopy;
    int rectangular = hasKey("rect", args, nArgs);
    int insertPos, col;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    if (primary->selected && rectangular) {
	textToCopy = BufGetSelectionText(buf);
	insertPos = TextDGetInsertPosition(textD);
	col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
	BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
	TextDSetInsertPosition(textD, buf->cursorPosHint);
	XtFree(textToCopy);
	BufRemoveSelected(buf);
	checkAutoShowInsertPos(w);
    } else if (primary->selected) {
	textToCopy = BufGetSelectionText(buf);
	insertPos = TextDGetInsertPosition(textD);
	BufInsert(buf, insertPos, textToCopy);
	TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
	XtFree(textToCopy);
	BufRemoveSelected(buf);
	checkAutoShowInsertPos(w);
    } else if (rectangular) {
    	if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
    	    	&((TextWidget)w)->text.btnDownX,
    	    	&((TextWidget)w)->text.btnDownY))
    	    return; /* shouldn't happen */
    	MovePrimarySelection(w, e->time, True);
    } else {
    	MovePrimarySelection(w, e->time, False);
    }
}

static void pasteClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    if (hasKey("rect", args, nArgs))
    	TextColPasteClipboard(w, ((XKeyEvent *)event)->time);
    else
	TextPasteClipboard(w, ((XKeyEvent *)event)->time);
}

static void copyClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    TextCopyClipboard(w, ((XKeyEvent *)event)->time);
}

static void cutClipboardAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    TextCutClipboard(w, ((XKeyEvent *)event)->time);
}

static void insertStringAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{   
    if (*nArgs == 0)
    	return;
    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    insertAtCursor(w, args[0], event);
    BufUnselect((((TextWidget)w)->text.textD)->buffer);
}

static void selfInsertAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{   
    XKeyEvent *e = (XKeyEvent *)event;
    char chars[20];
    KeySym keysym;
    int nChars;

    nChars = XLookupString((XKeyEvent *)event, chars, 19, &keysym, NULL);
    if (nChars == 0)
    	return;
    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    chars[nChars] = '\0';
    if (!checkAutoWrap(w, chars, event))
    	insertAtCursor(w, chars, event);
    BufUnselect((((TextWidget)w)->text.textD)->buffer);
}

static void newlineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{   
    if (((TextWidget)w)->text.autoIndent)
    	newlineAndIndentAP(w, event, args, nArgs);
    else
    	newlineNoIndentAP(w, event, args, nArgs);
}

static void newlineNoIndentAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{   
    XKeyEvent *e = (XKeyEvent *)event;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    insertAtCursor(w, "\n", event);
    BufUnselect((((TextWidget)w)->text.textD)->buffer);
}

static void newlineAndIndentAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{   
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    char c, *indentStr, *indentPtr;
    int i, cursorPos, lineStartPos;
    
    if (checkReadOnly(w))
	return;
    cancelDrag(w);
    TakeMotifDestination(w, e->time);
    
    /* Create a string containing a newline followed by the leading
       whitespace copied from the current line with the cursor */
    cursorPos = TextDGetInsertPosition(textD);
    lineStartPos = BufStartOfLine(buf, cursorPos);
    indentStr = XtMalloc(cursorPos - lineStartPos + 2);
    indentPtr = indentStr;
    *indentPtr++ = '\n';
    for (i=lineStartPos; i<cursorPos; i++) {
	c=BufGetCharacter(buf, i);
	if (c != ' ' && c != '\t')
	    break;
	*indentPtr++ = c;
    }
    *indentPtr = '\0';
    
    /* Insert it at the cursor */
    insertAtCursor(w, indentStr, event);
    XtFree(indentStr);
    BufUnselect(buf);
}

static void processTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *sel = &buf->primary;
    int emTabDist = ((TextWidget)w)->text.emulateTabs;
    int emTabsBeforeCursor = ((TextWidget)w)->text.emTabsBeforeCursor;
    int insertPos, indent, startIndent, toIndent, lineStart, tabWidth;
    char *outStr, *outPtr;
    
    if (checkReadOnly(w))
	return;
    cancelDrag(w);
    TakeMotifDestination(w, ((XKeyEvent *)event)->time);
    
    /* If emulated tabs are off, just insert a tab */
    if (emTabDist <= 0) {
	if (!checkAutoWrap(w, "\t", event))
    	    insertAtCursor(w, "\t", event);
    	return;
    }

    /* Find the starting and ending indentation.  If the tab is to
       replace an existing selection, use the start of the selection
       instead of the cursor position as the indent.  When replacing
       rectangular selections, tabs are automatically recalculated as
       if the inserted text began at the start of the line */
    insertPos = pendingSelection(w) ?
    	    sel->start : TextDGetInsertPosition(textD);
    lineStart = BufStartOfLine(buf, insertPos);
    if (pendingSelection(w) && sel->rectangular)
    	insertPos = BufCountForwardDispChars(buf, lineStart, sel->rectStart);
    startIndent = BufCountDispChars(buf, lineStart, insertPos);
    toIndent = startIndent + emTabDist - (startIndent % emTabDist);
    if (pendingSelection(w) && sel->rectangular) {
    	toIndent -= startIndent;
    	startIndent = 0;
    }

    /* Allocate a buffer assuming all the inserted characters will be spaces */
    outStr = XtMalloc(toIndent - startIndent + 1);

    /* Add spaces and tabs to outStr until it reaches toIndent */
    outPtr = outStr;
    indent = startIndent;
    while (indent < toIndent) {
    	tabWidth = BufCharWidth('\t', indent, buf->tabDist);
    	if (buf->useTabs && tabWidth > 1 && indent + tabWidth <= toIndent) {
    	    *outPtr++ = '\t';
    	    indent += tabWidth;
    	} else {
    	    *outPtr++ = ' ';
    	    indent++;
    	}
    }
    *outPtr = '\0';
    
    /* Insert the emulated tab */
    insertAtCursor(w, outStr, event);
    XtFree(outStr);
    
    /* Restore and ++ emTabsBeforeCursor which was cleared by insertAtCursor */
    ((TextWidget)w)->text.emTabsBeforeCursor = emTabsBeforeCursor + 1;

    BufUnselect(buf);
}

static void deleteSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    deletePendingSelection(w, event);
}

static void deletePreviousCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);
    char c;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == 0) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    if (deleteEmulatedTab(w, event))
    	return;
    if (((TextWidget)w)->text.overstrike) {
    	c = BufGetCharacter(textD->buffer, insertPos - 1);
    	if (c == '\n')
    	    BufRemove(textD->buffer, insertPos - 1, insertPos);
    	else if (c != '\t')
    	    BufReplace(textD->buffer, insertPos - 1, insertPos, " ");
    } else
    	BufRemove(textD->buffer, insertPos - 1, insertPos);
    TextDSetInsertPosition(textD, insertPos - 1);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void deleteNextCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == textD->buffer->length) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    BufRemove(textD->buffer, insertPos , insertPos + 1);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void deletePreviousWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);
    int pos, lineStart = BufStartOfLine(textD->buffer, insertPos);
    char *delimiters = ((TextWidget)w)->text.delimiters;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == lineStart) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    pos = max(insertPos - 1, 0);
    while (strchr(delimiters, BufGetCharacter(textD->buffer, pos)) != NULL &&
    	    pos != lineStart)
    	pos--;
    pos = startOfWord((TextWidget)w, pos);
    BufRemove(textD->buffer, pos, insertPos);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void deleteNextWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);
    int pos, lineEnd = BufEndOfLine(textD->buffer, insertPos);
    char *delimiters = ((TextWidget)w)->text.delimiters;

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == lineEnd) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    pos = insertPos;
    while (strchr(delimiters, BufGetCharacter(textD->buffer, pos)) != NULL &&
    	    pos != lineEnd)
        pos++;
    pos = endOfWord((TextWidget)w, pos);
    BufRemove(textD->buffer, insertPos, pos);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void deleteToEndOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);
    int endOfLine = BufEndOfLine(textD->buffer, insertPos);

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == endOfLine) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    BufRemove(textD->buffer, insertPos, endOfLine);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void deleteToStartOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);
    int startOfLine = BufStartOfLine(textD->buffer, insertPos);

    cancelDrag(w);
    if (checkReadOnly(w))
	return;
    TakeMotifDestination(w, e->time);
    if (deletePendingSelection(w, event))
    	return;
    if (insertPos == startOfLine) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    BufRemove(textD->buffer, startOfLine, insertPos);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void forwardCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveRight(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void backwardCharacterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveLeft(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void forwardWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int pos, insertPos = TextDGetInsertPosition(textD);
    char *delimiters = ((TextWidget)w)->text.delimiters;

    cancelDrag(w);
    if (insertPos == buf->length) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    pos = insertPos;
    if (strchr(delimiters, BufGetCharacter(buf, pos)) == NULL)
        pos = endOfWord((TextWidget)w, pos);
    for (; pos<buf->length; pos++) {
    	if (strchr(delimiters, BufGetCharacter(buf, pos)) == NULL)
    	    break;
    }
    TextDSetInsertPosition(textD, pos);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void backwardWordAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int pos, insertPos = TextDGetInsertPosition(textD);
    char *delimiters = ((TextWidget)w)->text.delimiters;

    cancelDrag(w);
    if (insertPos == 0) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    pos = max(insertPos - 1, 0);
    while (strchr(delimiters, BufGetCharacter(buf, pos)) != NULL)
    	pos--;
    pos = startOfWord((TextWidget)w, pos);
    	
    TextDSetInsertPosition(textD, pos);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void forwardParagraphAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int pos, insertPos = TextDGetInsertPosition(textD);
    textBuffer *buf = textD->buffer;
    char c;
    static char whiteChars[] = " \t";

    cancelDrag(w);
    if (insertPos == buf->length) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    pos = min(BufEndOfLine(buf, insertPos)+1, buf->length);
    while (pos < buf->length) {
    	c = BufGetCharacter(buf, pos);
    	if (c == '\n')
    	    break;
    	if (strchr(whiteChars, c) != NULL)
    	    pos++;
    	else
    	    pos = min(BufEndOfLine(buf, pos)+1, buf->length);
    }
    TextDSetInsertPosition(textD, min(pos+1, buf->length));
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void backwardParagraphAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int parStart, pos, insertPos = TextDGetInsertPosition(textD);
    textBuffer *buf = textD->buffer;
    char c;
    static char whiteChars[] = " \t";

    cancelDrag(w);
    if (insertPos == 0) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    parStart = BufStartOfLine(buf, max(insertPos-1, 0));
    pos = max(parStart - 2, 0);
    while (pos > 0) {
    	c = BufGetCharacter(buf, pos);
    	if (c == '\n')
    	    break;
    	if (strchr(whiteChars, c) != NULL)
    	    pos--;
    	else {
    	    parStart = BufStartOfLine(buf, pos);
    	    pos = max(parStart - 2, 0);
    	}
    }
    TextDSetInsertPosition(textD, parStart);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void keySelectAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int stat, insertPos = TextDGetInsertPosition(textD);
    
    cancelDrag(w);
    if (hasKey("left", args, nArgs)) stat = TextDMoveLeft(textD);
    else if (hasKey("right", args, nArgs)) stat = TextDMoveRight(textD);
    else if (hasKey("up", args, nArgs)) stat = TextDMoveUp(textD);
    else if (hasKey("down", args, nArgs)) stat = TextDMoveDown(textD);
    else {
    	keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
    	return;
    }
    if (!stat)
    	XBell(XtDisplay(w), 0);
    else {
	keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
	checkAutoShowInsertPos(w);
    }
}

static void processUpAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveUp(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void processShiftUpAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveUp(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void processDownAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveDown(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void processShiftDownAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    cancelDrag(w);
    if (!TextDMoveDown(((TextWidget)w)->text.textD))
    	XBell(XtDisplay(w), 0);
    keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void beginningOfLineAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);

    cancelDrag(w);
    TextDSetInsertPosition(textD, BufStartOfLine(textD->buffer, insertPos));
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void endOfLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);

    cancelDrag(w);
    TextDSetInsertPosition(textD, BufEndOfLine(textD->buffer, insertPos));
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void beginningOfFileAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);

    cancelDrag(w);
    TextDSetInsertPosition(((TextWidget)w)->text.textD, 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void endOfFileAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int insertPos = TextDGetInsertPosition(textD);

    cancelDrag(w);
    TextDSetInsertPosition(textD, textD->buffer->length);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void nextPageAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int lastTopLine = textD->nBufferLines - (textD->nVisibleLines - 2);
    int insertPos = TextDGetInsertPosition(textD);
    int i, pos, targetLine;

    cancelDrag(w);
    if (insertPos >= buf->length && textD->topLineNum == lastTopLine) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    targetLine = textD->topLineNum + textD->nVisibleLines - 1;
    if (targetLine < 1) targetLine = 1;
    if (targetLine > lastTopLine) targetLine = lastTopLine;
    pos = insertPos;
    for (i=0; i<textD->nVisibleLines-1; i++)
    	pos = min(BufEndOfLine(buf, pos) + 1, buf->length);
    TextDSetInsertPosition(textD, pos);
    TextDSetScroll(textD, targetLine, textD->horizOffset);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void previousPageAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int insertPos = TextDGetInsertPosition(textD);
    int i, pos, targetLine;

    cancelDrag(w);
    if (insertPos <= 0 && textD->topLineNum == 1) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    targetLine = textD->topLineNum - (textD->nVisibleLines - 1);
    if (targetLine < 1)
    	targetLine = 1;
    pos = insertPos;
    for (i=0; i<textD->nVisibleLines-1; i++)
    	pos = BufStartOfLine(buf, max(pos-1, 0));
    TextDSetInsertPosition(textD, pos);
    TextDSetScroll(textD, targetLine, textD->horizOffset);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void pageLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int insertPos = TextDGetInsertPosition(textD);
    int maxCharWidth = textD->fontStruct->max_bounds.width;
    int lineStartPos, indent, pos;

    cancelDrag(w);
    lineStartPos = BufStartOfLine(buf, insertPos);
    if (insertPos == lineStartPos && textD->horizOffset == 0) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    indent = BufCountDispChars(buf, lineStartPos, insertPos);
    pos = BufCountForwardDispChars(buf, lineStartPos,
    	    max(0, indent - textD->width / maxCharWidth));
    TextDSetInsertPosition(textD, pos);
    TextDSetScroll(textD, textD->topLineNum,
    	    max(0, textD->horizOffset - textD->width));
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void pageRightAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    int insertPos = TextDGetInsertPosition(textD);
    int maxCharWidth = textD->fontStruct->max_bounds.width;
    int oldHorizOffset = textD->horizOffset;
    int lineStartPos, indent, pos;

    cancelDrag(w);
    lineStartPos = BufStartOfLine(buf, insertPos);
    indent = BufCountDispChars(buf, lineStartPos, insertPos);
    pos = BufCountForwardDispChars(buf, lineStartPos,
    	    indent + textD->width / maxCharWidth);
    TextDSetInsertPosition(textD, pos);
    TextDSetScroll(textD, textD->topLineNum, textD->horizOffset + textD->width);
    if (textD->horizOffset == oldHorizOffset && insertPos == pos)
    	XBell(XtDisplay(w), 0);
    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

static void toggleOverstrikeAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    TextWidget tw = (TextWidget)w;
    
    if (tw->text.overstrike) {
    	tw->text.overstrike = False;
    	TextDSetCursorStyle(tw->text.textD,
    	    	tw->text.heavyCursor ? HEAVY_CURSOR : NORMAL_CURSOR);
    } else {
    	tw->text.overstrike = True;
    	if (    tw->text.textD->cursorStyle == NORMAL_CURSOR ||
    		tw->text.textD->cursorStyle == HEAVY_CURSOR)
    	    TextDSetCursorStyle(tw->text.textD, BLOCK_CURSOR);
    }
}

static void scrollUpAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int topLineNum, horizOffset, nLines;

    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
    	return;
    TextDGetScroll(textD, &topLineNum, &horizOffset);
    TextDSetScroll(textD, topLineNum-nLines, horizOffset);
}

static void scrollDownAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int topLineNum, horizOffset, nLines;
    
    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
    	return;
    TextDGetScroll(textD, &topLineNum, &horizOffset);
    TextDSetScroll(textD, topLineNum+nLines, horizOffset);
}

static void scrollToLineAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    int topLineNum, horizOffset, lineNum;
    
    if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1)
    	return;
    TextDGetScroll(textD, &topLineNum, &horizOffset);
    TextDSetScroll(textD, lineNum, horizOffset);
}
  
static void selectAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textBuffer *buf = ((TextWidget)w)->text.textD->buffer;

    cancelDrag(w);
    BufSelect(buf, 0, buf->length);
}

static void deselectAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    cancelDrag(w);
    BufUnselect(((TextWidget)w)->text.textD->buffer);
}

static void focusInAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    TextWidget tw = (TextWidget)w;
    textDisp *textD = tw->text.textD;

    /* I don't entirely understand the traversal mechanism in Motif widgets,
       particularly, what leads to this widget getting a focus-in event when
       it does not actually have the input focus.  The temporary solution is
       to do the comparison below, and not show the cursor when Motif says
       we don't have focus, but keep looking for the real answer */
#if XmVersion >= 1002
    if (w != XmGetFocusWidget(w))
    	return;
#endif

    /* If the timer is not already started, start it */
    if (tw->text.cursorBlinkRate != 0 && tw->text.cursorBlinkProcID == 0) {
    	tw->text.cursorBlinkProcID = XtAppAddTimeOut(
    	    	XtWidgetToApplicationContext((Widget)w),
    		tw->text.cursorBlinkRate, cursorBlinkTimerProc, w);
    }
    
    /* Change the cursor to active style */
    if (((TextWidget)w)->text.overstrike)
    	TextDSetCursorStyle(textD, BLOCK_CURSOR);
    else
    	TextDSetCursorStyle(textD, ((TextWidget)w)->text.heavyCursor ?
    		HEAVY_CURSOR : NORMAL_CURSOR);
    TextDUnblankCursor(textD);

    /* Call any registered focus-in callbacks */
    XtCallCallbacks((Widget)w, textNfocusCallback, (XtPointer)event);
}

static void focusOutAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    
    /* Remove the cursor blinking timer procedure */
    if (((TextWidget)w)->text.cursorBlinkProcID != 0)
    	XtRemoveTimeOut(((TextWidget)w)->text.cursorBlinkProcID);
    ((TextWidget)w)->text.cursorBlinkProcID = 0;

    /* Leave a dim or destination cursor */
    TextDSetCursorStyle(textD, ((TextWidget)w)->text.motifDestOwner ?
    	    CARET_CURSOR : DIM_CURSOR);
    TextDUnblankCursor(textD);

    /* Call any registered focus-out callbacks */
    XtCallCallbacks((Widget)w, textNlosingFocusCallback, (XtPointer)event);
}

/*
** For actions involving cursor movement, "extend" keyword means incorporate
** the new cursor position in the selection, and lack of an "extend" keyword
** means cancel the existing selection
*/
static void checkMoveSelectionChange(Widget w, XEvent *event, int startPos,
	String *args, Cardinal *nArgs)
{
    if (hasKey("extend", args, nArgs))
    	keyMoveExtendSelection(w, event, startPos, hasKey("rect", args, nArgs));
    else
    	BufUnselect((((TextWidget)w)->text.textD)->buffer);
}

/*
** If a selection change was requested via a keyboard command for moving
** the insertion cursor (usually with the "extend" keyword), adjust the
** selection to include the new cursor position, or begin a new selection
** between startPos and the new cursor position with anchor at startPos.
*/
static void keyMoveExtendSelection(Widget w, XEvent *event, int origPos,
	int rectangular)
{
    XKeyEvent *e = (XKeyEvent *)event;
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    selection *sel = &buf->primary;
    int newPos = TextDGetInsertPosition(textD);
    int startPos, endPos, col, startCol, endCol, newCol, origCol;
    int anchor, rectAnchor, anchorLineStart;

    /* Moving the cursor does not take the Motif destination, but as soon as
       the user selects something, grab it (I'm not sure if this distinction
       actually makes sense, but it's what Motif was doing, back when their
       secondary selections actually worked correctly) */
    TakeMotifDestination(w, e->time);
    
    /* Extend the selection based on original and new selection type */
    if (sel->selected && sel->rectangular && rectangular) { /* rect -> rect */
	col = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
    	rectAnchor = col < (sel->rectEnd + sel->rectStart) / 2 ?
    	    	sel->rectEnd : sel->rectStart;
    	anchorLineStart = BufStartOfLine(buf, newPos <
    	    	(sel->end + sel->start) / 2 ? sel->end : sel->start);
    	anchor = BufCountForwardDispChars(buf, anchorLineStart, rectAnchor);
	startCol = min(rectAnchor, col);
	endCol = max(rectAnchor, col);
	startPos = BufStartOfLine(buf, min(anchor, newPos));
	endPos = BufEndOfLine(buf, max(anchor, newPos));
	BufRectSelect(buf, startPos, endPos, startCol, endCol);
    } else if (sel->selected && sel->rectangular) { /* rect -> plain */
    	startPos = BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, sel->start), sel->rectStart);
    	endPos = BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, sel->end), sel->rectEnd);
    	if (abs(origPos - startPos) < abs(origPos - endPos))
    	    anchor = endPos;
    	else
    	    anchor = startPos;
    	BufSelect(buf, anchor, newPos);
    } else if (sel->selected && rectangular) { /* plain -> rect */
    	startCol = BufCountDispChars(buf,
    		BufStartOfLine(buf, sel->start), sel->start);
    	endCol = BufCountDispChars(buf,
    		BufStartOfLine(buf, sel->end), sel->end);
	newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
	startPos = BufStartOfLine(buf, min(sel->start, newPos));
	endPos = BufEndOfLine(buf, max(sel->end, newPos));
	BufRectSelect(buf, startPos, endPos, startCol, endCol);
    } else if (sel->selected) { /* plain -> plain */
    	if (abs(origPos - sel->start) < abs(origPos - sel->end))
    	    anchor = sel->end;
    	else
    	    anchor = sel->start;
     	BufSelect(buf, anchor, newPos);
    } else if (rectangular) { /* no sel -> rect */
	origCol = BufCountDispChars(buf, BufStartOfLine(buf, origPos), origPos);
	newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
	startCol = min(newCol, origCol);
	endCol = max(newCol, origCol);
	startPos = BufStartOfLine(buf, min(origPos, newPos));
	endPos = BufEndOfLine(buf, max(origPos, newPos));
	BufRectSelect(buf, startPos, endPos, startCol, endCol);
    } else { /* no sel -> plain */
    	anchor = origPos;
        BufSelect(buf, anchor, newPos);
    }
}

static void checkAutoShowInsertPos(Widget w)
{
    if (((TextWidget)w)->text.autoShowInsertPos)
    	TextDMakeInsertPosVisible(((TextWidget)w)->text.textD);
}

static int checkReadOnly(Widget w)
{
    if (((TextWidget)w)->text.readOnly) {
    	XBell(XtDisplay(w), 0);
	return True;
    }
    return False;
}

static void insertAtCursor(Widget w, char *chars, XEvent *event)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;
    char *c;

    if (pendingSelection(w)) {
    	BufReplaceSelected(buf, chars);
    	TextDSetInsertPosition(textD, buf->cursorPosHint);
    } else if (((TextWidget)w)->text.overstrike) {
    	for (c=chars; *c!='\0' && *c!='\n'; c++);
    	if (*c == '\n')
    	    TextDInsert(textD, chars);
    	else
    	    TextDOverstrike(textD, chars);
    } else
    	TextDInsert(textD, chars);
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);
}

/*
** If there's a selection, delete it and position the cursor where the
** selection was deleted.  (Called by routines which do deletion to check
** first for and do possible selection delete)
*/
static int deletePendingSelection(Widget w, XEvent *event)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = textD->buffer;

    if (((TextWidget)w)->text.textD->buffer->primary.selected) {
	BufRemoveSelected(buf);
	TextDSetInsertPosition(textD, buf->cursorPosHint);
    	checkAutoShowInsertPos(w);
    	callCursorMovementCBs(w, event);
	return True;
    } else
	return False;
}

/*
** Return true if pending delete is on and there's a selection contiguous
** with the cursor ready to be deleted.  These criteria are used to decide
** if typing a character or inserting something should delete the selection
** first.
*/
static int pendingSelection(Widget w)
{
    selection *sel = &((TextWidget)w)->text.textD->buffer->primary;
    int pos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    return ((TextWidget)w)->text.pendingDelete && sel->selected &&
    	    pos >= sel->start && pos <= sel->end;
}

/*
** Check if tab emulation is on and if there are emulated tabs before the
** cursor, and if so, delete an emulated tab as a unit.  Also finishes up
** by calling checkAutoShowInsertPos and callCursorMovementCBs, so the
** calling action proc can just return (this is necessary to preserve
** emTabsBeforeCursor which is otherwise cleared by callCursorMovementCBs).
*/
static int deleteEmulatedTab(Widget w, XEvent *event)
{
    textDisp *textD = ((TextWidget)w)->text.textD;
    textBuffer *buf = ((TextWidget)w)->text.textD->buffer;
    int emTabDist = ((TextWidget)w)->text.emulateTabs;
    int emTabsBeforeCursor = ((TextWidget)w)->text.emTabsBeforeCursor;
    int startIndent, toIndent, insertPos, startPos, lineStart;
    char *removedText;
    
    if (emTabDist <= 0 || emTabsBeforeCursor <= 0)
    	return False;
    
    /* Find the position to delete back to */
    insertPos = TextDGetInsertPosition(textD);
    lineStart = BufStartOfLine(buf, insertPos);
    startIndent = BufCountDispChars(buf, lineStart, insertPos);
    toIndent = (startIndent-1) - ((startIndent-1) % emTabDist);
    startPos = BufCountForwardDispChars(buf, lineStart, toIndent);
    
    /* Verify that the intervening characters are only tabs and spaces */
    removedText = BufGetRange(buf, startPos, insertPos);
    if (strspn(removedText, "\t ") != strlen(removedText))
    	return False;
    XtFree(removedText);
    
    /* Delete the characters and reposition the cursor */
    BufRemove(buf, startPos, insertPos);
    TextDSetInsertPosition(textD, startPos);

    /* The normal cursor movement stuff would usually be called by the action
       routine, but this wraps around it to restore emTabsBeforeCursor */
    checkAutoShowInsertPos(w);
    callCursorMovementCBs(w, event);

    /* Decrement and restore the marker for consecutive emulated tabs, which
       would otherwise have been zeroed by callCursorMovementCBs */
    ((TextWidget)w)->text.emTabsBeforeCursor = emTabsBeforeCursor - 1;
    return True;
}

/*
** Select the word adjacent to the cursor
*/
static void selectWord(Widget w)
{
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    
    BufSelect(((TextWidget)w)->text.textD->buffer, startOfWord((TextWidget)w,
    	    insertPos), endOfWord((TextWidget)w, insertPos));
}

static int startOfWord(TextWidget w, int pos)
{
    int startPos;
    
    if (!BufSearchBackward(w->text.textD->buffer, pos, w->text.delimiters,
    	    &startPos))
    	return 0;
    return min(pos, startPos+1);
}

static int endOfWord(TextWidget w, int pos)
{
    textBuffer *buf = w->text.textD->buffer;
    int endPos;
    
    if (!BufSearchForward(buf, pos, w->text.delimiters, &endPos))
    	endPos = buf->length;
    return endPos;
}

/*
** Select the line containing the cursor, including the terminating newline
*/
static void selectLine(Widget w)
{
    textBuffer *buf = ((TextWidget)w)->text.textD->buffer;
    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
    int endPos;
   
    endPos = BufEndOfLine(buf,insertPos);
    BufSelect(buf, BufStartOfLine(buf, insertPos), min(endPos+1, buf->length));
}

/*
** Given a new mouse pointer location, pass the position on to the 
** autoscroll timer routine, and make sure the timer is on when it's
** needed and off when it's not.
*/
static void checkAutoScroll(TextWidget w, int x, int y)
{
    int inWindow;
    
    /* Is the pointer in or out of the window? */
    inWindow = x >= w->text.marginWidth &&
    	    x < w->core.width - w->text.marginWidth &&
    	    y >= w->text.marginHeight &&
    	    y < w->core.height - w->text.marginHeight;
    
    /* If it's in the window, cancel the timer procedure */
    if (inWindow) {
    	if (w->text.autoScrollProcID != 0)
    	    XtRemoveTimeOut(w->text.autoScrollProcID);;
    	w->text.autoScrollProcID = 0;
    	return;
    }

    /* If the timer is not already started, start it */
    if (w->text.autoScrollProcID == 0) {
    	w->text.autoScrollProcID = XtAppAddTimeOut(
    	    	XtWidgetToApplicationContext((Widget)w),
    		0, autoScrollTimerProc, w);
    }
    
    /* Pass on the newest mouse location to the autoscroll routine */
    w->text.mouseX = x;
    w->text.mouseY = y;
}

/*
** Reset drag state and cancel the auto-scroll timer
*/
static void endDrag(Widget w)
{
    if (((TextWidget)w)->text.autoScrollProcID != 0)
    	XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
    ((TextWidget)w)->text.autoScrollProcID = 0;
    ((TextWidget)w)->text.dragState = NOT_CLICKED;
}

/*
** Cancel any drag operation that might be in progress.  Should be included
** in nearly every key event to cleanly end any dragging before edits are made
** which might change the insert position or the content of the buffer during
** a drag operation)
*/
static void cancelDrag(Widget w)
{
    int dragState = ((TextWidget)w)->text.dragState;
    
    if (((TextWidget)w)->text.autoScrollProcID != 0)
    	XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
    if (dragState == SECONDARY_DRAG || dragState == SECONDARY_RECT_DRAG)
    	BufSecondaryUnselect(((TextWidget)w)->text.textD->buffer);
    if (dragState == PRIMARY_BLOCK_DRAG)
    	CancelBlockDrag((TextWidget)w);
    if (dragState != NOT_CLICKED)
    	((TextWidget)w)->text.dragState = DRAG_CANCELED;
}

/*
** Do operations triggered by cursor movement: Call cursor movement callback
** procedure(s), and cancel marker indicating that the cursor is after one or
** more just-entered emulated tabs (spaces to be deleted as a unit).
*/
static void callCursorMovementCBs(Widget w, XEvent *event)
{
    ((TextWidget)w)->text.emTabsBeforeCursor = 0;
    XtCallCallbacks((Widget)w, textNcursorMovementCallback, (XtPointer)event);
}

/*
** Adjust the selection as the mouse is dragged to position: (x, y).
*/
static void adjustSelection(TextWidget tw, int x, int y)
{
    textDisp *textD = tw->text.textD;
    textBuffer *buf = textD->buffer;
    int row, col, startCol, endCol, startPos, endPos;
    int newPos = TextDXYToPosition(textD, x, y);
    
    /* Adjust the selection */
    if (tw->text.dragState == PRIMARY_RECT_DRAG) {
	TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
	startCol = min(tw->text.rectAnchor, col);
	endCol = max(tw->text.rectAnchor, col);
	startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
	endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
	BufRectSelect(buf, startPos, endPos, startCol, endCol);
    } else if (tw->text.multiClickState == ONE_CLICK) {
    	startPos = startOfWord(tw, min(tw->text.anchor, newPos));
    	endPos = endOfWord(tw, max(tw->text.anchor, newPos));
    	BufSelect(buf, startPos, endPos);
    	newPos = newPos < tw->text.anchor ? startPos : endPos;
    } else if (tw->text.multiClickState == TWO_CLICKS) {
        startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
        endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
        BufSelect(buf, startPos, min(endPos+1, buf->length));
        newPos = newPos < tw->text.anchor ? startPos : endPos;
    } else
    	BufSelect(buf, tw->text.anchor, newPos);
    
    /* Move the cursor */
    TextDSetInsertPosition(textD, newPos);
    callCursorMovementCBs((Widget)tw, NULL);
}

static void adjustSecondarySelection(TextWidget tw, int x, int y)
{
    textDisp *textD = tw->text.textD;
    textBuffer *buf = textD->buffer;
    int row, col, startCol, endCol, startPos, endPos;
    int newPos = TextDXYToPosition(textD, x, y);

    if (tw->text.dragState == SECONDARY_RECT_DRAG) {
	TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
	startCol = min(tw->text.rectAnchor, col);
	endCol = max(tw->text.rectAnchor, col);
	startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
	endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
	BufSecRectSelect(buf, startPos, endPos, startCol, endCol);
    } else
    	BufSecondarySelect(textD->buffer, tw->text.anchor, newPos);
}
	
/*
** Checks autoWrap is on and if inserting "chars" puts the cursor beyond
** the wrap margin.  If so, inserts a newline (and possibly additional
** characters if autoIndent is on), followed by "chars" and returns True.
** otherwise returns False and does not insert the characters.  "chars"
** should normally be only a single character, or whatever is returned
** from XLookupString
*/
static int checkAutoWrap(Widget w, char *chars, XEvent *event)
{
    int p, wrapMargin, colNum, lineLen, lineStartPos, replacePos, cursorPos;
    char ch, *c, *indentStr, *indentPtr, expandedChar[MAX_EXP_CHAR_LEN];
    TextWidget tw = (TextWidget)w;
    textDisp *textD = tw->text.textD;
    textBuffer *buf = textD->buffer;
    int charWidth, fontWidth = textD->fontStruct->max_bounds.width;

    /* Don't interfere if auto-wrap is off or the character is a newline */
    if (!tw->text.autoWrap || chars[0] == '\n')
        return False;
    
    /* Decide whether to wrap */
    wrapMargin = tw->text.wrapMargin != 0 ? tw->text.wrapMargin :
            (tw->core.width - tw->text.marginWidth) / fontWidth;
    cursorPos = TextDGetInsertPosition(textD);
    lineStartPos = BufStartOfLine(buf, cursorPos);
    lineLen = cursorPos - lineStartPos;
    colNum = BufCountDispChars(buf, lineStartPos, cursorPos);
    charWidth = 0;
    for (c=chars; *c!='\0'; c++)
        charWidth += BufExpandCharacter(*c, colNum+charWidth, expandedChar,
                buf->tabDist);
    if (colNum < wrapMargin-charWidth)
        return False;
    
    /* Find the point to break the line, if there's no whitespace, don't wrap */
    for (replacePos=cursorPos-1, ch=BufGetCharacter(buf, replacePos);
    	    replacePos>=lineStartPos && ch!='\t' && ch!=' ';
    	    replacePos--, ch=BufGetCharacter(buf, replacePos));
    if (replacePos < lineStartPos)
	return False;
    
    /* Create a newline-and-indent string for inserting to line up the
       indentation (or just a newline if auto-indent is off).  Skip auto-wrap
       if auto-indent would bring the line back to the same length (line has
       only leading whitespace).  Skip auto-indent if line is all white */
    if (tw->text.autoIndent) {
        indentStr = XtMalloc(lineLen + 2);
        indentPtr = indentStr;
        *indentPtr++ = '\n';
	for (p=lineStartPos, ch=BufGetCharacter(buf, p); ch=='\t' || ch==' ';
		p++, ch=BufGetCharacter(buf, p)) {
	    if (p >= cursorPos-1) {
	    	strcpy(indentStr, "\n");
	    	break;
	    }
	    if (p >= replacePos) {
	    	XtFree(indentStr);
	    	return False;
	    }
	    *indentPtr++ = ch;
	}
	*indentPtr = '\0';
    } else
        indentStr = "\n";
    
    /* If the character to insert is white space, just insert the
       newline-and-indent string, otherwise break the line at replacePos
       with the newline-and-indent string and insert "chars" at the cursor */
    if (!strcmp(chars, " ") || !strcmp(chars, "\t")) {
        insertAtCursor(w, indentStr, event);
    } else {
        BufReplace(buf, replacePos, replacePos+1, indentStr);
        insertAtCursor(w, chars, event);
    }
    if (tw->text.autoIndent)
        XtFree(indentStr);
    return True;
}

/*
** Xt timer procedure for autoscrolling
*/
static void autoScrollTimerProc(XtPointer clientData, XtIntervalId *id)
{
    TextWidget w = (TextWidget)clientData;
    textDisp *textD = w->text.textD;
    int topLineNum, horizOffset, newPos, cursorX, y;
    int fontWidth = textD->fontStruct->max_bounds.width;
    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;

    /* For vertical autoscrolling just dragging the mouse outside of the top
       or bottom of the window is sufficient, for horizontal (non-rectangular)
       scrolling, see if the position where the CURSOR would go is outside */
    newPos = TextDXYToPosition(textD, w->text.mouseX, w->text.mouseY);
    if (w->text.dragState == PRIMARY_RECT_DRAG)
    	cursorX = w->text.mouseX;
    else if (!TextDPositionToXY(textD, newPos, &cursorX, &y))
    	cursorX = w->text.mouseX;
    
    /* Scroll 1 character away from the pointer */
    TextDGetScroll(textD, &topLineNum, &horizOffset);
    if (cursorX >= (int)w->core.width - w->text.marginWidth)
    	horizOffset += fontWidth;
    else if (w->text.mouseX < w->text.marginWidth)
    	horizOffset -= fontWidth;
    if (w->text.mouseY >= (int)w->core.height - w->text.marginHeight)
    	topLineNum++;
    else if (w->text.mouseY < w->text.marginHeight)
    	topLineNum--;
    TextDSetScroll(textD, topLineNum, horizOffset);
    
    /* Continue the drag operation in progress.  If none is in progress
       (safety check) don't continue to re-establish the timer proc */
    if (w->text.dragState == PRIMARY_DRAG) {
	adjustSelection(w, w->text.mouseX, w->text.mouseY);
    } else if (w->text.dragState == PRIMARY_RECT_DRAG) {
	adjustSelection(w, w->text.mouseX, w->text.mouseY);
    } else if (w->text.dragState == SECONDARY_DRAG) {
	adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
    } else if (w->text.dragState == SECONDARY_RECT_DRAG) {
	adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
    } else if (w->text.dragState == PRIMARY_BLOCK_DRAG) {
    	BlockDragSelection(w, w->text.mouseX, w->text.mouseY, USE_LAST);
    } else {
    	w->text.autoScrollProcID = 0;
    	return;
    }
    
    /* re-establish the timer proc (this routine) to continue processing */
    w->text.autoScrollProcID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext((Widget)w),
    	    w->text.mouseY >= w->text.marginHeight &&
    	    w->text.mouseY < w->core.height - w->text.marginHeight ?
    	    (VERTICAL_SCROLL_DELAY*fontWidth) / fontHeight :
    	    VERTICAL_SCROLL_DELAY, autoScrollTimerProc, w);
}

/*
** Xt timer procedure for cursor blinking
*/
static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId *id)
{
    TextWidget w = (TextWidget)clientData;
    textDisp *textD = w->text.textD;
    
    /* Blink the cursor */
    if (textD->cursorOn)
    	TextDBlankCursor(textD);
    else
    	TextDUnblankCursor(textD);
    	
    /* re-establish the timer proc (this routine) to continue processing */
    w->text.cursorBlinkProcID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext((Widget)w),
    	    w->text.cursorBlinkRate, cursorBlinkTimerProc, w);
}

/*
** look at an action procedure's arguments to see if argument "key" has been
** specified in the argument list
*/
static int hasKey(char *key, String *args, Cardinal *nArgs)
{
    int i;
    
    for (i=0; i<*nArgs; i++)
    	if (!strCaseCmp(args[i], key))
    	    return True;
    return False;
}

static int max(int i1, int i2)
{
    return i1 >= i2 ? i1 : i2;
}

static int min(int i1, int i2)
{
    return i1 <= i2 ? i1 : i2;
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

/*******************************************************************************
*									       *
* textDisp.c - Display text from a text buffer				       *
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
* June 15, 1995								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include "textBuf.h"
#include "textDisp.h"

#define TOP_MARGIN 1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN 3
#define RIGHT_MARGIN 3

/* Masks for text drawing methods.  These are or'd together to form an
   integer which describes what drawing calls to use to draw a string */
#define FILL_MASK 0x100
#define SECONDARY_MASK 0x200
#define PRIMARY_MASK 0x400
#define HIGHLIGHT_MASK 0x800
#define STYLE_LOOKUP_MASK 0xff

/* Maximum displayable line length (how many characters will fit across the
   widest window).  This amount of memory is temporarily allocated from the
   stack in the redisplayLine routine for drawing strings */
#define MAX_DISP_LINE_LEN 1000

enum positionTypes {CURSOR_POS, CHARACTER_POS};

static void updateLineStarts(textDisp *textD, int pos, int charsInserted,
	int charsDeleted, int linesInserted, int linesDeleted, int *scrolled);
static void offsetLineStarts(textDisp *textD, int newTopLineNum);
static void calcLineStarts(textDisp *textD, int startLine, int endLine);
static void calcLastChar(textDisp *textD);
static int posToVisibleLineNum(textDisp *textD, int pos, int *lineNum);
static void redisplayLine(textDisp *textD, int visLineNum, int leftClip,
	int rightClip, int leftCharIndex, int rightCharIndex);
static void drawString(textDisp *textD, int style, int x, int y, int toX,
	char *string, int nChars);
static void clearRect(textDisp *textD, int style, int x, int y, 
    	int width, int height);
static void drawCursor(textDisp *textD, int x, int y);
static int styleOfPos(textDisp *textD, int lineStartPos,
    	int lineLen, int lineIndex, int dispIndex);
static int stringWidth(textDisp *textD, char *string, int length, int style);
static int inSelection(selection *sel, int pos, int lineStartPos,
	int dispIndex);
static int xyToPos(textDisp *textD, int x, int y, int posType);
static void xyToUnconstrainedPos(textDisp *textD, int x, int y, int *row,
	int *column, int posType);
static void bufModifiedCB(int pos, int nInserted, int nDeleted,
	int nRestyled, char *deletedText, void *cbArg);
static void setScroll(textDisp *textD, int topLineNum, int horizOffset,
	int updateVScrollBar, int updateHScrollBar);
static void hScrollCB(Widget w, XtPointer clientData, XtPointer callData);
static void vScrollCB(Widget w, XtPointer clientData, XtPointer callData);
static void visibilityEH(Widget w, XtPointer data, XEvent *event,
	Boolean *continueDispatch);
static void updateVScrollBarRange(textDisp *textD);
static int updateHScrollBarRange(textDisp *textD);
static int max(int i1, int i2);
static int min(int i1, int i2);
static int countLines(char *string);
static int measureVisLine(textDisp *textD, int visLineNum);
static int emptyLinesVisible(textDisp *textD);
static void blankCursorProtrusions(textDisp *textD);
static void allocateFixedFontGCs(textDisp *textD, XFontStruct *fontStruct,
	Pixel bgPixel, Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
	Pixel highlightFGPixel, Pixel highlightBGPixel);
static GC allocateGC(Widget w, unsigned long valueMask,
	unsigned long foreground, unsigned long background, Font font,
	unsigned long dynamicMask, unsigned long dontCareMask);
static void releaseGC(Widget w, GC gc);
static void resetClipRectangles(textDisp *textD);
static int visLineLength(textDisp *textD, int visLineNum);
static void findWrapRange(textDisp *textD, char *deletedText, int pos,
    	int nInserted, int nDeleted, int *modRangeStart, int *modRangeEnd,
    	int *linesInserted, int *linesDeleted);
static void wrappedLineCounter(textDisp *textD, textBuffer *buf, int startPos,
    	int maxPos, int maxLines, int startPosIsLineStart, int *retPos,
    	int *retLines, int *retLineStart, int *retLineEnd);
static void findLineEnd(textDisp *textD, int startPos, int startPosIsLineStart,
    	int *lineEnd, int *nextLineStart);
static int wrapUsesCharacter(textDisp *textD, int lineEndPos);
static void hideOrShowHScrollBar(textDisp *textD);
static int rangeTouchesRectSel(selection *sel, int rangeStart, int rangeEnd);
static void extendRangeForStyleMods(textBuffer *styleBuf, int *start, int *end);

textDisp *TextDCreate(Widget widget, Widget hScrollBar, Widget vScrollBar,
	Position left, Position top, Position width, Position height,
	textBuffer *buffer, XFontStruct *fontStruct, Pixel bgPixel,
	Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
	Pixel highlightFGPixel, Pixel highlightBGPixel, Pixel cursorFGPixel,
	int continuousWrap, int wrapMargin)
{
    textDisp *textD;
    XGCValues gcValues;
    int i;
    
    textD = (textDisp *)XtMalloc(sizeof(textDisp));
    textD->w = widget;
    textD->top = top;
    textD->left = left;
    textD->width = width;
    textD->height = height;
    textD->cursorOn = True;
    textD->cursorPos = 0;
    textD->cursorX = -100;
    textD->cursorY = -100;
    textD->cursorToHint = NO_HINT;
    textD->cursorStyle = NORMAL_CURSOR;
    textD->cursorPreferredCol = -1;
    textD->buffer = buffer;
    textD->firstChar = 0;
    textD->lastChar = 0;
    textD->nBufferLines = 0;
    textD->topLineNum = 1;
    textD->horizOffset = 0;
    textD->visibility = VisibilityUnobscured;
    textD->hScrollBar = hScrollBar;
    textD->vScrollBar = vScrollBar;
    textD->fontStruct = fontStruct;
    textD->ascent = fontStruct->ascent;
    textD->descent = fontStruct->descent;
    textD->styleBuffer = NULL;
    textD->styleTable = NULL;
    textD->nStyles = 0;
    textD->bgPixel = bgPixel;
    textD->selectBGPixel = selectBGPixel;
    textD->highlightBGPixel = highlightBGPixel;
    textD->wrapMargin = wrapMargin;
    textD->continuousWrap = continuousWrap;
    allocateFixedFontGCs(textD, fontStruct, bgPixel, fgPixel, selectFGPixel,
	    selectBGPixel, highlightFGPixel, highlightBGPixel);
    textD->styleGC = allocateGC(textD->w, 0, 0, 0, fontStruct->fid,
    	    GCClipMask|GCForeground|GCBackground, GCArcMode);
    textD->nVisibleLines =
    	    (height - 1) / (textD->ascent + textD->descent) + 1;
    gcValues.foreground = cursorFGPixel;
    textD->cursorFGGC = XtGetGC(widget, GCForeground, &gcValues);
    textD->lineStarts = (int *)XtMalloc(sizeof(int) * textD->nVisibleLines);
    textD->lineStarts[0] = 0;
    for (i=1; i<textD->nVisibleLines; i++)
    	textD->lineStarts[i] = -1;
    
    /* Attach an event handler to the widget so we can know the visibility
       (used for choosing the fastest drawing method) */
    XtAddEventHandler(widget, VisibilityChangeMask, False,
    	    visibilityEH, textD);
    
    /* Attach the callback to the text buffer for receiving modification
       information */
    if (buffer != NULL)
    	BufAddModifyCB(buffer, bufModifiedCB, textD);
    
    /* Initialize the scroll bars and attach movement callbacks */
    if (vScrollBar != NULL) {
	XtVaSetValues(vScrollBar, XmNminimum, 1, XmNmaximum, 2,
    		XmNsliderSize, 1, XmNrepeatDelay, 10, XmNvalue, 1, 0);
	XtAddCallback(vScrollBar, XmNdragCallback, vScrollCB, (XtPointer)textD);
	XtAddCallback(vScrollBar, XmNvalueChangedCallback, vScrollCB, 
		(XtPointer)textD);
    }
    if (hScrollBar != NULL) {
	XtVaSetValues(hScrollBar, XmNminimum, 0, XmNmaximum, 1,
    		XmNsliderSize, 1, XmNrepeatDelay, 10, XmNvalue, 0,
    		XmNincrement, fontStruct->max_bounds.width, 0);
	XtAddCallback(hScrollBar, XmNdragCallback, hScrollCB, (XtPointer)textD);
	XtAddCallback(hScrollBar, XmNvalueChangedCallback, hScrollCB,
		(XtPointer)textD);
    }

    /* Update the display to reflect the contents of the buffer */
    if (buffer != NULL)
    	bufModifiedCB(0, buffer->length, 0, 0, NULL, textD);

    /* Decide if the horizontal scroll bar needs to be visible */
    hideOrShowHScrollBar(textD);

    return textD;
}

/*
** Free a text display and release its associated memory.  Note, the text
** BUFFER that the text display displays is a separate entity and is not
** freed, nor are the style buffer or style table.
*/
void TextDFree(textDisp *textD)
{
    BufRemoveModifyCB(textD->buffer, bufModifiedCB, textD);
    releaseGC(textD->w, textD->gc);
    releaseGC(textD->w, textD->selectGC);
    releaseGC(textD->w, textD->highlightGC);
    releaseGC(textD->w, textD->selectBGGC);
    releaseGC(textD->w, textD->highlightBGGC);
    releaseGC(textD->w, textD->styleGC);
    XtFree((char *)textD->lineStarts);
    XtFree((char *)textD);
}

/*
** Attach a text buffer to display, replacing the current buffer (if any)
*/
void TextDSetBuffer(textDisp *textD, textBuffer *buffer)
{
    /* If the text display is already displaying a buffer, clear it off
       of the display and remove our callback from it */
    if (textD->buffer != NULL) {
    	bufModifiedCB(0, 0, textD->buffer->length, 0, NULL, textD);
    	BufRemoveModifyCB(textD->buffer, bufModifiedCB, textD);
    }
    
    /* Add the buffer to the display, and attach a callback to the buffer for
       receiving modification information when the buffer contents change */
    textD->buffer = buffer;
    BufAddModifyCB(buffer, bufModifiedCB, textD);
    
    /* Update the display */
    bufModifiedCB(0, buffer->length, 0, 0, NULL, textD);
}

/*
** return the displayed text buffer
*/
textBuffer *TextDGetBuffer(textDisp *textD)
{
    return textD->buffer;
}

/*
** Attach (or remove) highlight information in text display and redisplay.
** Highlighting information consists of a style buffer which parallels the
** normal text buffer, but codes font and color information for the display;
** a style table which translates style buffer codes (indexed by buffer
** character - 'A') into fonts and colors; and a callback mechanism for
** as-needed highlighting, triggered by a style buffer entry of
** "unfinishedStyle".  Style buffer can trigger additional redisplay during
** a normal buffer modification if the buffer contains a primary selection
** (see extendRangeForStyleMods for more information on this protocol).
**
** Style buffers, tables and their associated memory are managed by the caller.
*/
void TextDAttachHighlightData(textDisp *textD, textBuffer *styleBuffer,
    	styleTableEntry *styleTable, int nStyles, char unfinishedStyle,
    	unfinishedStyleCBProc unfinishedHighlightCB, void *cbArg)
{
    textD->styleBuffer = styleBuffer;
    textD->styleTable = styleTable;
    textD->nStyles = nStyles;
    textD->unfinishedStyle = unfinishedStyle;
    textD->unfinishedHighlightCB = unfinishedHighlightCB;
    textD->highlightCBArg = cbArg;
    
    /* Call TextDSetFont to combine font information from style table and
       primary font, adjust font-related parameters, and then redisplay */
    TextDSetFont(textD, textD->fontStruct);
}

/*
** Change the (non highlight) font
*/
void TextDSetFont(textDisp *textD, XFontStruct *fontStruct)
{
    Display *display = XtDisplay(textD->w);
    int i, maxAscent = fontStruct->ascent, maxDescent = fontStruct->descent;
    int width, height;
    Pixel bgPixel, fgPixel, selectFGPixel, selectBGPixel;
    Pixel highlightFGPixel, highlightBGPixel;
    XGCValues values;
    
    /* If font size changes, cursor will be redrawn in a new position */
    blankCursorProtrusions(textD);
    
    /* If there is a (syntax highlighting) style table in use, find the new
       maximum font height for this text display */
    for (i=0; i<textD->nStyles; i++) {
    	if (textD->styleTable[i].font->ascent > maxAscent)
    	    maxAscent = textD->styleTable[i].font->ascent;
    	if (textD->styleTable[i].font->descent > maxDescent)
    	    maxDescent = textD->styleTable[i].font->descent;
    }
    textD->ascent = maxAscent;
    textD->descent = maxDescent;
    
    /* Don't let the height dip below one line, or bad things can happen */
    if (textD->height < maxAscent + maxDescent)
        textD->height = maxAscent + maxDescent;

    /* Change the font.  In most cases, this means re-allocating the
       affected GCs (they are shared with other widgets, and if the primary
       font changes, must be re-allocated to change it). Unfortunately,
       this requres recovering all of the colors from the existing GCs */
    textD->fontStruct = fontStruct;
    XGetGCValues(display, textD->gc, GCForeground|GCBackground, &values);
    fgPixel = values.foreground;
    bgPixel = values.background;
    XGetGCValues(display, textD->selectGC, GCForeground|GCBackground, &values);
    selectFGPixel = values.foreground;
    selectBGPixel = values.background;
    XGetGCValues(display, textD->highlightGC,GCForeground|GCBackground,&values);
    highlightFGPixel = values.foreground;
    highlightBGPixel = values.background;
    releaseGC(textD->w, textD->gc);
    releaseGC(textD->w, textD->selectGC);
    releaseGC(textD->w, textD->highlightGC);
    releaseGC(textD->w, textD->selectBGGC);
    releaseGC(textD->w, textD->highlightBGGC);
    allocateFixedFontGCs(textD, fontStruct, bgPixel, fgPixel, selectFGPixel,
	    selectBGPixel, highlightFGPixel, highlightBGPixel);
    XSetFont(display, textD->styleGC, fontStruct->fid);
    
    /* Do a full resize to force recalculation of font related parameters */
    width = textD->width;
    height = textD->height;
    textD->width = textD->height = 0;
    TextDResize(textD, width, height);
    
    /* Redisplay */
    TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    	    textD->height);
}

/*
** Change the size of the displayed text area
*/
void TextDResize(textDisp *textD, int width, int height)
{
    int oldVisibleLines = textD->nVisibleLines;
    int canRedraw = XtWindow(textD->w) != 0;
    int newVisibleLines = height / (textD->ascent + textD->descent);
    int redrawAll = False;
    int oldWidth = textD->width;
    int exactHeight = height - height % (textD->ascent + textD->descent);
    
    textD->width = width;
    textD->height = height;
    
    /* In continuous wrap mode, a change in width affects the total number of
       lines in the buffer, and can leave the top line number incorrect, and
       the top character no longer pointing at a valid line start */
    if (textD->continuousWrap && textD->wrapMargin==0 && width!=oldWidth) {
    	textD->nBufferLines = TextDCountLines(textD, 0, textD->buffer->length,
    	    	True);
	textD->firstChar = TextDStartOfLine(textD, textD->firstChar);
	textD->topLineNum = TextDCountLines(textD, 0, textD->firstChar, True)+1;
    	redrawAll = True;
    }
   
    /* reallocate and update the line starts array, which may have changed
       size and/or contents. (contents can change in continuous wrap mode
       when the width changes, even without a change in height) */
    if (oldVisibleLines < newVisibleLines) {
	XtFree((char *)textD->lineStarts);
	textD->lineStarts = (int *)XtMalloc(sizeof(int) * newVisibleLines);
    }
    textD->nVisibleLines = newVisibleLines;
    calcLineStarts(textD, 0, newVisibleLines);
    calcLastChar(textD);
    
    /* if the window became shorter, there may be partially drawn
       text left at the bottom edge, which must be cleaned up */
    if (canRedraw && oldVisibleLines>newVisibleLines && exactHeight!=height)
    	XClearArea(XtDisplay(textD->w), XtWindow(textD->w), textD->left,
    		textD->top + exactHeight,  textD->width,
    		height - exactHeight, False);
    
    /* if the window became taller, there may be an opportunity to display
       more text by scrolling down */
    if (canRedraw && oldVisibleLines < newVisibleLines && textD->topLineNum +
    	    textD->nVisibleLines > textD->nBufferLines)
    	setScroll(textD, max(1, textD->nBufferLines - textD->nVisibleLines + 2),
    		textD->horizOffset, False, False);
    
    /* Update the scroll bar page increment size (as well as other scroll
       bar parameters.  If updating the horizontal range caused scrolling,
       redraw */
    updateVScrollBarRange(textD);
    if (updateHScrollBarRange(textD))
    	redrawAll = True;

    /* If a full redraw is needed */
    if (redrawAll && canRedraw)
    	TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    		textD->height);

    /* Decide if the horizontal scroll bar needs to be visible */
    hideOrShowHScrollBar(textD);
}

/*
** Refresh a rectangle of the text display.  left and top are in coordinates of
** the text drawing window
*/
void TextDRedisplayRect(textDisp *textD, int left, int top, int width,
	int height)
{
    int fontHeight, firstLine, lastLine, line;
    
    /* find the line number range of the display */
    fontHeight = textD->ascent + textD->descent;
    firstLine = (top - textD->top - fontHeight + 1) / fontHeight;
    lastLine = (top + height - textD->top) / fontHeight;
    
    /* If the graphics contexts are shared using XtAllocateGC, their
       clipping rectangles may have changed since the last use */
    resetClipRectangles(textD);
    
    /* draw the lines */
    for (line=firstLine; line<=lastLine; line++)
    	redisplayLine(textD, line, left, left+width, 0, INT_MAX);
}

/*
** Refresh all of the text between buffer positions "start" and "end"
** not including the character at the position "end".
** If end points beyond the end of the buffer, refresh the whole display
** after pos, including blank lines which are not technically part of
** any range of characters.
*/
void TextDRedisplayRange(textDisp *textD, int start, int end)
{
    int i, startLine, lastLine, startIndex, endIndex;
    
    /* If the range is outside of the displayed text, just return */
    if (end < textD->firstChar || (start > textD->lastChar &&
    	    !emptyLinesVisible(textD)))
        return;
       
    /* Clean up the starting and ending values */
    if (start < 0) start = 0;
    if (start > textD->buffer->length) start = textD->buffer->length;
    if (end < 0) end = 0;
    if (end > textD->buffer->length) end = textD->buffer->length;
    
    /* Get the starting and ending lines */
    if (start < textD->firstChar)
    	start = textD->firstChar;
    if (!posToVisibleLineNum(textD, start, &startLine))
    	startLine = textD->nVisibleLines - 1;
    if (end >= textD->lastChar) {
    	lastLine = textD->nVisibleLines - 1;
    } else {
    	if (!posToVisibleLineNum(textD, end, &lastLine)) {
    	    /* shouldn't happen */
    	    lastLine = textD->nVisibleLines - 1;
    	}
    }
    	
    /* Get the starting and ending positions within the lines */
    startIndex = textD->lineStarts[startLine] == -1 ? 0 :
    	    start - textD->lineStarts[startLine];
    if (end >= textD->lastChar)
    	endIndex = INT_MAX;
    else if (textD->lineStarts[lastLine] == -1)
    	endIndex = 0;
    else
    	endIndex = end - textD->lineStarts[lastLine];
    
    /* Reset the clipping rectangles for the drawing GCs which are shared
       using XtAllocateGC, and may have changed since the last use */
    resetClipRectangles(textD);
    
    /* If the starting and ending lines are the same, redisplay the single
       line between "start" and "end" */
    if (startLine == lastLine) {
    	redisplayLine(textD, startLine, 0, INT_MAX, startIndex, endIndex);
    	return;
    }
    
    /* Redisplay the first line from "start" */
    redisplayLine(textD, startLine, 0, INT_MAX, startIndex, INT_MAX);
    
    /* Redisplay the lines in between at their full width */
    for (i=startLine+1; i<lastLine; i++)
	redisplayLine(textD, i, 0, INT_MAX, 0, INT_MAX);

    /* Redisplay the last line to "end" */
    redisplayLine(textD, lastLine, 0, INT_MAX, 0, endIndex);
}

/*
** Set the scroll position of the text display vertically by line number and
** horizontally by pixel offset from the left margin
*/
void TextDSetScroll(textDisp *textD, int topLineNum, int horizOffset)
{
    int sliderSize, sliderMax;
    
    /* Limit the requested scroll position to allowable values */
    if (topLineNum < 1)
    	topLineNum = 1;
    else if (topLineNum > textD->topLineNum &&
    	     topLineNum > textD->nBufferLines + 2 - textD->nVisibleLines)
    	topLineNum = max(textD->topLineNum,
    	    	textD->nBufferLines + 2 - textD->nVisibleLines);
    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax, 
    	    XmNsliderSize, &sliderSize, 0);
    if (horizOffset < 0)
    	horizOffset = 0;
    if (horizOffset > sliderMax - sliderSize)
    	horizOffset = sliderMax - sliderSize;

    setScroll(textD, topLineNum, horizOffset, True, True);
}

/*
** Get the current scroll position for the text display, in terms of line
** number of the top line and horizontal pixel offset from the left margin
*/
void TextDGetScroll(textDisp *textD, int *topLineNum, int *horizOffset)
{
    *topLineNum = textD->topLineNum;
    *horizOffset = textD->horizOffset;
}

/*
** Set the position of the text insertion cursor for text display "textD"
*/
void TextDSetInsertPosition(textDisp *textD, int newPos)
{
    /* make sure new position is ok, do nothing if it hasn't changed */
    if (newPos == textD->cursorPos)
    	return;
    if (newPos < 0) newPos = 0;
    if (newPos > textD->buffer->length) newPos = textD->buffer->length;
 
    /* cursor movement cancels vertical cursor motion column */
    textD->cursorPreferredCol = -1;
   
    /* erase the cursor at it's previous position */
    TextDBlankCursor(textD);
    
    /* draw it at its new position */
    textD->cursorPos = newPos;
    textD->cursorOn = True;
    TextDRedisplayRange(textD, textD->cursorPos, textD->cursorPos + 1);
}

void TextDBlankCursor(textDisp *textD)
{
    if (!textD->cursorOn)
    	return;
    
    blankCursorProtrusions(textD);
    textD->cursorOn = False;
    TextDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
}

void TextDUnblankCursor(textDisp *textD)
{
    if (!textD->cursorOn) {
    	textD->cursorOn = True;
    	TextDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
    }
}

void TextDSetCursorStyle(textDisp *textD, int style)
{
    textD->cursorStyle = style;
    blankCursorProtrusions(textD);
    if (textD->cursorOn)
    	TextDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos + 1);
}

void TextDSetWrapMode(textDisp *textD, int wrap, int wrapMargin)
{
    textD->wrapMargin = wrapMargin;
    textD->continuousWrap = wrap;
    
    /* wrapping can change change the total number of lines, re-count */
    textD->nBufferLines = TextDCountLines(textD, 0, textD->buffer->length,True);
    
    /* changing wrap margins wrap or changing from wrapped mode to non-wrapped
       can leave the character at the top no longer at a line start, and/or
       change the line number */
    textD->firstChar = TextDStartOfLine(textD, textD->firstChar);
    textD->topLineNum = TextDCountLines(textD, 0, textD->firstChar, True) + 1;
    
    /* update the line starts array */
    calcLineStarts(textD, 0, textD->nVisibleLines);
    calcLastChar(textD);
    
    /* Update the scroll bar page increment size (as well as other scroll
       bar parameters) */
    updateVScrollBarRange(textD);
    updateHScrollBarRange(textD);
    
    /* Decide if the horizontal scroll bar needs to be visible */
    hideOrShowHScrollBar(textD);

    /* Do a full redraw */
    TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    	    textD->height);
}

int TextDGetInsertPosition(textDisp *textD)
{
    return textD->cursorPos;
}

/*
** Insert "text" at the current cursor location.  This has the same
** effect as inserting the text into the buffer using BufInsert and
** then moving the insert position after the newly inserted text, except
** that it's optimized to do less redrawing.
*/
void TextDInsert(textDisp *textD, char *text)
{
    int pos = textD->cursorPos;
    
    textD->cursorToHint = pos + strlen(text);
    BufInsert(textD->buffer, pos, text);
    textD->cursorToHint = NO_HINT;
}

/*
** Insert "text" (which must not contain newlines), overstriking the current
** cursor location.
*/
void TextDOverstrike(textDisp *textD, char *text)
{
    int startPos = textD->cursorPos;
    textBuffer *buf = textD->buffer;
    int lineStart = BufStartOfLine(buf, startPos);
    int textLen = strlen(text);
    int i, p, endPos, indent, startIndent, endIndent;
    char *c, ch, *paddedText = NULL;
    
    /* determine how many displayed character positions are covered */
    startIndent = BufCountDispChars(textD->buffer, lineStart, startPos);
    indent = startIndent;
    for (c=text; *c!='\0'; c++)
    	indent += BufCharWidth(*c, indent, buf->tabDist, buf->nullSubsChar);
    endIndent = indent;
    
    /* find which characters to remove, and if necessary generate additional
       padding to make up for removed control characters at the end */
    indent=startIndent;
    for (p=startPos; ; p++) {
    	if (p == buf->length)
    	    break;
    	ch = BufGetCharacter(buf, p);
    	if (ch == '\n')
    	    break;
    	indent += BufCharWidth(ch, indent, buf->tabDist, buf->nullSubsChar);
    	if (indent == endIndent) {
    	    p++;
    	    break;
    	} else if (indent > endIndent) {
    	    if (ch != '\t') {
    	    	p++;
    	    	paddedText = XtMalloc(textLen + MAX_EXP_CHAR_LEN + 1);
    	    	strcpy(paddedText, text);
    	    	for (i=0; i<indent-endIndent; i++)
    	    	    paddedText[textLen+i] = ' ';
    	    	paddedText[textLen+i] = '\0';
    	    }
    	    break;
    	}
    }
    endPos = p;	    
    
    textD->cursorToHint = startPos + textLen;
    BufReplace(buf, startPos, endPos, paddedText == NULL ? text : paddedText);
    textD->cursorToHint = NO_HINT;
    if (paddedText != NULL)
    	XtFree(paddedText);
}

/*
** Translate window coordinates to the nearest (insert cursor) text position.
*/
int TextDXYToPosition(textDisp *textD, int x, int y)
{
    return xyToPos(textD, x, y, CURSOR_POS);
}

/*
** Translate window coordinates to the nearest row and column number for
** positioning the cursor.  This, of course, makes no sense when the font
** is proportional, since there are no absolute columns.
*/
void TextDXYToUnconstrainedPosition(textDisp *textD, int x, int y, int *row,
	int *column)
{
    xyToUnconstrainedPos(textD, x, y, row, column, CURSOR_POS);
}

/*
** Translate a buffer text position to the XY location where the center
** of the cursor would be positioned to point to that character.  Returns
** False if the position is not displayed because it is VERTICALLY out
** of view.  If the position is horizontally out of view, returns the
** x coordinate where the position would be if it were visible.
*/
int TextDPositionToXY(textDisp *textD, int pos, int *x, int *y)
{
    int charIndex, lineStartPos, fontHeight, lineLen;
    int visLineNum, charLen, outIndex, xStep, charStyle;
    char *lineStr, expandedChar[MAX_EXP_CHAR_LEN];
    
    /* If position is not displayed, return false */
    if (pos < textD->firstChar ||
    	    (pos > textD->lastChar && !emptyLinesVisible(textD)))
    	return False;
    	
    /* Calculate y coordinate */
    if (!posToVisibleLineNum(textD, pos, &visLineNum))
    	return False;
    fontHeight = textD->ascent + textD->descent;
    *y = textD->top + visLineNum*fontHeight + fontHeight/2;
    
    /* Get the text, length, and  buffer position of the line. If the position
       is beyond the end of the buffer and should be at the first position on
       the first empty line, don't try to get or scan the text  */
    lineStartPos = textD->lineStarts[visLineNum];
    if (lineStartPos == -1) {
    	*x = textD->left - textD->horizOffset;
    	return True;
    }
    lineLen = visLineLength(textD, visLineNum);
    lineStr = BufGetRange(textD->buffer, lineStartPos, lineStartPos + lineLen);
    
    /* Step through character positions from the beginning of the line
       to "pos" to calculate the x coordinate */
    xStep = textD->left - textD->horizOffset;
    outIndex = 0;
    for(charIndex=0; charIndex<pos-lineStartPos; charIndex++) {
    	charLen = BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
    		textD->buffer->tabDist, textD->buffer->nullSubsChar);
   	charStyle = styleOfPos(textD, lineStartPos, lineLen, charIndex,
   	    	outIndex);
    	xStep += stringWidth(textD, expandedChar, charLen, charStyle);
    	outIndex += charLen;
    }
    *x = xStep;
    XtFree(lineStr);
    return True;
}

/*
** Find the line number of position "pos".  Note: this only works for
** displayed lines.  If the line is not displayed, the function returns
** False (without the lineStarts array it could turn in to very long
** calculation involving scanning large amounts of text in the buffer).
*/
int TextDPosToLineAndCol(textDisp *textD, int pos, int *lineNum, int *column)
{
    int retVal;
    
    retVal = posToVisibleLineNum(textD, pos, lineNum);
    if (retVal) {
    	*column = BufCountDispChars(textD->buffer,
    	    	textD->lineStarts[*lineNum], pos);
    	*lineNum += textD->topLineNum;
    }
    return retVal;
}

/*
** Return True if position (x, y) is inside of the primary selection
*/
int TextDInSelection(textDisp *textD, int x, int y)
{
    int row, column, pos = xyToPos(textD, x, y, CHARACTER_POS);
    textBuffer *buf = textD->buffer;
    
    xyToUnconstrainedPos(textD, x, y, &row, &column, CHARACTER_POS);
    if (rangeTouchesRectSel(&buf->primary, textD->firstChar, textD->lastChar))
    	column = TextDOffsetWrappedColumn(textD, row, column);
    return inSelection(&buf->primary, pos, BufStartOfLine(buf, pos), column);
}

/*
** Correct a column number based on an unconstrained position (as returned by
** TextDXYToUnconstrainedPosition) to be relative to the last actual newline
** in the buffer before the row and column position given, rather than the
** last line start created by line wrapping.  This is an adapter
** for rectangular selections and code written before continuous wrap mode,
** which thinks that the unconstrained column is the number of characters
** from the last newline.  Obviously this is time consuming, because it
** invloves character re-counting.
*/
int TextDOffsetWrappedColumn(textDisp *textD, int row, int column)
{
    int lineStart, dispLineStart;
    
    if (!textD->continuousWrap || row < 0 || row > textD->nVisibleLines)
    	return column;
    dispLineStart = textD->lineStarts[row];
    if (dispLineStart == -1)
    	return column;
    lineStart = BufStartOfLine(textD->buffer, dispLineStart);
    return column + BufCountDispChars(textD->buffer, lineStart, dispLineStart);
}

/*
** Correct a row number from an unconstrained position (as returned by
** TextDXYToUnconstrainedPosition) to a straight number of newlines from the
** top line of the display.  Because rectangular selections are based on
** newlines, rather than display wrapping, and anywhere a rectangular selection
** needs a row, it needs it in terms of un-wrapped lines.
*/
int TextDOffsetWrappedRow(textDisp *textD, int row)
{
    if (!textD->continuousWrap || row < 0 || row > textD->nVisibleLines)
    	return row;
    return BufCountLines(textD->buffer, textD->firstChar, 
    	    textD->lineStarts[row]);
}

/*
** Scroll the display to bring insertion cursor into view.
**
** Note: it would be nice to be able to do this without counting lines twice
** (setScroll counts them too) and/or to count from the most efficient
** starting point, but the efficiency of this routine is not as important to
** the overall performance of the text display.
*/
void TextDMakeInsertPosVisible(textDisp *textD)
{
    int hOffset, topLine, x, y;
    int cursorPos = textD->cursorPos;
    
    hOffset = textD->horizOffset;
    topLine = textD->topLineNum;
    
    /* Find the new top line number */
    if (cursorPos < textD->firstChar)
    	topLine -= TextDCountLines(textD, cursorPos, textD->firstChar, False);
    else if (cursorPos > textD->lastChar && !emptyLinesVisible(textD)) {
    	topLine += TextDCountLines(textD, textD->lastChar -
    	    	(wrapUsesCharacter(textD, textD->lastChar) ? 0 : 1),
    	    	cursorPos, False);
    } else if (cursorPos == textD->lastChar && !emptyLinesVisible(textD) &&
    	    !wrapUsesCharacter(textD, textD->lastChar))
    	topLine++;
    if (topLine < 1) {
    	fprintf(stderr, "internal consistency check tl1 failed\n");
    	topLine = 1;
    }
    
    /* Find the new setting for horizontal offset (this is a bit ungraceful).
       If the line is visible, just use TextDPositionToXY to get the position
       to scroll to, otherwise, do the vertical scrolling first, then the
       horizontal */
    if (!TextDPositionToXY(textD, cursorPos, &x, &y)) {
    	setScroll(textD, topLine, hOffset, True, True);
    	if (!TextDPositionToXY(textD, cursorPos, &x, &y))
    	    return; /* Give up, it's not worth it (but why does it fail?) */
    }
    if (x > textD->left + textD->width)
    	hOffset += x - (textD->left + textD->width);
    else if (x < textD->left)
    	hOffset += x - textD->left;
    
    /* Do the scroll */
    setScroll(textD, topLine, hOffset, True, True);
}

/*
** Cursor movement functions
*/
int TextDMoveRight(textDisp *textD)
{
    if (textD->cursorPos >= textD->buffer->length)
    	return False;
    TextDSetInsertPosition(textD, textD->cursorPos + 1);
    return True;
}

int TextDMoveLeft(textDisp *textD)
{
    if (textD->cursorPos <= 0)
    	return False;
    TextDSetInsertPosition(textD, textD->cursorPos - 1); 
    return True;
}

int TextDMoveUp(textDisp *textD)
{
    int lineStartPos, column, prevLineStartPos, newPos, visLineNum;
    
    /* Find the position of the start of the line.  Use the line starts array
       if possible, to avoid unbounded line-counting in continuous wrap mode */
    if (posToVisibleLineNum(textD, textD->cursorPos, &visLineNum))
    	lineStartPos = textD->lineStarts[visLineNum];
    else {
    	lineStartPos = TextDStartOfLine(textD, textD->cursorPos);
    	visLineNum = -1;
    }
    if (lineStartPos == 0)
    	return False;
    
    /* Decide what column to move to, if there's a preferred column use that */
    column = textD->cursorPreferredCol >= 0 ? textD->cursorPreferredCol :
    	    BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
    
    /* count forward from the start of the previous line to reach the column */
    if (visLineNum != -1 && visLineNum != 0)
    	prevLineStartPos = textD->lineStarts[visLineNum-1];
    else
    	prevLineStartPos = TextDCountBackwardNLines(textD, lineStartPos, 1);
    newPos = BufCountForwardDispChars(textD->buffer, prevLineStartPos, column);
    if (textD->continuousWrap)
    	newPos = min(newPos, TextDEndOfLine(textD, prevLineStartPos, True));
    
    /* move the cursor */
    TextDSetInsertPosition(textD, newPos);
    
    /* if a preferred column wasn't aleady established, establish it */
    textD->cursorPreferredCol = column;
    return True;
}
int TextDMoveDown(textDisp *textD)
{
    int lineStartPos, column, nextLineStartPos, newPos, visLineNum;
    
    if (textD->cursorPos == textD->buffer->length)
    	return False;
    if (posToVisibleLineNum(textD, textD->cursorPos, &visLineNum))
    	lineStartPos = textD->lineStarts[visLineNum];
    else {
    	lineStartPos = TextDStartOfLine(textD, textD->cursorPos);
    	visLineNum = -1;
    }
    column = textD->cursorPreferredCol >= 0 ? textD->cursorPreferredCol :
    	    BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
    nextLineStartPos = TextDCountForwardNLines(textD, lineStartPos, 1, True);
    newPos = BufCountForwardDispChars(textD->buffer, nextLineStartPos, column);
    if (textD->continuousWrap)
    	newPos = min(newPos, TextDEndOfLine(textD, nextLineStartPos, True));
    TextDSetInsertPosition(textD, newPos);
    textD->cursorPreferredCol = column;
    return True;
}

/*
** Same as BufCountLines, but takes in to account wrapping if wrapping is
** turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int TextDCountLines(textDisp *textD, int startPos, int endPos,
    	int startPosIsLineStart)
{
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping use simple (and more efficient) BufCountLines */
    if (!textD->continuousWrap)
    	return BufCountLines(textD->buffer, startPos, endPos);
    
    wrappedLineCounter(textD, textD->buffer, startPos, endPos, INT_MAX,
    	    startPosIsLineStart, &retPos, &retLines, &retLineStart,&retLineEnd);
    return retLines;
}

/*
** Same as BufCountForwardNLines, but takes in to account line breaks when
** wrapping is turned on. If the caller knows that startPos is at a line start,
** it can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int TextDCountForwardNLines(textDisp *textD, int startPos, int nLines,
    	int startPosIsLineStart)
{
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* if we're not wrapping use more efficient BufCountForwardNLines */
    if (!textD->continuousWrap)
    	return BufCountForwardNLines(textD->buffer, startPos, nLines);
    
    /* wrappedLineCounter can't handle the 0 lines case */
    if (nLines == 0)
    	return startPos;
    
    /* use the common line counting routine to count forward */
    wrappedLineCounter(textD, textD->buffer, startPos, textD->buffer->length,
    	    nLines, startPosIsLineStart, &retPos, &retLines, &retLineStart,
    	    &retLineEnd);
    return retPos;
}

/*
** Same as BufEndOfLine, but takes in to account line breaks when wrapping
** is turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
**
** Note that the definition of the end of a line is less clear when continuous
** wrap is on.  With continuous wrap off, it's just a pointer to the newline
** that ends the line.  When it's on, it's the character beyond the last
** DISPLAYABLE character on the line, where a whitespace character which has
** been "converted" to a newline for wrapping is not considered displayable.
** Also note that, a line can be wrapped at a non-whitespace character if the
** line had no whitespace.  In this case, this routine returns a pointer to
** the start of the next line.  This is also consistent with the model used by
** visLineLength.
*/
int TextDEndOfLine(textDisp *textD, int pos, int startPosIsLineStart)
{
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping use more efficien BufEndOfLine */
    if (!textD->continuousWrap)
    	return BufEndOfLine(textD->buffer, pos);
    
    if (pos == textD->buffer->length)
    	return pos;
    wrappedLineCounter(textD, textD->buffer, pos, textD->buffer->length, 1,
    	    startPosIsLineStart, &retPos, &retLines, &retLineStart,&retLineEnd);
    return retLineEnd;
}

/*
** Same as BufStartOfLine, but returns the character after last wrap point
** rather than the last newline.
*/
int TextDStartOfLine(textDisp *textD, int pos)
{
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping, use the more efficient BufStartOfLine */
    if (!textD->continuousWrap)
    	return BufStartOfLine(textD->buffer, pos);

    wrappedLineCounter(textD, textD->buffer, BufStartOfLine(textD->buffer, pos),
    	    pos, INT_MAX, True, &retPos, &retLines, &retLineStart, &retLineEnd);
    return retLineStart;
}

/*
** Same as BufCountBackwardNLines, but takes in to account line breaks when
** wrapping is turned on.
*/
int TextDCountBackwardNLines(textDisp *textD, int startPos, int nLines)
{
    textBuffer *buf = textD->buffer;
    int pos, lineStart, retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping, use the more efficient BufCountBackwardNLines */
    if (!textD->continuousWrap)
    	return BufCountBackwardNLines(textD->buffer, startPos, nLines);

    pos = startPos;
    while (True) {
	lineStart = BufStartOfLine(buf, pos);
	wrappedLineCounter(textD, textD->buffer, lineStart, pos, INT_MAX,
	    	True, &retPos, &retLines, &retLineStart, &retLineEnd);
	if (retLines > nLines)
    	    return TextDCountForwardNLines(textD, lineStart, retLines-nLines,
    	    	    True);
    	nLines -= retLines;
    	pos = lineStart - 1;
    	if (pos <= 0)
    	    return 0;
    	nLines -= 1;
    }
}

/*
** Callback attached to the text buffer to receive modification information
*/
static void bufModifiedCB(int pos, int nInserted, int nDeleted,
	int nRestyled, char *deletedText, void *cbArg)
{
    int linesInserted, linesDeleted, startDispPos, endDispPos;
    textDisp *textD = (textDisp *)cbArg;
    textBuffer *buf = textD->buffer;
    int scrolled, origCursorPos = textD->cursorPos;
    int wrapModStart, wrapModEnd;
 
    /* buffer modification cancels vertical cursor motion column */
    if (nInserted != 0 || nDeleted != 0)
    	textD->cursorPreferredCol = -1;
    
    /* Count the number of lines inserted and deleted, and in the case
       of continuous wrap mode, how much has changed */
    if (textD->continuousWrap) {
    	findWrapRange(textD, deletedText, pos, nInserted, nDeleted,
    	    	&wrapModStart, &wrapModEnd, &linesInserted, &linesDeleted);
    } else {
	linesInserted = nInserted == 0 ? 0 :
    		BufCountLines(buf, pos, pos + nInserted);
	linesDeleted = nDeleted == 0 ? 0 : countLines(deletedText);
    }

    /* Update the line starts and topLineNum */
    if (nInserted != 0 || nDeleted != 0) {
	if (textD->continuousWrap) {
	    updateLineStarts(textD, wrapModStart, wrapModEnd-wrapModStart,
	    	    nDeleted + pos-wrapModStart + (wrapModEnd-(pos+nInserted)),
	    	    linesInserted, linesDeleted, &scrolled);
	} else {
	    updateLineStarts(textD, pos, nInserted, nDeleted, linesInserted,
    		    linesDeleted, &scrolled);
	}
    } else
    	scrolled = False;
    
    /* Update the line count for the whole buffer */
    textD->nBufferLines += linesInserted - linesDeleted;
    
    /* Update the scroll bar ranges (and value if the value changed).  Note
       that updating the horizontal scroll bar range requires scanning the
       entire displayed text, however, it doesn't seem to hurt performance
       much.  Note also, that the horizontal scroll bar update routine is
       allowed to re-adjust horizOffset if there is blank space to the right
       of all lines of text. */
    updateVScrollBarRange(textD);
    scrolled |= updateHScrollBarRange(textD);
    
    /* Update the cursor position */
    if (textD->cursorToHint != NO_HINT) {
    	textD->cursorPos = textD->cursorToHint;
    	textD->cursorToHint = NO_HINT;
    } else if (textD->cursorPos > pos) {
    	if (textD->cursorPos < pos + nDeleted)
    	    textD->cursorPos = pos;
    	else
    	    textD->cursorPos += nInserted - nDeleted;
    }

    /* If the changes caused scrolling, re-paint everything and we're done. */
    if (scrolled) {
    	blankCursorProtrusions(textD);
    	TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    		textD->height);
    	if (textD->styleBuffer) /* See comments in extendRangeForStyleMods */
    	    textD->styleBuffer->primary.selected = False;
    	return;
    }
    
    /* If the changes didn't cause scrolling, decide the range of characters
       that need to be re-painted.  Also if the cursor position moved, be
       sure that the redisplay range covers the old cursor position so the
       old cursor gets erased, and erase the bits of the cursor which extend
       beyond the left and right edges of the text. */
    startDispPos = textD->continuousWrap ? wrapModStart : pos;
    if (origCursorPos == startDispPos && textD->cursorPos != startDispPos)
    	startDispPos = min(startDispPos, origCursorPos-1);
    if (linesInserted == linesDeleted) {
        if (nInserted == 0 && nDeleted == 0)
            endDispPos = pos + nRestyled;
        else {
    	    endDispPos = textD->continuousWrap ? wrapModEnd :
    	    	    BufEndOfLine(buf, pos + nInserted) + 1;
    	    if (origCursorPos >= startDispPos &&
    	    	    (origCursorPos <= endDispPos || endDispPos == buf->length))
    	    	blankCursorProtrusions(textD);
    	}
    } else {
    	endDispPos = textD->lastChar + 1;
    	if (origCursorPos >= pos)
    	    blankCursorProtrusions(textD);
    }
    
    /* If there is a style buffer, check if the modification caused additional
       changes that need to be redisplayed.  (Redisplaying separately would
       cause double-redraw on almost every modification involving styled
       text).  Extend the redraw range to incorporate style changes */
    if (textD->styleBuffer)
    	extendRangeForStyleMods(textD->styleBuffer, &startDispPos, &endDispPos);
    
    /* Redisplay computed range */
    TextDRedisplayRange(textD, startDispPos, endDispPos);
}

/*
** Find the line number of position "pos" relative to the first line of
** displayed text. Returns False if the line is not displayed.
*/
static int posToVisibleLineNum(textDisp *textD, int pos, int *lineNum)
{
    int i;
    
    if (pos < textD->firstChar)
    	return False;
    if (pos > textD->lastChar) {
    	if (emptyLinesVisible(textD)) {
    	    if (textD->lastChar < textD->buffer->length) {
    		if (!posToVisibleLineNum(textD, textD->lastChar, lineNum)) {
    		    fprintf(stderr, "Consistency check ptvl failed\n");
    		    return False;
    		}
    		return ++(*lineNum) <= textD->nVisibleLines-1;
            } else {
            	posToVisibleLineNum(textD, textD->lastChar-1, lineNum);
            	return True;
            }
	}
	return False;
    }
    	
    for (i=textD->nVisibleLines-1; i>=0; i--) {
    	if (textD->lineStarts[i] != -1 && pos >= textD->lineStarts[i]) {
    	    *lineNum = i;
    	    return True;
    	}
    }
    return False; /* probably never be reached */
}

/*
** Redisplay the text on a single line represented by "visLineNum" (the
** number of lines down from the top of the display), limited by
** "leftClip" and "rightClip" window coordinates and "leftCharIndex" and
** "rightCharIndex" character positions (not including the character at
** position "rightCharIndex").
**
** The cursor is also drawn if it appears on the line.
*/
static void redisplayLine(textDisp *textD, int visLineNum, int leftClip,
	int rightClip, int leftCharIndex, int rightCharIndex)
{
    textBuffer *buf = textD->buffer;
    int i, x, y, startX, charIndex, lineStartPos, lineLen, fontHeight;
    int stdCharWidth, charWidth, startIndex, charStyle, style;
    int charLen, outStartIndex, outIndex, cursorX, hasCursor = False;
    int dispIndexOffset, cursorPos = textD->cursorPos;
    char expandedChar[MAX_EXP_CHAR_LEN], outStr[MAX_DISP_LINE_LEN];
    char *lineStr, *outPtr;
    
    /* If line is not displayed, skip it */
    if (visLineNum < 0 || visLineNum >= textD->nVisibleLines)
    	return;

    /* Calculate y coordinate of the string to draw */
    fontHeight = textD->ascent + textD->descent;
    y = textD->top + visLineNum * fontHeight;

    /* Get the text, length, and  buffer position of the line to display */
    lineStartPos = textD->lineStarts[visLineNum];
    if (lineStartPos == -1) {
    	lineLen = 0;
    	lineStr = NULL;
    } else {
	lineLen = visLineLength(textD, visLineNum);
	lineStr = BufGetRange(buf, lineStartPos, lineStartPos + lineLen);
    }
    
    /* Space beyond the end of the line is still counted in units of characters
       of a standardized character width (this is done mostly because style
       changes based on character position can still occur in this region due
       to rectangular selections).  stdCharWidth must be non-zero to prevent a
       potential infinite loop if x does not advance */
    stdCharWidth = textD->fontStruct->max_bounds.width;
    if (stdCharWidth <= 0) {
    	fprintf(stderr, "Internal Error, bad font measurement\n");
    	XtFree(lineStr);
    	return;
    }
    
    /* Shrink the clipping range to the active display area */
    leftClip = max(textD->left, leftClip);
    rightClip = min(rightClip, textD->left + textD->width);
    
    /* Rectangular selections are based on "real" line starts (after a newline
       or start of buffer).  Calculate the difference between the last newline
       position and the line start we're using.  Since scanning back to find a
       newline is expensive, only do so if there's actually a rectangular
       selection which needs it */
    if (textD->continuousWrap && (rangeTouchesRectSel(&buf->primary,
    	    lineStartPos, lineStartPos + lineLen) || rangeTouchesRectSel(
    	    &buf->secondary, lineStartPos, lineStartPos + lineLen) ||
    	    rangeTouchesRectSel(&buf->highlight, lineStartPos,
    	    lineStartPos + lineLen))) {
    	dispIndexOffset = BufCountDispChars(buf,
    	    	BufStartOfLine(buf, lineStartPos), lineStartPos);
    } else
    	dispIndexOffset = 0;

    /* Step through character positions from the beginning of the line (even if
       that's off the left edge of the displayed area) to find the first
       character position that's not clipped, and the x coordinate for drawing
       that character */
    x = textD->left - textD->horizOffset;
    outIndex = 0;
    for(charIndex=0; ; charIndex++) {
    	charLen = charIndex >= lineLen ? 1 :
    	    	BufExpandCharacter(lineStr[charIndex], outIndex,
    	    	expandedChar, buf->tabDist, buf->nullSubsChar);
    	style = styleOfPos(textD, lineStartPos, lineLen, charIndex,
    	    	outIndex + dispIndexOffset);
    	charWidth = charIndex >= lineLen ? stdCharWidth :
    	    	stringWidth(textD, expandedChar, charLen, style);
    	if (x + charWidth >= leftClip && charIndex >= leftCharIndex) {
    	    startIndex = charIndex;
    	    outStartIndex = outIndex;
    	    startX = x;
    	    break;
    	}
    	x += charWidth;
    	outIndex += charLen;
    }

    /* Scan character positions from the beginning of the clipping range, and
       draw parts whenever the style changes (also note if the cursor is on
       this line, and where it should be drawn to take advantage of the x
       position which we've gone to so much trouble to calculate) */
    outPtr = outStr;
    outIndex = outStartIndex;
    x = startX;
    for(charIndex=startIndex; charIndex<rightCharIndex; charIndex++) {
    	if (lineStartPos+charIndex == cursorPos) {
    	    if (charIndex < lineLen || (charIndex == lineLen &&
    	    	    cursorPos >= buf->length)) {
    		hasCursor = True;
    		cursorX = x - 1;
    	    } else if (charIndex == lineLen) {
    	    	if (wrapUsesCharacter(textD, cursorPos)) {
    	    	    hasCursor = True;
    	    	    cursorX = x - 1;
    	    	}
    	    }
    	}
     	charLen = charIndex >= lineLen ? 1 :
     		BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
    		buf->tabDist, buf->nullSubsChar);
   	charStyle = styleOfPos(textD, lineStartPos, lineLen, charIndex,
   	    	outIndex + dispIndexOffset);
   	for (i=0; i<charLen; i++) {
   	    if (i != 0 && charIndex < lineLen && lineStr[charIndex] == '\t')
   		charStyle = styleOfPos(textD, lineStartPos, lineLen,
   		    	charIndex, outIndex + dispIndexOffset);
     	    if (charStyle != style) {
    		drawString(textD, style, startX, y, x, outStr, outPtr - outStr);
    		outPtr = outStr;
    		startX = x;
    		style = charStyle;
    	    }
    	    if (charIndex < lineLen) {
    		*outPtr = expandedChar[i];
    		charWidth = stringWidth(textD, &expandedChar[i], 1, charStyle);
    	    } else
    		charWidth = stdCharWidth;
    	    outPtr++;
    	    x += charWidth;
    	    outIndex++;
	}
    	if (outPtr-outStr+MAX_EXP_CHAR_LEN>=MAX_DISP_LINE_LEN || x>=rightClip)
    	    break;
    }
    
    /* Draw the remaining style segment */
    drawString(textD, style, startX, y, x, outStr, outPtr - outStr);
    
    /* Draw the cursor if part of it appeared on the redisplayed part of
       this line.  Also check for the cases which are not caught as the
       line is scanned above: when the cursor appears at the very end
       of the redisplayed section. */
    if (textD->cursorOn) {
	if (hasCursor)
    	    drawCursor(textD, cursorX, y);
	else if (charIndex<lineLen && (lineStartPos+charIndex+1 == cursorPos)
	    	&& x == rightClip) {
    	    if (cursorPos >= buf->length)
    	    	drawCursor(textD, x - 1, y);
    	    else {
    		if (wrapUsesCharacter(textD, cursorPos))
    	    	    drawCursor(textD, x - 1, y);
    	    }
    	}
    }
    
    if (lineStr != NULL)
    	XtFree(lineStr);
}

/*
** Draw a string or blank area according to parameter "style", using the
** appropriate colors and drawing method for that style, with top left
** corner at x, y.  If style says to draw text, use "string" as source of
** characters, and draw "nChars", if style is FILL, erase
** rectangle where text would have drawn from x to toX and from y to
** the maximum y extent of the current font(s).
*/
static void drawString(textDisp *textD, int style, int x, int y, int toX,
	char *string, int nChars)
{
    GC gc;
    XGCValues gcValues;
    XFontStruct *fs = textD->fontStruct;
    styleTableEntry *styleRec;
    
    /* Don't draw if widget isn't realized */
    if (XtWindow(textD->w) == 0)
    	return;
    
    /* Draw blank area rather than text, if that was the request */
    if (style & FILL_MASK) {
	clearRect(textD, style, x, y, toX - x, textD->ascent + textD->descent);
        return;
    }
    
    /* Set font, color, and gc depending on style.  For normal text, GCs
       for normal drawing, or drawing within a selection or highlight are
       pre-allocated and pre-configured.  For syntax highlighting, GCs are
       configured here, on the fly. */
    if (style & STYLE_LOOKUP_MASK) {
    	styleRec = &textD->styleTable[(style & STYLE_LOOKUP_MASK) - 'A'];
    	fs = styleRec->font;
    	gc = textD->styleGC ;
	gcValues.font = fs->fid;
	gcValues.foreground = styleRec->color;
	gcValues.background = style&PRIMARY_MASK ? textD->selectBGPixel :
	    	style&HIGHLIGHT_MASK ? textD->highlightBGPixel : textD->bgPixel;
    	if (gcValues.foreground == gcValues.background) /* B&W kludge */
    	    gcValues.foreground = textD->bgPixel;
    	XChangeGC(XtDisplay(textD->w), gc,
    	    	GCFont | GCForeground | GCBackground, &gcValues);
    } else if (style & HIGHLIGHT_MASK)
    	gc = textD->highlightGC;
    else if (style & PRIMARY_MASK)
    	gc = textD->selectGC;
    else
    	gc = textD->gc;

    /* Draw the string using gc and font set above */
    XDrawImageString(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
    	    y + textD->ascent, string, nChars);
    
    /* If any space around the character remains unfilled (due to use of
       different sized fonts for highlighting), fill in above or below
       to erase previously drawn characters */
    if (fs->ascent < textD->ascent)
    	clearRect(textD, style, x, y, toX - x, textD->ascent - fs->ascent);
    if (fs->descent < textD->descent)
    	clearRect(textD, style, x, y + textD->ascent + fs->descent, toX - x,
    		textD->descent - fs->descent);

    /* Underline if style is secondary selection */
    if (style & SECONDARY_MASK)
    	XDrawLine(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
    	    	y + textD->ascent, toX - 1, y + fs->ascent);
}

/*
** Clear a rectangle with the appropriate background color for "style"
*/
static void clearRect(textDisp *textD, int style, int x, int y, 
    	int width, int height)
{
    /* A width of zero means "clear to end of window" to XClearArea */
    if (width == 0)
    	return;
    
    if (style & HIGHLIGHT_MASK)
    	XFillRectangle(XtDisplay(textD->w), XtWindow(textD->w),
    	    	textD->highlightBGGC, x, y, width, height);
    else if (style & PRIMARY_MASK)
    	XFillRectangle(XtDisplay(textD->w), XtWindow(textD->w),
    	    	textD->selectBGGC, x, y, width, height);
    else
    	XClearArea(XtDisplay(textD->w), XtWindow(textD->w), x, y,
    	    	width, height, False);
}


/*
** Draw a cursor with top center at x, y.
*/
static void drawCursor(textDisp *textD, int x, int y)
{
    XSegment segs[5];
    int left, right, cursorWidth, midY;
    int fontWidth = textD->fontStruct->min_bounds.width, nSegs = 0;  
    int fontHeight = textD->ascent + textD->descent;
    int bot = y + fontHeight - 1;
    
    if (XtWindow(textD->w) == 0 || x < textD->left-1 ||
	    x > textD->left + textD->width)
    	return;
    
    /* For cursors other than the block, make them around 2/3 of a character
       width, rounded to an even number of pixels so that X will draw an
       odd number centered on the stem at x. */
    cursorWidth = (fontWidth/3) * 2;
    left = x - cursorWidth/2;
    right = left + cursorWidth;
    
    /* Create segments and draw cursor */
    if (textD->cursorStyle == CARET_CURSOR) {
    	midY = bot - fontHeight/5;
    	segs[0].x1 = left; segs[0].y1 = bot; segs[0].x2 = x; segs[0].y2 = midY;
    	segs[1].x1 = x; segs[1].y1 = midY; segs[1].x2 = right; segs[1].y2 = bot;
    	segs[2].x1 = left; segs[2].y1 = bot; segs[2].x2 = x; segs[2].y2=midY-1;
    	segs[3].x1 = x; segs[3].y1=midY-1; segs[3].x2 = right; segs[3].y2 = bot;
    	nSegs = 4;
    } else if (textD->cursorStyle == NORMAL_CURSOR) {
	segs[0].x1 = left; segs[0].y1 = y; segs[0].x2 = right; segs[0].y2 = y;
	segs[1].x1 = x; segs[1].y1 = y; segs[1].x2 = x; segs[1].y2 = bot;
	segs[2].x1 = left; segs[2].y1 = bot; segs[2].x2 = right; segs[2].y2=bot;
	nSegs = 3;
    } else if (textD->cursorStyle == HEAVY_CURSOR) {
	segs[0].x1 = x-1; segs[0].y1 = y; segs[0].x2 = x-1; segs[0].y2 = bot;
	segs[1].x1 = x; segs[1].y1 = y; segs[1].x2 = x; segs[1].y2 = bot;
	segs[2].x1 = x+1; segs[2].y1 = y; segs[2].x2 = x+1; segs[2].y2 = bot;
	segs[3].x1 = left; segs[3].y1 = y; segs[3].x2 = right; segs[3].y2 = y;
	segs[4].x1 = left; segs[4].y1 = bot; segs[4].x2 = right; segs[4].y2=bot;
	nSegs = 5;
    } else if (textD->cursorStyle == DIM_CURSOR) {
	midY = y + fontHeight/2;
	segs[0].x1 = x; segs[0].y1 = y; segs[0].x2 = x; segs[0].y2 = y;
	segs[1].x1 = x; segs[1].y1 = midY; segs[1].x2 = x; segs[1].y2 = midY;
	segs[2].x1 = x; segs[2].y1 = bot; segs[2].x2 = x; segs[2].y2 = bot;
	nSegs = 3;
    } else if (textD->cursorStyle == BLOCK_CURSOR) {
	right = x + fontWidth;
	segs[0].x1 = x; segs[0].y1 = y; segs[0].x2 = right; segs[0].y2 = y;
	segs[1].x1 = right; segs[1].y1 = y; segs[1].x2 = right; segs[1].y2=bot;
	segs[2].x1 = right; segs[2].y1 = bot; segs[2].x2 = x; segs[2].y2 = bot;
	segs[3].x1 = x; segs[3].y1 = bot; segs[3].x2 = x; segs[3].y2 = y;
	nSegs = 4;
    }
    XDrawSegments(XtDisplay(textD->w), XtWindow(textD->w),
    	    textD->cursorFGGC, segs, nSegs);
    
    /* Save the last position drawn */
    textD->cursorX = x;
    textD->cursorY = y;
}

/*
** Determine the drawing method to use to draw a specific character from "buf".
** "lineStartPos" gives the character index where the line begins, "lineIndex",
** the number of characters past the beginning of the line, and "dispIndex",
** the number of displayed characters past the beginning of the line.  Passing
** lineStartPos of -1 returns the drawing style for "no text".
**
** Why not just: styleOfPos(textD, pos)?  Because style applies to blank areas
** of the window beyond the text boundaries, and because this routine must also
** decide whether a position is inside of a rectangular selection, and do so
** efficiently, without re-counting character positions from the start of the
** line.
**
** Note that style is a somewhat incorrect name, drawing method would
** be more appropriate.
*/
static int styleOfPos(textDisp *textD, int lineStartPos,
    	int lineLen, int lineIndex, int dispIndex)
{
    textBuffer *buf = textD->buffer;
    textBuffer *styleBuf = textD->styleBuffer;
    int pos, style = 0;
    
    if (lineStartPos == -1 || buf == NULL)
    	return FILL_MASK;
    
    pos = lineStartPos + min(lineIndex, lineLen);
    
    if (lineIndex >= lineLen)
   	style = FILL_MASK;
    else if (styleBuf != NULL) {
    	style = (unsigned char)BufGetCharacter(styleBuf, pos);
    	if (style == textD->unfinishedStyle) {
    	    /* encountered "unfinished" style, trigger parsing */
    	    (textD->unfinishedHighlightCB)(textD, pos, textD->highlightCBArg);
    	    style = (unsigned char)BufGetCharacter(styleBuf, pos);
    	}
    }
    if (inSelection(&buf->primary, pos, lineStartPos, dispIndex))
    	style |= PRIMARY_MASK;
    if (inSelection(&buf->highlight, pos, lineStartPos, dispIndex))
    	style |= HIGHLIGHT_MASK;
    if (inSelection(&buf->secondary, pos, lineStartPos, dispIndex))
    	style |= SECONDARY_MASK;
    return style;
}

/*
** Find the width of a string in the font of a particular style
*/
static int stringWidth(textDisp *textD, char *string, int length, int style)
{
    XFontStruct *fs;
    
    if (style & STYLE_LOOKUP_MASK)
    	fs = textD->styleTable[(style & STYLE_LOOKUP_MASK) - 'A'].font;
    else 
    	fs = textD->fontStruct;
    return XTextWidth(fs, string, length);
}

/*
** Return true if position "pos" with indentation "dispIndex" is in
** selection "sel"
*/
static int inSelection(selection *sel, int pos, int lineStartPos, int dispIndex)
{
    return sel->selected &&
    	 ((!sel->rectangular &&
    	   pos >= sel->start && pos < sel->end) ||
    	  (sel->rectangular &&
    	   pos >= sel->start && lineStartPos <= sel->end &&
     	   dispIndex >= sel->rectStart && dispIndex < sel->rectEnd));
}

/*
** Translate window coordinates to the nearest (insert cursor or character
** cell) text position.  The parameter posType specifies how to interpret the
** position: CURSOR_POS means translate the coordinates to the nearest cursor
** position, and CHARACTER_POS means return the position of the character
** closest to (x, y).
*/
static int xyToPos(textDisp *textD, int x, int y, int posType)
{
    int charIndex, lineStart, lineLen, fontHeight;
    int charWidth, charLen, charStyle, visLineNum, xStep, outIndex;
    char *lineStr, expandedChar[MAX_EXP_CHAR_LEN];

    /* Find the visible line number corresponding to the y coordinate */
    fontHeight = textD->ascent + textD->descent;
    visLineNum = (y - textD->top) / fontHeight;
    if (visLineNum < 0)
	return textD->firstChar;
    if (visLineNum >= textD->nVisibleLines)
	visLineNum = textD->nVisibleLines - 1;
    
    /* Find the position at the start of the line */
    lineStart = textD->lineStarts[visLineNum];
    
    /* If the line start was empty, return the last position in the buffer */
    if (lineStart == -1)
    	return textD->buffer->length;
    
    /* Get the line text and its length */
    lineLen = visLineLength(textD, visLineNum);
    lineStr = BufGetRange(textD->buffer, lineStart, lineStart + lineLen);
    
    /* Step through character positions from the beginning of the line
       to find the character position corresponding to the x coordinate */
    xStep = textD->left - textD->horizOffset;
    outIndex = 0;
    for(charIndex=0; charIndex<lineLen; charIndex++) {
    	charLen = BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
    		textD->buffer->tabDist, textD->buffer->nullSubsChar);
   	charStyle = styleOfPos(textD, lineStart, lineLen, charIndex, outIndex);
    	charWidth = stringWidth(textD, expandedChar, charLen, charStyle);
    	if (x < xStep + (posType == CURSOR_POS ? charWidth/2 : charWidth)) {
    	    XtFree(lineStr);
    	    return lineStart + charIndex;
    	}
    	xStep += charWidth;
    	outIndex += charLen;
    }
    
    /* If the x position was beyond the end of the line, return the position
       of the newline at the end of the line */
    XtFree(lineStr);
    return lineStart + lineLen;
}

/*
** Translate window coordinates to the nearest row and column number for
** positioning the cursor.  This, of course, makes no sense when the font is
** proportional, since there are no absolute columns.  The parameter posType
** specifies how to interpret the position: CURSOR_POS means translate the
** coordinates to the nearest position between characters, and CHARACTER_POS
** means translate the position to the nearest character cell.
*/
static void xyToUnconstrainedPos(textDisp *textD, int x, int y, int *row,
	int *column, int posType)
{
    int fontHeight = textD->ascent + textD->descent;
    int fontWidth = textD->fontStruct->max_bounds.width;

    /* Find the visible line number corresponding to the y coordinate */
    *row = (y - textD->top) / fontHeight;
    if (*row < 0) *row = 0;
    if (*row >= textD->nVisibleLines) *row = textD->nVisibleLines - 1;
    *column = ((x-textD->left) + textD->horizOffset +
    	    (posType == CURSOR_POS ? fontWidth/2 : 0)) / fontWidth;
    if (*column < 0) *column = 0;
}

/*
** Offset the line starts array, topLineNum, firstChar and lastChar, for a new
** vertical scroll position given by newTopLineNum.  If any currently displayed
** lines will still be visible, salvage the line starts values, otherwise,
** count lines from the nearest known line start (start or end of buffer, or
** the closest value in the lineStarts array)
*/
static void offsetLineStarts(textDisp *textD, int newTopLineNum)
{
    int oldTopLineNum = textD->topLineNum;
    int lineDelta = newTopLineNum - oldTopLineNum;
    int nVisLines = textD->nVisibleLines;
    int *lineStarts = textD->lineStarts;
    int i, lastLineNum;
    textBuffer *buf = textD->buffer;
    
    /* If there was no offset, nothing needs to be changed */
    if (lineDelta == 0)
    	return;
    	
    /* {   int i;
    	printf("Scroll, lineDelta %d\n", lineDelta);
    	printf("lineStarts Before: ");
    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	printf("\n");
    } */
    
    /* Find the new value for firstChar by counting lines from the nearest
       known line start (start or end of buffer, or the closest value in the
       lineStarts array) */
    lastLineNum = oldTopLineNum + nVisLines - 1;
    if (newTopLineNum < oldTopLineNum && newTopLineNum < -lineDelta) {
    	textD->firstChar = TextDCountForwardNLines(textD, 0, newTopLineNum-1,
    	    	True);
    	/* printf("counting forward %d lines from start\n", newTopLineNum-1);*/
    } else if (newTopLineNum < oldTopLineNum) {
    	textD->firstChar = TextDCountBackwardNLines(textD, textD->firstChar,
    		-lineDelta);
    	/* printf("counting backward %d lines from firstChar\n", -lineDelta);*/
    } else if (newTopLineNum < lastLineNum) {
    	textD->firstChar = lineStarts[newTopLineNum - oldTopLineNum];
    	/* printf("taking new start from lineStarts[%d]\n",
    		newTopLineNum - oldTopLineNum); */
    } else if (newTopLineNum-lastLineNum < textD->nBufferLines-newTopLineNum) {
    	textD->firstChar = TextDCountForwardNLines(textD, lineStarts[nVisLines-1],
    		newTopLineNum - lastLineNum, True);
    	/* printf("counting forward %d lines from start of last line\n",
    		newTopLineNum - lastLineNum); */
    } else {
    	textD->firstChar = TextDCountBackwardNLines(textD, buf->length,
		textD->nBufferLines - newTopLineNum + 1);
	/* printf("counting backward %d lines from end\n",
    		textD->nBufferLines - newTopLineNum + 1); */
    }
    
    /* Fill in the line starts array */
    if (lineDelta < 0 && -lineDelta < nVisLines) {
    	for (i=nVisLines-1; i >= -lineDelta; i--)
    	    lineStarts[i] = lineStarts[i+lineDelta];
    	calcLineStarts(textD, 0, -lineDelta);
    } else if (lineDelta > 0 && lineDelta < nVisLines) {
    	for (i=0; i<nVisLines-lineDelta; i++)
    	    lineStarts[i] = lineStarts[i+lineDelta];
    	calcLineStarts(textD, nVisLines-lineDelta, nVisLines-1);
    } else
	calcLineStarts(textD, 0, nVisLines);
    
    /* Set lastChar and topLineNum */
    calcLastChar(textD);
    textD->topLineNum = newTopLineNum;
    
    /* {   int i;
    	printf("lineStarts After: ");
    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	printf("\n");
    } */
}

/*
** Update the line starts array, topLineNum, firstChar and lastChar for text
** display "textD" after a modification to the text buffer, given by the
** position where the change began "pos", and the nmubers of characters
** and lines inserted and deleted.
*/
static void updateLineStarts(textDisp *textD, int pos, int charsInserted,
	int charsDeleted, int linesInserted, int linesDeleted, int *scrolled)
{
    int *lineStarts = textD->lineStarts;
    int i, lineOfPos, lineOfEnd, nVisLines = textD->nVisibleLines;
    int charDelta = charsInserted - charsDeleted;
    int lineDelta = linesInserted - linesDeleted;

    /* {   int i;
    	printf("linesDeleted %d, linesInserted %d, charsInserted %d, charsDeleted %d\n",
    	    	linesDeleted, linesInserted, charsInserted, charsDeleted);
    	printf("lineStarts Before: ");
    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	printf("\n");
    } */
    /* If all of the changes were before the displayed text, the display
       doesn't change, just update the top line num and offset the line
       start entries and first and last characters */
    if (pos + charsDeleted < textD->firstChar) {
    	textD->topLineNum += lineDelta;
    	for (i=0; i<nVisLines; i++)
    	    lineStarts[i] += charDelta;
    	/* {   int i;
    	    printf("lineStarts after delete doesn't touch: ");
    	    for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	    printf("\n");
    	} */
    	textD->firstChar += charDelta;
    	textD->lastChar += charDelta;
    	*scrolled = False;
    	return;
    }
    
    /* The change began before the beginning of the displayed text, but
       part or all of the displayed text was deleted */
    if (pos < textD->firstChar) {
    	/* If some text remains in the window, anchor on that  */
    	if (posToVisibleLineNum(textD, pos + charsDeleted, &lineOfEnd) &&
    		++lineOfEnd < nVisLines && lineStarts[lineOfEnd] != -1) {
    	    textD->topLineNum = max(1, textD->topLineNum + lineDelta);
    	    textD->firstChar = TextDCountBackwardNLines(textD,
    	    	    lineStarts[lineOfEnd] + charDelta, lineOfEnd);
    	/* Otherwise anchor on original line number and recount everything */
    	} else {
    	    if (textD->topLineNum > textD->nBufferLines + lineDelta) {
    	    	textD->topLineNum = 1;
    	    	textD->firstChar = 0;
    	    } else
    		textD->firstChar = TextDCountForwardNLines(textD, 0,
    	    		textD->topLineNum - 1, True);
    	}
    	calcLineStarts(textD, 0, nVisLines-1);
    	/* {   int i;
    	    printf("lineStarts after delete encroaches: ");
    	    for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	    printf("\n");
    	} */
    	/* calculate lastChar by finding the end of the last displayed line */
    	calcLastChar(textD);
    	*scrolled = True;
    	return;
    }
    
    /* If the change was in the middle of the displayed text (it usually is),
       salvage as much of the line starts array as possible by moving and
       offsetting the entries after the changed area, and re-counting the
       added lines or the lines beyond the salvaged part of the line starts
       array */
    if (pos <= textD->lastChar) {
    	/* find line on which the change began */
    	posToVisibleLineNum(textD, pos, &lineOfPos);
    	/* salvage line starts after the changed area */
    	if (lineDelta == 0) {
    	    for (i=lineOfPos+1; i<nVisLines && lineStarts[i]!= -1; i++)
    		lineStarts[i] += charDelta;
    	} else if (lineDelta > 0) {
    	    for (i=nVisLines-1; i>=lineOfPos+lineDelta+1; i--)
    		lineStarts[i] = lineStarts[i-lineDelta] +
    			(lineStarts[i-lineDelta] == -1 ? 0 : charDelta);
    	} else /* (lineDelta < 0) */ {
    	    for (i=max(0,lineOfPos+1); i<nVisLines+lineDelta; i++)
    	    	lineStarts[i] = lineStarts[i-lineDelta] +
    	    		(lineStarts[i-lineDelta] == -1 ? 0 : charDelta);
    	}
    	/* {   int i;
    	    printf("lineStarts after salvage: ");
    	    for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	    printf("\n");
    	} */
    	/* fill in the missing line starts */
    	if (linesInserted >= 0)
    	    calcLineStarts(textD, lineOfPos + 1, lineOfPos + linesInserted);
    	if (lineDelta < 0)
    	    calcLineStarts(textD, nVisLines+lineDelta, nVisLines);
    	/* {   int i;
    	    printf("lineStarts after recalculation: ");
    	    for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	    printf("\n");
    	} */
    	/* calculate lastChar by finding the end of the last displayed line */
    	calcLastChar(textD);
    	*scrolled = False;
    	return;
    }
    
    /* Change was past the end of the displayed text, but displayable by virtue
       of being an insert at the end of the buffer into visible blank lines */
    if (emptyLinesVisible(textD)) {
    	posToVisibleLineNum(textD, pos, &lineOfPos);
    	calcLineStarts(textD, lineOfPos, lineOfPos+linesInserted);
    	calcLastChar(textD);
    	/* {   int i;
    	    printf("lineStarts after insert at end: ");
    	    for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
    	    printf("\n");
    	} */
    	*scrolled = False;
    	return;
    }
    
    /* Change was beyond the end of the buffer and not visible, do nothing */
    *scrolled = False;
}

/*
** Scan through the text in the "textD"'s buffer and recalculate the line
** starts array values beginning at index "startLine" and continuing through
** (including) "endLine".  It assumes that the line starts entry preceding
** "startLine" (or textD->firstChar if startLine is 0) is good, and re-counts
** newlines to fill in the requested entries.  Out of range values for
** "startLine" and "endLine" are acceptable.
*/
static void calcLineStarts(textDisp *textD, int startLine, int endLine)
{
    int startPos, bufLen = textD->buffer->length;
    int line, lineEnd, nextLineStart, nVis = textD->nVisibleLines;
    int *lineStarts = textD->lineStarts;
    
    /* Clean up (possibly) messy input parameters */
    if (endLine < 0) endLine = 0;
    if (endLine >= nVis) endLine = nVis - 1;
    if (startLine < 0) startLine = 0;
    if (startLine >=nVis) startLine = nVis - 1;
    if (startLine > endLine)
    	return;
    
    /* Find the last known good line number -> position mapping */
    if (startLine == 0) {
    	lineStarts[0] = textD->firstChar;
    	startLine = 1;
    }
    startPos = lineStarts[startLine-1];
    
    /* If the starting position is already past the end of the text,
       fill in -1's (means no text on line) and return */
    if (startPos == -1) {
        for (line=startLine; line<=endLine; line++)
    	    lineStarts[line] = -1;
    	return;
    }
    
    /* Loop searching for ends of lines and storing the positions of the
       start of the next line in lineStarts */
    for (line=startLine; line<=endLine; line++) {
    	findLineEnd(textD, startPos, True, &lineEnd, &nextLineStart);
    	startPos = nextLineStart;
    	if (startPos >= bufLen) {
    	    /* If the buffer ends with a newline or line break, put
    	       buf->length in the next line start position (instead of
    	       a -1 which is the normal marker for an empty line) to
    	       indicate that the cursor may safely be displayed there */
    	    if (line == 0 || (lineStarts[line-1] != bufLen &&
    	    	    lineEnd != nextLineStart)) {
    	    	lineStarts[line] = bufLen;
    	    	line++;
    	    }
    	    break;
    	}
    	lineStarts[line] = startPos;
    }
    
    /* Set any entries beyond the end of the text to -1 */
    for (; line<=endLine; line++)
    	lineStarts[line] = -1;
}

/* 
** Given a textDisp with a complete, up-to-date lineStarts array, update
** the lastChar entry to point to the last buffer position displayed.
*/
static void calcLastChar(textDisp *textD)
{
    int i;
    
    for (i=textD->nVisibleLines-1; i>0 && textD->lineStarts[i]== -1; i--);
    textD->lastChar = i < 0 ? 0 :
    	    TextDEndOfLine(textD, textD->lineStarts[i], True);
}

static void setScroll(textDisp *textD, int topLineNum, int horizOffset,
	int updateVScrollBar, int updateHScrollBar)
{
    int fontHeight = textD->ascent + textD->descent;
    int origHOffset = textD->horizOffset;
    int lineDelta = textD->topLineNum - topLineNum;
    int xOffset, yOffset, srcX, srcY, dstX, dstY, width, height;
    int exactHeight = textD->height - textD->height %
    	    (textD->ascent + textD->descent);
    
    /* Do nothing if scroll position hasn't actually changed or there's no
       window to draw in yet */
    if (XtWindow(textD->w) == 0 ||  (textD->horizOffset == horizOffset &&
    	    textD->topLineNum == topLineNum))
    	return;
    
    /* If part of the cursor is protruding beyond the text clipping region,
       clear it off */
    blankCursorProtrusions(textD);

    /* If the vertical scroll position has changed, update the line
       starts array and related counters in the text display */
    offsetLineStarts(textD, topLineNum);
    
    /* Just setting textD->horizOffset is enough information for redisplay */
    textD->horizOffset = horizOffset;
    
    /* Update the scroll bar positions if requested, note: updating the
       horizontal scroll bars can have the further side-effect of changing
       the horizontal scroll position, textD->horizOffset */
    if (updateVScrollBar && textD->vScrollBar != NULL)
    	updateVScrollBarRange(textD);
    if (updateHScrollBar && textD->hScrollBar != NULL) {
    	updateHScrollBarRange(textD);
    }
    
    /* Redisplay everything if the window is partially obscured (since
       it's too hard to tell what displayed areas are salvageable) or
       if there's nothing to recover because the scroll distance is large */
    xOffset = origHOffset - textD->horizOffset;
    yOffset = lineDelta * fontHeight;
    if (textD->visibility != VisibilityUnobscured ||
    	    abs(xOffset) > textD->width || abs(yOffset) > exactHeight) {
    	TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    		textD->height);
    
    /* If the window is not obscured, paint most of the window using XCopyArea
       from existing displayed text, and redraw only what's necessary */
    } else {
	/* Recover the useable window areas by moving to the proper location */
	srcX = textD->left + (xOffset >= 0 ? 0 : -xOffset);
	dstX = textD->left + (xOffset >= 0 ? xOffset : 0);
	width = textD->width - abs(xOffset);
	srcY = textD->top + (yOffset >= 0 ? 0 : -yOffset);
	dstY = textD->top + (yOffset >= 0 ? yOffset : 0);
	height = exactHeight - abs(yOffset);
	resetClipRectangles(textD);
	XCopyArea(XtDisplay(textD->w), XtWindow(textD->w), XtWindow(textD->w),
    		textD->gc, srcX, srcY, width, height, dstX, dstY);
	/* redraw the un-recoverable parts */
	if (yOffset > 0)
    	    TextDRedisplayRect(textD, textD->left, textD->top,
    	    	    textD->width, yOffset);
	else if (yOffset < 0)
    	    TextDRedisplayRect(textD, textD->left, textD->top +
    	    	    textD->height + yOffset, textD->width, -yOffset);
	if (xOffset > 0)
    	    TextDRedisplayRect(textD, textD->left, textD->top,
    	    	    xOffset, textD->height);
	else if (xOffset < 0)
    	    TextDRedisplayRect(textD, textD->left + textD->width + xOffset,
    	    	    textD->top, -xOffset, textD->height);
	/* Restore protruding parts of the cursor */
	TextDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
    }
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for vertical scroll bar.
*/
static void updateVScrollBarRange(textDisp *textD)
{
    int sliderSize, sliderMax, sliderValue;
    
    if (textD->vScrollBar == NULL)
    	return;
    
    /* The Vert. scroll bar value and slider size directly represent the top
       line number, and the number of visible lines respectively.  The scroll
       bar maximum value is chosen to generally represent the size of the whole
       buffer, with minor adjustments to keep the scroll bar widget happy */
    sliderSize = textD->nVisibleLines;
    sliderValue = textD->topLineNum;
    sliderMax = max(textD->nBufferLines + 2, sliderSize + sliderValue);
    XtVaSetValues(textD->vScrollBar,
    	    XmNmaximum, sliderMax,
    	    XmNsliderSize, sliderSize,
     	    XmNpageIncrement, max(1, textD->nVisibleLines - 1),
   	    XmNvalue, sliderValue, 0);
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for the horizontal scroll bar.  If scroll position is such that there
** is blank space to the right of all lines of text, scroll back (adjust
** horizOffset but don't redraw) to take up the slack and position the
** right edge of the text at the right edge of the display.
** 
** Note, there is some cost to this routine, since it scans the whole range
** of displayed text, particularly since it's usually called for each typed
** character!
*/
static int updateHScrollBarRange(textDisp *textD)
{
    int i, maxWidth = 0, sliderMax, sliderWidth;
    int origHOffset = textD->horizOffset;
    
    if (textD->hScrollBar == NULL || !XtIsManaged(textD->hScrollBar))
    	return False;
    
    /* Scan all the displayed lines to find the width of the longest line */
    for (i=0; i<textD->nVisibleLines && textD->lineStarts[i]!= -1; i++)
    	maxWidth = max(measureVisLine(textD, i), maxWidth);
    
    /* If the scroll position is beyond what's necessary to keep all lines
       in view, scroll to the left to bring the end of the longest line to
       the right margin */
    if (maxWidth < textD->width + textD->horizOffset && textD->horizOffset > 0)
    	textD->horizOffset = max(0, maxWidth - textD->width);
    
    /* Readjust the scroll bar */
    sliderWidth = textD->width;
    sliderMax = max(maxWidth, sliderWidth + textD->horizOffset);
    XtVaSetValues(textD->hScrollBar,
    	    XmNmaximum, sliderMax,
    	    XmNsliderSize, sliderWidth,
    	    XmNpageIncrement, max(textD->width - 100, 10),
    	    XmNvalue, textD->horizOffset, 0);
    
    /* Return True if scroll position was changed */
    return origHOffset != textD->horizOffset;
}

/*
** Callbacks for drag or valueChanged on scroll bars
*/
static void vScrollCB(Widget w, XtPointer clientData, XtPointer callData)
{
    textDisp *textD = (textDisp *)clientData;
    int newValue = ((XmScrollBarCallbackStruct *)callData)->value;
    int lineDelta = newValue - textD->topLineNum;
    
    if (lineDelta == 0)
    	return;
    setScroll(textD, newValue, textD->horizOffset, False, True);
}
static void hScrollCB(Widget w, XtPointer clientData, XtPointer callData)
{
    textDisp *textD = (textDisp *)clientData;
    int newValue = ((XmScrollBarCallbackStruct *)callData)->value;
    
    if (newValue == textD->horizOffset)
    	return;
    setScroll(textD, textD->topLineNum, newValue, False, False);
}

static void visibilityEH(Widget w, XtPointer data, XEvent *event,
	Boolean *continueDispatch)
{
    /* Record whether the window is fully visible or not.  This information
       is used for choosing the scrolling methodology for optimal performance,
       if the window is partially obscured, XCopyArea may not work */
    ((textDisp *)data)->visibility = ((XVisibilityEvent *)event)->state;
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
** Count the number of newlines in a null-terminated text string;
*/
static int countLines(char *string)
{
    char *c;
    int lineCount = 0;
    
    for (c=string; *c!='\0'; c++)
    	if (*c == '\n') lineCount++;
    return lineCount;
}

/*
** Return the width in pixels of the displayed line pointed to by "visLineNum"
*/
static int measureVisLine(textDisp *textD, int visLineNum)
{
    int i, width = 0, len, style, lineLen = visLineLength(textD, visLineNum);
    int charCount = 0, lineStartPos = textD->lineStarts[visLineNum];
    char expandedChar[MAX_EXP_CHAR_LEN];
    
    if (textD->styleBuffer == NULL) {
	for (i=0; i<lineLen; i++) {
    	    len = BufGetExpandedChar(textD->buffer, lineStartPos + i,
    		    charCount, expandedChar);
    	    width += XTextWidth(textD->fontStruct, expandedChar, len);
    	    charCount += len;
	}
    } else {
    	for (i=0; i<lineLen; i++) {
    	    len = BufGetExpandedChar(textD->buffer, lineStartPos+i,
    		    charCount, expandedChar);
    	    style = (unsigned char)BufGetCharacter(textD->styleBuffer,
		    lineStartPos+i) - 'A';
    	    width += XTextWidth(textD->styleTable[style].font, expandedChar,
    	    	    len);
    	    charCount += len;
	}
    }
    return width;
}

/*
** Return true if there are lines visible with no corresponding buffer text
*/
static int emptyLinesVisible(textDisp *textD)
{
    return textD->nVisibleLines > 0 &&
    	    textD->lineStarts[textD->nVisibleLines-1] == -1;
}

/*
** When the cursor is at the left or right edge of the text, part of it
** sticks off into the clipped region beyond the text.  Normal redrawing
** can not overwrite this protruding part of the cursor, so it must be
** erased independently by calling this routine.
*/
static void blankCursorProtrusions(textDisp *textD)
{
    int x, width, cursorX = textD->cursorX, cursorY = textD->cursorY;
    int fontWidth = textD->fontStruct->max_bounds.width;  
    int fontHeight = textD->ascent + textD->descent;
    int cursorWidth, left = textD->left, right = left + textD->width;
    
    cursorWidth = (fontWidth/3) * 2;
    if (cursorX >= left-1 && cursorX <= left + cursorWidth/2 - 1) {
    	x = cursorX - cursorWidth/2;
    	width = left - x;
    } else if (cursorX >= right - cursorWidth/2 && cursorX <= right) {
    	x = right;
    	width = cursorX + cursorWidth/2 + 2 - right;
    } else
    	return;
    	
    XClearArea(XtDisplay(textD->w), XtWindow(textD->w), x, cursorY,
    	    width, fontHeight, False);
}

/*
** Allocate shared graphics contexts used by the widget, which must be
** re-allocated on a font change.
*/
static void allocateFixedFontGCs(textDisp *textD, XFontStruct *fontStruct,
	Pixel bgPixel, Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
	Pixel highlightFGPixel, Pixel highlightBGPixel)
{
    textD->gc = allocateGC(textD->w, GCFont | GCForeground | GCBackground,
    	    fgPixel, bgPixel, fontStruct->fid, GCClipMask, GCArcMode); 
    textD->selectGC = allocateGC(textD->w, GCFont | GCForeground | GCBackground,
    	    selectFGPixel, selectBGPixel, fontStruct->fid, GCClipMask,
    	    GCArcMode);
    textD->selectBGGC = allocateGC(textD->w, GCForeground, selectBGPixel, 0,
    	    fontStruct->fid, GCClipMask, GCArcMode);
    textD->highlightGC = allocateGC(textD->w, GCFont|GCForeground|GCBackground,
    	    highlightFGPixel, highlightBGPixel, fontStruct->fid, GCClipMask,
    	    GCArcMode);
    textD->highlightBGGC = allocateGC(textD->w, GCForeground, highlightBGPixel,
	    0, fontStruct->fid, GCClipMask, GCArcMode);
}

/*
** X11R4 does not have the XtAllocateGC function for sharing graphics contexts
** with changeable fields.  Unfortunately the R4 call for creating shared
** graphics contexts (XtGetGC) is rarely useful because most widgets need
** to be able to set and change clipping, and that makes the GC unshareable.
**
** This function allocates and returns a gc, using XtAllocateGC if possible,
** or XCreateGC on X11R4 systems where XtAllocateGC is not available.
*/
static GC allocateGC(Widget w, unsigned long valueMask,
	unsigned long foreground, unsigned long background, Font font,
	unsigned long dynamicMask, unsigned long dontCareMask)
{
    XGCValues gcValues;

    gcValues.font = font;
    gcValues.background = background;
    gcValues.foreground = foreground;
#if defined(XlibSpecificationRelease) && XlibSpecificationRelease > 4
    return XtAllocateGC(w, 0, valueMask, &gcValues, dynamicMask,
    	    dontCareMask);
#else
    return XCreateGC(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
    	    valueMask, &gcValues);
#endif
}

/*
** Release a gc allocated with allocateGC above
*/
static void releaseGC(Widget w, GC gc)
{
#if defined(XlibSpecificationRelease) && XlibSpecificationRelease > 4
    XtReleaseGC(w, gc);
#else
    XFreeGC(XtDisplay(w), gc);
#endif
}

/*
** resetClipRectangles sets the clipping rectangles for GCs which clip
** at the text boundary (as opposed to the window boundary).  These GCs
** are shared such that the drawing styles are constant, but the clipping
** rectangles are allowed to change among different users of the GCs (the
** GCs were created with XtAllocGC).  This routine resets them so the clipping
** rectangles are correct for this text display.
*/
static void resetClipRectangles(textDisp *textD)
{
    XRectangle clipRect;
    Display *display = XtDisplay(textD->w);
    
    clipRect.x = textD->left;
    clipRect.y = textD->top;
    clipRect.width = textD->width;
    clipRect.height = textD->height - textD->height %
    	    (textD->ascent + textD->descent);
    
    XSetClipRectangles(display, textD->gc, 0, 0,
    	    &clipRect, 1, Unsorted);
    XSetClipRectangles(display, textD->selectGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(display, textD->highlightGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(display, textD->selectBGGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(display, textD->highlightBGGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(display, textD->styleGC, 0, 0,
            &clipRect, 1, Unsorted);
} 

/*
** Return the length of a line (number of displayable characters) by examining
** entries in the line starts array rather than by scanning for newlines
*/
static int visLineLength(textDisp *textD, int visLineNum)
{
    int nextLineStart, lineStartPos = textD->lineStarts[visLineNum];
    
    if (lineStartPos == -1)
    	return 0;
    if (visLineNum+1 >= textD->nVisibleLines)
    	return textD->lastChar - lineStartPos;
    nextLineStart = textD->lineStarts[visLineNum+1];
    if (nextLineStart == -1)
	return textD->lastChar - lineStartPos;
    if (wrapUsesCharacter(textD, nextLineStart-1))
    	return nextLineStart-1 - lineStartPos;
    return nextLineStart - lineStartPos;
}

/*
** When continuous wrap is on, and the user inserts or deletes characters,
** wrapping can happen before and beyond the changed position.  This routine
** finds the extent of the changes, and counts the deleted and inserted lines
** over that range.  It also attempts to minimize the size of the range to
** what has to be counted and re-displayed, so the results can be useful
** both for delimiting where the line starts need to be recalculated, and
** for deciding what part of the text to redisplay.
*/
static void findWrapRange(textDisp *textD, char *deletedText, int pos,
    	int nInserted, int nDeleted, int *modRangeStart, int *modRangeEnd,
    	int *linesInserted, int *linesDeleted)
{
    int length, retPos, retLines, retLineStart, retLineEnd;
    textBuffer *deletedTextBuf, *buf = textD->buffer;
    int nVisLines = textD->nVisibleLines;
    int *lineStarts = textD->lineStarts;
    int countFrom, countTo, lineStart, adjLineStart, i;
    int visLineNum = 0, nLines = 0;
    
    /*
    ** Determine where to begin searching: either the previous newline, or
    ** if possible, limit to the start of the (original) previous displayed
    ** line, using information from the existing line starts array
    */
    if (pos >= textD->firstChar && pos <= textD->lastChar) {
    	for (i=nVisLines-1; i>0; i--)
    	    if (lineStarts[i] != -1 && pos >= lineStarts[i])
    		break;
    	if (i > 0) {
    	    countFrom = lineStarts[i-1];
    	    visLineNum = i-1;
    	} else
    	    countFrom = BufStartOfLine(buf, pos);
    } else
    	countFrom = BufStartOfLine(buf, pos);

    
    /*
    ** Move forward through the (new) text one line at a time, counting
    ** displayed lines, and looking for either a real newline, or for the
    ** line starts to re-sync with the original line starts array
    */
    lineStart = countFrom;
    *modRangeStart = countFrom;
    while (True) {
    	
    	/* advance to the next line.  If the line ended in a real newline
    	   or the end of the buffer, that's far enough */
    	wrappedLineCounter(textD, buf, lineStart, buf->length, 1, True,
    	    	&retPos, &retLines, &retLineStart, &retLineEnd);
    	if (retPos >= buf->length) {
    	    countTo = buf->length;
    	    *modRangeEnd = countTo;
    	    if (retPos != retLineEnd)
    	    	nLines++;
    	    break;
    	} else
    	    lineStart = retPos;
    	nLines++;
    	if (lineStart > pos + nInserted &&
    	    	BufGetCharacter(buf, lineStart-1) == '\n') {
    	    countTo = lineStart;
    	    *modRangeEnd = lineStart;
    	    break;
    	}
    	
    	/* check for synchronization with the original line starts array
    	   before pos, if so, the modified range can begin later */
     	if (lineStart <= pos) {
    	    while (visLineNum<nVisLines && lineStarts[visLineNum] < lineStart)
    		visLineNum++;
     	    if (visLineNum < nVisLines && lineStarts[visLineNum] == lineStart) {
    		countFrom = lineStart;
    		nLines = 0;
    		if (visLineNum+1 < nVisLines && lineStarts[visLineNum+1] != -1)
    		    *modRangeStart = min(pos, lineStarts[visLineNum+1]-1);
    		else
    		    *modRangeStart = countFrom;
    	    } else
    	    	*modRangeStart = min(*modRangeStart, lineStart-1);
    	}
    	
   	/* check for synchronization with the original line starts array
    	   after pos, if so, the modified range can end early */
    	else if (lineStart > pos + nInserted) {
    	    adjLineStart = lineStart - nInserted + nDeleted;
    	    while (visLineNum<nVisLines && lineStarts[visLineNum]<adjLineStart)
    	    	visLineNum++;
    	    if (visLineNum < nVisLines && lineStarts[visLineNum] != -1 &&
    	    	    lineStarts[visLineNum] == adjLineStart) {
    	    	countTo = TextDEndOfLine(textD, lineStart, True);
    	    	*modRangeEnd = lineStart;
    	    	break;
    	    }
    	}
    }
    *linesInserted = nLines;


    /* Count deleted lines between countFrom and countTo as the text existed
       before the modification (that is, as if the text between pos and
       pos+nInserted were replaced by "deletedText").  This extra context is
       necessary because wrapping can occur outside of the modified region
       as a result of adding or deleting text in the region. This is done by
       creating a textBuffer containing the deleted text and the necessary
       additional context, and calling the wrappedLineCounter on it. */
    length = (pos-countFrom) + nDeleted +(countTo-(pos+nInserted));
    deletedTextBuf = BufCreatePreallocated(length);
    BufCopyFromBuf(textD->buffer, deletedTextBuf, countFrom, pos, 0);
    if (nDeleted != 0)
    	BufInsert(deletedTextBuf, pos-countFrom, deletedText);
    BufCopyFromBuf(textD->buffer, deletedTextBuf,
    	    pos+nInserted, countTo, pos-countFrom+nDeleted);
    wrappedLineCounter(textD, deletedTextBuf, 0, length, INT_MAX, True,
    	    &retPos, &retLines, &retLineStart, &retLineEnd);
    BufFree(deletedTextBuf);
    *linesDeleted = retLines;
}

/*
** Count forward from startPos to either maxPos or maxLines (whichever is
** reached first), and return all relevant positions and line count.
**
** Returned values:
**
**   retPos:	    Position where counting ended.  When counting lines, the
**  	    	    position returned is the start of the line "maxLines"
**  	    	    lines beyond "startPos".
**   retLines:	    Number of line breaks counted
**   retLineStart:  Start of the line where counting ended
**   retLineEnd:    End position of the last line traversed
*/
static void wrappedLineCounter(textDisp *textD, textBuffer *buf, int startPos,
    	int maxPos, int maxLines, int startPosIsLineStart, int *retPos,
    	int *retLines, int *retLineStart, int *retLineEnd)
{
    int lineStart, newLineStart, b, p, colNum, wrapMargin;
    int maxWidth, width, countPixels, expLen, i, foundBreak;
    int nLines = 0, tabDist = textD->buffer->tabDist;
    int fontWidth = textD->fontStruct->max_bounds.width;
    unsigned char c;
    char expChar[MAX_EXP_CHAR_LEN], nullSubsChar = textD->buffer->nullSubsChar;
    
    /* If the font is fixed, or there's a wrap margin set, it's more efficient
       to measure in columns, than to count pixels.  Determine if we can count
       in columns (countPixels == False) or must count pixels (countPixels ==
       True), and set the wrap target for either pixels or columns */
    if (textD->fontStruct->min_bounds.width == fontWidth ||
    	    textD->wrapMargin != 0) {
    	countPixels = False;
	wrapMargin = textD->wrapMargin != 0 ? textD->wrapMargin :
            	textD->width / fontWidth;
        maxWidth = INT_MAX;
    } else {
    	countPixels = True;
    	wrapMargin = INT_MAX;
    	maxWidth = textD->width;
    }
    
    /* Find the start of the line if the start pos is not marked as a
       line start. */
    if (startPosIsLineStart)
	lineStart = startPos;
    else
	lineStart = TextDStartOfLine(textD, startPos);
    
    /*
    ** Loop until position exceeds maxPos or line count exceeds maxLines.
    ** (actually, contines beyond maxPos to end of line containing maxPos,
    ** in case later characters cause a word wrap back before maxPos)
    */
    colNum = 0;
    width = 0;
    for (p=lineStart; p<buf->length; p++) {
    	c = BufGetCharacter(buf, p);

    	/* If the character was a newline, count the line and start over,
    	   otherwise, add it to the width and column counts */
    	if (c == '\n') {
    	    if (p >= maxPos) {
    		*retPos = maxPos;
    		*retLines = nLines;
    		*retLineStart = lineStart;
    		*retLineEnd = maxPos;
    		return;
    	    }
    	    nLines++;
    	    if (nLines >= maxLines) {
    		*retPos = p + 1;
    		*retLines = nLines;
    		*retLineStart = p + 1;
    		*retLineEnd = p;
    		return;
    	    }
    	    lineStart = p + 1;
    	    colNum = 0;
    	    width = 0;
    	} else {
    	    colNum += BufCharWidth(c, colNum, tabDist, nullSubsChar);
    	    if (countPixels) {
    	    	expLen = BufExpandCharacter(c, colNum, expChar, tabDist,
			nullSubsChar);
    	    	width += XTextWidth(textD->fontStruct, expChar, expLen);
    	    }
    	}

    	/* If character exceeded wrap margin, find the break point
    	   and wrap there */
    	if (colNum > wrapMargin || width > maxWidth) {
    	    foundBreak = False;
    	    for (b=p; b>=lineStart; b--) {
    	    	c = BufGetCharacter(buf, b);
    	    	if (c == '\t' || c == ' ') {
    	    	    newLineStart = b + 1;
    	    	    if (countPixels) {
    	    	    	colNum = 0;
    	    	    	width = 0;
    	    	    	for (i=b+1; i<p+1; i++) {
    	    	    	    expLen = BufGetExpandedChar(buf, i, colNum,expChar);
    	    	    	    width += XTextWidth(textD->fontStruct, expChar,
    	    	    	    	    expLen);
    	    	    	    colNum++;
    	    	    	}
    	    	    } else
    	    	    	colNum = BufCountDispChars(buf, b+1, p+1);
    	    	    foundBreak = True;
    	    	    break;
    	    	}
    	    }
    	    if (!foundBreak) { /* no whitespace, just break at margin */
    	    	newLineStart = p;
    	    	colNum = BufCharWidth(c, colNum, tabDist, nullSubsChar);
    	    	if (countPixels) {
   	    	    expLen = BufExpandCharacter(c, colNum, expChar, tabDist,
			    nullSubsChar);
    	    	    width = XTextWidth(textD->fontStruct, expChar, expLen);
    	    	}
    	    }
    	    if (p >= maxPos) {
    		*retPos = maxPos;
    		*retLines = maxPos < newLineStart ? nLines : nLines + 1;
    		*retLineStart = maxPos < newLineStart ? lineStart :
    		    	newLineStart;
    		*retLineEnd = maxPos;
    		return;
    	    }
    	    nLines++;
    	    if (nLines >= maxLines) {
    		*retPos = foundBreak ? b + 1 : p;
    		*retLines = nLines;
    		*retLineStart = lineStart;
    		*retLineEnd = foundBreak ? b : p;
    		return;
    	    }
    	    lineStart = newLineStart;
    	}
    }

    /* reached end of buffer before reaching pos or line target */
    *retPos = buf->length;
    *retLines = nLines;
    *retLineStart = lineStart;
    *retLineEnd = buf->length;
}


/*
** Finds both the end of the current line and the start of the next line.  Why?
** In continuous wrap mode, if you need to know both, figuring out one from the
** other can be expensive or error prone.  The problem comes when there's a
** trailing space or tab just before the end of the buffer.  To translate an
** end of line value to or from the next lines start value, you need to know
** whether the trailing space or tab is being used as a line break or just a
** normal character, and to find that out would otherwise require counting all
** the way back to the beginning of the line.
*/
static void findLineEnd(textDisp *textD, int startPos, int startPosIsLineStart,
    	int *lineEnd, int *nextLineStart)
{
    int retLines, retLineStart;
    
    /* if we're not wrapping use more efficient BufEndOfLine */
    if (!textD->continuousWrap) {
    	*lineEnd = BufEndOfLine(textD->buffer, startPos);
    	*nextLineStart = min(textD->buffer->length, *lineEnd + 1);
    	return;
    }
    
    /* use the wrapped line counter routine to count forward one line */
    wrappedLineCounter(textD, textD->buffer, startPos, textD->buffer->length,
    	    1, startPosIsLineStart, nextLineStart, &retLines,
    	    &retLineStart, lineEnd);
    return;
}

/*
** Line breaks in continuous wrap mode usually happen at newlines or
** whitespace.  This line-terminating character is not included in line
** width measurements and has a special status as a non-visible character.
** However, lines with no whitespace are wrapped without the benefit of a
** line terminating character, and this distinction causes endless trouble
** with all of the text display code which was originally written without
** continuous wrap mode and always expects to wrap at a newline character.
**
** Given the position of the end of the line, as returned by TextDEndOfLine
** or BufEndOfLine, this returns true if there is a line terminating
** character, and false if there's not.  On the last character in the
** buffer, this function can't tell for certain whether a trailing space was
** used as a wrap point, and just guesses that it wasn't.  So if an exact
** accounting is necessary, don't use this function.
*/ 
static int wrapUsesCharacter(textDisp *textD, int lineEndPos)
{
    char c;
    
    if (!textD->continuousWrap || lineEndPos == textD->buffer->length)
    	return True;
    
    c = BufGetCharacter(textD->buffer, lineEndPos);
    return c == '\n' || ((c == '\t' || c == ' ') &&
    	    lineEndPos + 1 != textD->buffer->length);
}

/*
** Decide whether the user needs (or may need) a horizontal scroll bar,
** and manage or unmanage the scroll bar widget accordingly.  The H.
** scroll bar is only hidden in continuous wrap mode when it's absolutely
** certain that the user will not need it: when wrapping is set
** to the window edge, or when the wrap margin is strictly less than
** the longest possible line.
*/
static void hideOrShowHScrollBar(textDisp *textD)
{
    if (textD->continuousWrap && (textD->wrapMargin == 0 || textD->wrapMargin *
    	    textD->fontStruct->max_bounds.width < textD->width))
    	XtUnmanageChild(textD->hScrollBar);
    else
    	XtManageChild(textD->hScrollBar);
}

/*
** Return true if the selection "sel" is rectangular, and touches a
** buffer position withing "rangeStart" to "rangeEnd"
*/
static int rangeTouchesRectSel(selection *sel, int rangeStart, int rangeEnd)
{
    return sel->selected && sel->rectangular && sel->end >= rangeStart &&
    	    sel->start <= rangeEnd;
}

/*
** Extend the range of a redraw request (from *start to *end) with additional
** redraw requests resulting from changes to the attached style buffer (which
** contains auxiliary information for coloring or styling text).
*/
static void extendRangeForStyleMods(textBuffer *styleBuf, int *start, int *end)
{
    selection *sel = &styleBuf->primary;
    
    /* The peculiar protocol used here is that modifications to the style
       buffer are marked by selecting them with the buffer's primary selection.
       The style buffer is usually modified in response to a modify callback on
       the text buffer BEFORE textDisp.c's modify callback, so that it can keep
       the style buffer in step with the text buffer.  The style-update
       callback can't just call for a redraw, because textDisp hasn't processed
       the original text changes yet.  Anyhow, to minimize redrawing and to
       avoid the complexity of scheduling redraws later, this simple protocol
       tells the text display's buffer modify callback to extend it's redraw
       range to show the text color/and font changes as well. */
    if (sel->selected) {
	*start = min(sel->start, *start);
	*end = max(sel->end, *end);
    }
}

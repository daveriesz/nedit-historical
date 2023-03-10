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
#define FILL_MASK 1
#define SECONDARY_MASK 2
#define PRIMARY_MASK 4
#define HIGHLIGHT_MASK 8

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
static void drawString(textDisp *textD, int style, int x, int y,
	char *string, int nChars);
static void drawCursor(textDisp *textD, int x, int y);
static int styleOfPos(textBuffer *buf, int lineStartPos, int lineLen,
	int lineIndex, int dispIndex);
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
static GC allocateGC(Widget w, unsigned long valueMask,
	unsigned long foreground, unsigned long background, Font font,
	unsigned long dynamicMask, unsigned long dontCareMask);
static void releaseGC(Widget w, GC gc);
static void resetClipRectangles(textDisp *textD);
static int visLineLength(textDisp *textD, int visLineNum);

textDisp *TextDCreate(Widget widget, Widget hScrollBar, Widget vScrollBar,
	Position left, Position top, Position width, Position height,
	textBuffer *buffer, XFontStruct *fontStruct, Pixel bgPixel,
	Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
	Pixel highlightFGPixel, Pixel highlightBGPixel, Pixel cursorFGPixel)
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
    textD->gc = allocateGC(widget, GCFont | GCForeground | GCBackground,
    	    fgPixel, bgPixel, fontStruct->fid, GCClipMask, GCArcMode); 
    textD->selectGC = allocateGC(widget, GCFont | GCForeground | GCBackground,
    	    selectFGPixel, selectBGPixel, fontStruct->fid, GCClipMask,
    	    GCArcMode);
    textD->selectBGGC = allocateGC(widget, GCForeground, selectBGPixel, 0,
    	    fontStruct->fid, GCClipMask, GCArcMode);
    textD->highlightGC = allocateGC(widget, GCFont|GCForeground|GCBackground,
    	    highlightFGPixel, highlightBGPixel, fontStruct->fid, GCClipMask,
    	    GCArcMode);
    textD->highlightBGGC = allocateGC(widget, GCForeground, highlightBGPixel, 0,
    	    fontStruct->fid, GCClipMask, GCArcMode);
    textD->nVisibleLines =
    	    (height - 1) / (fontStruct->ascent + fontStruct->descent) + 1;
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

    return textD;
}

/*
** Free a text display and release its associated memory.  Note, the text
** BUFFER that the text display displays is a separate entity and is not
** freed
*/
void TextDFree(textDisp *textD)
{
    BufRemoveModifyCB(textD->buffer, bufModifiedCB, textD);
    releaseGC(textD->w, textD->gc);
    releaseGC(textD->w, textD->selectGC);
    releaseGC(textD->w, textD->highlightGC);
    releaseGC(textD->w, textD->selectBGGC);
    releaseGC(textD->w, textD->highlightBGGC);
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
** Change the size of the displayed text are
*/
void TextDResize(textDisp *textD, int width, int height)
{
    int oldVisibleLines = textD->nVisibleLines;
    XFontStruct *fs = textD->fontStruct;
    int canRedraw = XtWindow(textD->w) != 0;
    int i, newVisibleLines = height / (fs->ascent + fs->descent);
    int *oldLineStarts;
    
    /* reallocate the line starts array */
    if (oldVisibleLines > newVisibleLines) {
	textD->nVisibleLines = newVisibleLines;
	calcLastChar(textD);
    } else if (oldVisibleLines < newVisibleLines) {
	oldLineStarts = textD->lineStarts;
	textD->lineStarts = (int *)XtMalloc(sizeof(int) * newVisibleLines);
	for (i=0; i<oldVisibleLines; i++)
    	    textD->lineStarts[i] = oldLineStarts[i];
	textD->nVisibleLines = newVisibleLines;
	calcLineStarts(textD, oldVisibleLines, newVisibleLines);
	XtFree((char *)oldLineStarts);
    	calcLastChar(textD);
    }
    
    /* update width and height, height to an exact multiple of font height */
    textD->width = width;
    textD->height = height - height % (fs->ascent + fs->descent);
    
    /* if the window became shorter, there may be partially drawn
       text left at the bottom edge, which must be cleaned up */
    if (canRedraw && oldVisibleLines>newVisibleLines && textD->height!=height)
    	XClearArea(XtDisplay(textD->w), XtWindow(textD->w), textD->left,
    		textD->top + textD->height,  textD->width,
    		height - textD->height, False);
    
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
    if (updateHScrollBarRange(textD) && canRedraw)
    	TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    		textD->height);

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
    fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
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
    	indent += BufCharWidth(*c, indent, buf->tabDist);
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
    	indent += BufCharWidth(ch, indent, buf->tabDist);
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
    int charIndex, lineStartPos, fontHeight;
    int visLineNum, charLen, outIndex, xStep;
    char *lineStr, expandedChar[MAX_EXP_CHAR_LEN];
    
    /* If position is not displayed, return false */
    if (pos < textD->firstChar ||
    	    (pos > textD->lastChar && !emptyLinesVisible(textD)))
    	return False;
    	
    /* Calculate y coordinate */
    if (!posToVisibleLineNum(textD, pos, &visLineNum))
    	return False;
    fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
    *y = textD->top + visLineNum*fontHeight + fontHeight/2;
    
    /* Get the text, length, and  buffer position of the line. If the position
       is beyond the end of the buffer and should be at the first position on
       the first empty line, don't try to get or scan the text  */
    lineStartPos = textD->lineStarts[visLineNum];
    if (lineStartPos == -1) {
    	*x = textD->left - textD->horizOffset;
    	return True;
    }
    lineStr = BufGetRange(textD->buffer, lineStartPos, lineStartPos +
    	    visLineLength(textD, visLineNum));
    
    /* Step through character positions from the beginning of the line
       to "pos" to calculate the x coordinate */
    xStep = textD->left - textD->horizOffset;
    outIndex = 0;
    for(charIndex=0; charIndex<pos-lineStartPos; charIndex++) {
    	charLen = BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
    		textD->buffer->tabDist);
    	xStep += XTextWidth(textD->fontStruct, expandedChar, charLen);
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
int TextDPosToLineNum(textDisp *textD, int pos, int *lineNum)
{
    int retVal;
    
    retVal = posToVisibleLineNum(textD, pos, lineNum);
    if (retVal)
    	*lineNum += textD->topLineNum;
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
    return inSelection(&buf->primary, pos, BufStartOfLine(buf, pos), column);
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
    
    hOffset = textD->horizOffset;
    topLine = textD->topLineNum;
    
    /* Find the new top line number */
    if (textD->cursorPos < textD->firstChar)
    	topLine -= BufCountLines(textD->buffer, textD->cursorPos,
    		textD->firstChar);
    else if (textD->cursorPos > textD->lastChar && !emptyLinesVisible(textD))
    	topLine += BufCountLines(textD->buffer, textD->lastChar,
    		textD->cursorPos);
    if (topLine < 1) {/*... remove this in final */
    	fprintf(stderr, "internal consistency check tl1 failed\n");
    	topLine = 1;
    }
    
    /* Find the new setting for horizontal offset (this is a bit ungraceful).
       If the line is visible, just use TextDPositionToXY to get the position
       to scroll to, otherwise, do the vertical scrolling first, then the
       horizontal */
    if (!TextDPositionToXY(textD, textD->cursorPos, &x, &y)) {
    	setScroll(textD, topLine, hOffset, True, True);
    	if (!TextDPositionToXY(textD, textD->cursorPos, &x, &y))
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
    int lineStartPos, column, prevLineStartPos;
    
    lineStartPos = BufStartOfLine(textD->buffer, textD->cursorPos);
    if (lineStartPos == 0)
    	return False;
    column = textD->cursorPreferredCol >= 0 ? textD->cursorPreferredCol :
    	    BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
    prevLineStartPos = BufCountBackwardNLines(textD->buffer, lineStartPos, 1);
    TextDSetInsertPosition(textD,
    	    BufCountForwardDispChars(textD->buffer, prevLineStartPos, column));
    textD->cursorPreferredCol = column;
    return True;
}
int TextDMoveDown(textDisp *textD)
{
    int lineStartPos, column, nextLineStartPos;
    
    if (textD->cursorPos == textD->buffer->length)
    	return False;
    lineStartPos = BufStartOfLine(textD->buffer, textD->cursorPos);
    column = textD->cursorPreferredCol >= 0 ? textD->cursorPreferredCol :
    	    BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
    nextLineStartPos = BufCountForwardNLines(textD->buffer, lineStartPos, 1);
    TextDSetInsertPosition(textD,
    	    BufCountForwardDispChars(textD->buffer, nextLineStartPos, column));
    textD->cursorPreferredCol = column;
    return True;
}

/*
** Callback attached to the text buffer to receive modification information
*/
static void bufModifiedCB(int pos, int nInserted, int nDeleted,
	int nRestyled, char *deletedText, void *cbArg)
{
    int linesInserted, linesDeleted, startDispPos, endDispPos;
    textDisp *textD = (textDisp *)cbArg;
    int scrolled, origCursorPos = textD->cursorPos;
 
    /* buffer modification cancels vertical cursor motion column */
    if (nInserted != 0 || nDeleted != 0)
    	textD->cursorPreferredCol = -1;
    
    /* Count the number of lines & chars inserted and deleted */
    linesInserted = nInserted == 0 ? 0 :
    	    BufCountLines(textD->buffer, pos, pos + nInserted);
    linesDeleted = nDeleted == 0 ? 0 : countLines(deletedText);

    /* Update the line starts and topLineNum */
    if (nInserted != 0 || nDeleted != 0)
	updateLineStarts(textD, pos, nInserted, nDeleted, linesInserted,
    		linesDeleted, &scrolled);
    else
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
    if (textD->cursorToHint != NO_HINT)
    	textD->cursorPos = textD->cursorToHint;
    else if (textD->cursorPos > pos) {
    	if (textD->cursorPos < pos + nDeleted)
    	    textD->cursorPos = pos;
    	else
    	    textD->cursorPos += nInserted - nDeleted;
    }
    
    /* If the changes caused scrolling, re-paint everything beyond pos,
       otherwise, redisplay to the end of the changed region.  Also if
       the cursor position moved, be sure that the redisplay range covers
       the old cursor position so the old cursor gets erased, and erase
       the bits of the cursor which extend beyond the left and right
       edges of the text */
    if (scrolled) {
    	blankCursorProtrusions(textD);
    	TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    		textD->height);
    	return;
    }
    if (origCursorPos == pos && textD->cursorPos != pos)
    	startDispPos = min(pos, origCursorPos-1);
    else
    	startDispPos = pos;
    if (linesInserted == linesDeleted) {
        if (nInserted == 0 && nDeleted == 0)
            endDispPos = nRestyled;
        else {
    	    endDispPos = BufEndOfLine(textD->buffer, pos + nInserted) + 1;
    	    if (origCursorPos>=startDispPos && origCursorPos<=endDispPos)
    	    	blankCursorProtrusions(textD);
    	}
    } else {
    	endDispPos = textD->lastChar + 1;
    	if (origCursorPos >= pos)
    	    blankCursorProtrusions(textD);
    }
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
    int i, x, y, startX, charIndex, lineStartPos, lineLen, fontHeight;
    int maxCharWidth, charWidth, startIndex, charStyle, style;
    int charLen, outStartIndex, outIndex, cursorX, hasCursor = False;
    int cursorPos = textD->cursorPos;
    char expandedChar[MAX_EXP_CHAR_LEN], outStr[MAX_DISP_LINE_LEN];
    char *lineStr, *outPtr;
    
    /* If line is not displayed, skip it */
    if (visLineNum < 0 || visLineNum >= textD->nVisibleLines)
    	return;

    /* Calculate y coordinate of the string to draw */
    fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
    y = textD->top + visLineNum * fontHeight;

    /* Get the text, length, and  buffer position of the line to display */
    lineStartPos = textD->lineStarts[visLineNum];
    if (lineStartPos == -1) {
    	lineLen = 0;
    	lineStr = NULL;
    } else {
	lineLen = visLineLength(textD, visLineNum);
	lineStr = BufGetRange(textD->buffer, lineStartPos, lineStartPos +
		lineLen);
    }
    
    /* Space beyond the end of the line is still counted in units of characters
       of the maximum character width in the font (this is done mostly because
       style changes based on character position can still occur in this
       region due to rectangular selections).  maxCharWidth must be non-zero
       to prevent a potential infinite loop if x does not advance */
    maxCharWidth = textD->fontStruct->max_bounds.width;
    if (maxCharWidth <= 0) {
    	fprintf(stderr, "Internal Error, bad font measurement\n");
    	XtFree(lineStr);
    	return;
    }
    
    /* Shrink the clipping range to the active display area */
    leftClip = max(textD->left, leftClip);
    rightClip = min(rightClip, textD->left + textD->width); 

    /* Step through character positions from the beginning of the line (even if
       that's off the left edge of the displayed area) to find the first
       character position that's not clipped, and the x coordinate for drawing
       that character */
    x = textD->left - textD->horizOffset;
    outIndex = 0;
    for(charIndex=0; ; charIndex++) {
    	charLen = charIndex >= lineLen ? 1 :
    	    	BufExpandCharacter(lineStr[charIndex], outIndex,
    	    	expandedChar, textD->buffer->tabDist);
    	charWidth = charIndex >= lineLen ? maxCharWidth :
    	    	XTextWidth(textD->fontStruct, expandedChar, charLen);
    	if (x + charWidth >= leftClip && charIndex >= leftCharIndex) {
    	    startIndex = charIndex;
    	    outStartIndex = outIndex;
    	    startX = x;
    	    style = styleOfPos(textD->buffer, lineStartPos, lineLen,
    	    	    charIndex, outIndex);
    	    break;
    	}
    	x += charWidth;
    	outIndex += charLen;
    }

    /* Scan character positions from the beginning of the clipping range, and
       draw parts whenever the style changes (also note if the cursor is on
       this line, and where it should be drawn) */
    outPtr = outStr;
    outIndex = outStartIndex;
    x = startX;
    for(charIndex=startIndex; charIndex<rightCharIndex; charIndex++) {
    	if (charIndex <= lineLen && (lineStartPos+charIndex == cursorPos)) {
    	    hasCursor = True;
    	    cursorX = x - 1;
    	}
     	charLen = charIndex >= lineLen ? 1 :
     		BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
    		textD->buffer->tabDist);
   	charStyle = styleOfPos(textD->buffer, lineStartPos, lineLen,
    		charIndex, outIndex);
   	for (i=0; i<charLen; i++) {
   	    if (i != 0 && charIndex < lineLen && lineStr[charIndex] == '\t')
   		charStyle = styleOfPos(textD->buffer, lineStartPos, lineLen,
    			charIndex, outIndex);
     	    if (charStyle != style) {
    		drawString(textD, style, startX, y, outStr, outPtr - outStr);
    		outPtr = outStr;
    		startX = x;
    		style = charStyle;
    	    }
    	    if (charIndex < lineLen) {
    		*outPtr = expandedChar[i];
    		charWidth = XTextWidth(textD->fontStruct, &expandedChar[i], 1);
    	    } else
    		charWidth = maxCharWidth;
    	    outPtr++;
    	    x += charWidth;
    	    outIndex++;
	}
    	if (outPtr-outStr+MAX_EXP_CHAR_LEN>=MAX_DISP_LINE_LEN || x>=rightClip)
    	    break;
    }
    
    /* Draw the remaining style segment */
    drawString(textD, style, startX, y, outStr, outPtr - outStr);
    
    /* Draw the cursor if part of it appeared on the redisplayed part of
       this line.  Also check for the cases which are not caught as the
       line is scanned above: when the cursor appears at the very end
       of the redisplayed section. */
    if (textD->cursorOn) {
	if (hasCursor)
    	    drawCursor(textD, cursorX, y);
	else if (charIndex<lineLen && (lineStartPos+charIndex+1 == cursorPos)
	    	&& x == rightClip)
    	    drawCursor(textD, x - 1, y);
    }
    
    if (lineStr != NULL)
    	XtFree(lineStr);
}

/*
** Draw a string or blank area according to parameter "style", using the
** appropriate colors and drawing method for that style, with top left
** corner at x, y.  If style says to draw text, use "string" as source of
** characters, and draw "nChars", if style is FILL, erase
** rectangle where text would have drawn, using the maximum width of
** characters from the textD font structure.
*/
static void drawString(textDisp *textD, int style, int x, int y,
	char *string, int nChars)
{
    GC gc;
    XFontStruct *fs = textD->fontStruct;
    
    if (nChars == 0 || XtWindow(textD->w) == 0)
    	return;
    
    if (style & HIGHLIGHT_MASK && style & FILL_MASK)
    	gc = textD->highlightBGGC;
    else if (style & HIGHLIGHT_MASK)
    	gc = textD->highlightGC;
    else if (style & PRIMARY_MASK && style & FILL_MASK)
    	gc = textD->selectBGGC;
    else if (style & PRIMARY_MASK)
    	gc = textD->selectGC;
    else
    	gc = textD->gc;
    
    if (style & FILL_MASK) {
    	if (style & PRIMARY_MASK || style & HIGHLIGHT_MASK)
    	    XFillRectangle(XtDisplay(textD->w), XtWindow(textD->w), gc, x, y,
    		nChars * fs->max_bounds.width, fs->ascent + fs->descent);
    	else /* avoid using an additional GC just for background color */
    	    XClearArea(XtDisplay(textD->w), XtWindow(textD->w), x, y,
    		nChars * fs->max_bounds.width, fs->ascent + fs->descent, False);
    } else {
    	XDrawImageString(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
    		y + fs->ascent, string, nChars);
    	if (style & SECONDARY_MASK)
    	    XDrawLine(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
    	    	    y + fs->ascent, x + XTextWidth(fs, string, nChars) - 1,
    	    	    y + fs->ascent);
    }
}

/*
** Draw a cursor with top center at x, y.
*/
static void drawCursor(textDisp *textD, int x, int y)
{
    XSegment segs[5];
    int left, right, cursorWidth, midY;
    int fontWidth = textD->fontStruct->max_bounds.width, nSegs = 0;  
    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
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
** Note that style is a somewhat incorrect name, drawing method would
** be more appropriate.
*/
static int styleOfPos(textBuffer *buf, int lineStartPos, int lineLen,
	int lineIndex, int dispIndex)
{
    int pos, style = 0;
    
    if (lineStartPos == -1 || buf == NULL)
    	return FILL_MASK;
    
    if (lineIndex >= lineLen)
   	style |= FILL_MASK;
   
    pos = lineStartPos + min(lineIndex, lineLen);
    if (inSelection(&buf->primary, pos, lineStartPos, dispIndex))
    	style |= PRIMARY_MASK;
    if (inSelection(&buf->highlight, pos, lineStartPos, dispIndex))
    	style |= HIGHLIGHT_MASK;
    if (inSelection(&buf->secondary, pos, lineStartPos, dispIndex))
    	style |= SECONDARY_MASK;
    return style;
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
    int charWidth, charLen, visLineNum, xStep, outIndex;
    char *lineStr, expandedChar[MAX_EXP_CHAR_LEN];

    /* Find the visible line number corresponding to the y coordinate */
    fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
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
    		textD->buffer->tabDist);
    	charWidth = XTextWidth(textD->fontStruct, expandedChar, charLen);
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
    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
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
    	textD->firstChar = BufCountForwardNLines(buf, 0, newTopLineNum-1);
    	/* printf("counting forward %d lines from start\n", newTopLineNum-1);*/
    } else if (newTopLineNum < oldTopLineNum) {
    	textD->firstChar = BufCountBackwardNLines(buf, textD->firstChar,
    		-lineDelta);
    	/* printf("counting backward %d lines from firstChar\n", -lineDelta);*/
    } else if (newTopLineNum < lastLineNum) {
    	textD->firstChar = lineStarts[newTopLineNum - oldTopLineNum];
    	/* printf("taking new start from lineStarts[%d]\n",
    		newTopLineNum - oldTopLineNum); */
    } else if (newTopLineNum-lastLineNum < textD->nBufferLines-newTopLineNum) {
    	textD->firstChar = BufCountForwardNLines(buf, lineStarts[nVisLines-1],
    		newTopLineNum - lastLineNum);
    	/* printf("counting forward %d lines from start of last line\n",
    		newTopLineNum - lastLineNum); */
    } else {
    	textD->firstChar = BufCountBackwardNLines(buf, buf->length,
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
    	    textD->firstChar = BufCountBackwardNLines(textD->buffer,
    	    	    lineStarts[lineOfEnd] + charDelta, lineOfEnd);
    	/* Otherwise anchor on original line number and recount everything */
    	} else {
    	    if (textD->topLineNum > textD->nBufferLines + lineDelta) {
    	    	textD->topLineNum = 1;
    	    	textD->firstChar = 0;
    	    } else
    		textD->firstChar = BufCountForwardNLines(textD->buffer, 0,
    	    		textD->topLineNum - 1);
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
    int startPos;
    int line, nVis = textD->nVisibleLines;
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
    	startPos = BufEndOfLine(textD->buffer, startPos) + 1;
    	if (startPos > textD->buffer->length)
    	    break;
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
    	    BufEndOfLine(textD->buffer, textD->lineStarts[i]);
}

static void setScroll(textDisp *textD, int topLineNum, int horizOffset,
	int updateVScrollBar, int updateHScrollBar)
{
    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
    int origHOffset = textD->horizOffset;
    int lineDelta = textD->topLineNum - topLineNum;
    int xOffset, yOffset, srcX, srcY, dstX, dstY, width, height;
    
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
    	    abs(xOffset) > textD->width || abs(yOffset) > textD->height) {
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
	height = textD->height - abs(yOffset);
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
    
    if (textD->hScrollBar == NULL)
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
    int i, width = 0, len, lineLen = visLineLength(textD, visLineNum);
    int charCount = 0, lineStartPos = textD->lineStarts[visLineNum];
    char expandedChar[MAX_EXP_CHAR_LEN];
    
    for (i=0; i<lineLen; i++) {
    	len = BufGetExpandedChar(textD->buffer, lineStartPos + i,
    		charCount, expandedChar);
    	width += XTextWidth(textD->fontStruct, expandedChar, len);
    	charCount += len;
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
    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
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
    
    clipRect.x = textD->left;
    clipRect.y = textD->top;
    clipRect.width = textD->width;
    clipRect.height = textD->height;
    
    XSetClipRectangles(XtDisplay(textD->w), textD->gc, 0, 0,
    	    &clipRect, 1, Unsorted);
    XSetClipRectangles(XtDisplay(textD->w), textD->selectGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(XtDisplay(textD->w), textD->highlightGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(XtDisplay(textD->w), textD->selectBGGC, 0, 0,
            &clipRect, 1, Unsorted);
    XSetClipRectangles(XtDisplay(textD->w), textD->highlightBGGC, 0, 0,
            &clipRect, 1, Unsorted);
} 

/*
** Return the length of a line by examining entries in the line starts array
** rather than by scanning for newlines
*/
static int visLineLength(textDisp *textD, int visLineNum)
{
    int nextLineStart, lineStartPos = textD->lineStarts[visLineNum];
    
    if (lineStartPos == -1)
    	return 0;
    if (visLineNum+1 < textD->nVisibleLines) {
	nextLineStart = textD->lineStarts[visLineNum+1];
	return (nextLineStart == -1 ? textD->lastChar : nextLineStart-1)
	    	- lineStartPos;
    }
    return textD->lastChar - lineStartPos;
}

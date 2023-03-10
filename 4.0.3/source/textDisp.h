/*******************************************************************************
*									       *
* textDisp.h - Display text from a text buffer				       *
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
enum cursorStyles {NORMAL_CURSOR, CARET_CURSOR, DIM_CURSOR, BLOCK_CURSOR,
	HEAVY_CURSOR};

#define NO_HINT -1

typedef struct _textDisp {
    Widget w;
    int top, left, width, height;
    int cursorPos;
    int cursorOn;
    int cursorX, cursorY;		/* X, Y pos. of cursor for blanking */
    int cursorToHint;			/* Tells the buffer modified callback
    					   where to move the cursor, to reduce
    					   the number of redraw calls */
    int cursorStyle;			/* One of enum cursorStyles above */
    int cursorPreferredCol;		/* Column for vert. cursor movement */
    int nVisibleLines;			/* # of visible (displayed) lines */
    int nBufferLines;			/* # of newlines in the buffer */
    textBuffer *buffer;
    int firstChar, lastChar;		/* Buffer positions of first and last
    					   displayed character (lastChar points
    					   either to a newline or one character
    					   beyond the end of the buffer) */
    int *lineStarts;
    int topLineNum;			/* Line number of top displayed line
    					   of file (first line of file is 1) */
    int horizOffset;			/* Horizontal scroll pos. in pixels */
    int visibility;			/* Window visibility (see XVisibility
    					   event) */
    XFontStruct *fontStruct;
    Widget hScrollBar, vScrollBar;
    GC gc, selectGC, highlightGC;	/* GCs for drawing text */
    GC selectBGGC, highlightBGGC;	/* GCs for erasing text */
    GC cursorFGGC;			/* GC for drawing the cursor */
} textDisp;

textDisp *TextDCreate(Widget widget, Widget hScrollBar, Widget vScrollBar,
	Position left, Position top, Position width, Position height,
	textBuffer *buffer, XFontStruct *fontStruct, Pixel bgPixel,
	Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
	Pixel highlightFGPixel, Pixel highlightBGPixel, Pixel cursorFGPixel);
void TextDFree(textDisp *textD);
void TextDSetBuffer(textDisp *textD, textBuffer *buffer);
textBuffer *TextDGetBuffer(textDisp *textD);
void TextDResize(textDisp *textD, int width, int height);
void TextDRedisplayRect(textDisp *textD, int left, int top, int width,
	int height);
void TextDRedisplayRange(textDisp *textD, int start, int end);
void TextDSetScroll(textDisp *textD, int topLineNum, int horizOffset);
void TextDGetScroll(textDisp *textD, int *topLineNum, int *horizOffset);
void TextDInsert(textDisp *textD, char *text);
void TextDOverstrike(textDisp *textD, char *text);
void TextDSetInsertPosition(textDisp *textD, int newPos);
int TextDGetInsertPosition(textDisp *textD);
int TextDXYToPosition(textDisp *textD, int x, int y);
void TextDXYToUnconstrainedPosition(textDisp *textD, int x, int y, int *row,
	int *column);
int TextDPositionToXY(textDisp *textD, int pos, int *x, int *y);
int TextDPosToLineNum(textDisp *textD, int pos, int *lineNum);
int TextDInSelection(textDisp *textD, int x, int y);
void TextDMakeInsertPosVisible(textDisp *textD);
int TextDMoveRight(textDisp *textD);
int TextDMoveLeft(textDisp *textD);
int TextDMoveUp(textDisp *textD);
int TextDMoveDown(textDisp *textD);
void TextDBlankCursor(textDisp *textD);
void TextDUnblankCursor(textDisp *textD);
void TextDSetCursorStyle(textDisp *textD, int style);

/*******************************************************************************
*									       *
* shift.c -- Nirvana Editor built-in filter commands			       *
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
* June 18, 1991								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)shift.c	1.10     8/3/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <limits.h>
#include <ctype.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "../util/DialogF.h"
#include "nedit.h"
#include "shift.h"
#include "window.h"

static int findSelectedLines(Widget widget, XmTextPosition *startPos,
	XmTextPosition *endPos);
static char *shiftText(char *text, int direction, int tabsAllowed, int tabDist,
	int byTabs, int *newLen);
static void shiftLineRight(char *line, int tabsAllowed, int tabDist);
static void shiftLineRightByTab(char *line);
static void shiftLineLeft(char *line, int tabDist, int nChars);
static int findLeftMargin(char *text, int tabDist);
static char *fillParagraph(char *text, int leftMargin, int rightMargin,
	int tabDist);
static int atTabStop(int pos, int tabDist);
static int nextTab(int pos, int tabDist);
static int countLines(char *text);
static XmTextPosition findParagraphStart(Widget textW, XmTextPosition pos);
static XmTextPosition findParagraphEnd(Widget textW, XmTextPosition pos);

void ShiftSelection(WindowInfo *window, int direction, int byTab, Time time)
{
    Widget tw = window->lastFocus;
    XmTextPosition startPos, endPos;
    int shiftedLen, newEndPos;
    char *text, *shiftedText;

    if (!findSelectedLines(tw, &startPos, &endPos))
    	return;
	
    text = GetTextRange(tw, startPos, endPos);
    shiftedText = shiftText(text, direction, TRUE, window->tabDist, byTab,
    	    &shiftedLen);
    XtFree(text);
    XmTextReplace(tw, startPos, endPos, shiftedText);
    XtFree(shiftedText);
    
    newEndPos = startPos + shiftedLen;
    XmTextSetSelection(tw, startPos, newEndPos, time);
}

void UpcaseSelection(WindowInfo *window)
{
    char *text, *c;
    XmTextPosition left, right;
    
    /* Get the selection and the range in character positions that it
       occupies.  use character before cursor if no selection */
    if (!XmTextGetSelectionPosition(window->textArea, &left, &right) ||
    		left == right) {
    	right = XmTextGetInsertionPosition(window->textArea);
    	if (right == 0) {
    	    XBell(TheDisplay, 100);
    	    return;
	}
    	left = right - 1;
    }
    text = GetTextRange(window->textArea, left, right);
    
    /* upcase the string */
    for (c=text; *c!='\0'; c++)
    	*c = toupper(*c);
    	
    /* Replace the text in the window */
    XmTextReplace(window->textArea, left, right, text);
    XtFree(text);
}

void DowncaseSelection(WindowInfo *window)
{
    char *text, *c;
    XmTextPosition left, right;
    
    /* Get the selection and the range in character positions that it
       occupies.  use character before cursor if no selection */
    if (!XmTextGetSelectionPosition(window->textArea, &left, &right) ||
    		left == right) {
    	right = XmTextGetInsertionPosition(window->textArea);
    	if (right == 0) {
    	    XBell(TheDisplay, 100);
    	    return;
	}
    	left = right - 1;
    }
    text = GetTextRange(window->textArea, left, right);
    
    /* downcase the string */
    for (c=text; *c!='\0'; c++)
    	*c = tolower(*c);
    	
    /* Replace the text in the window */
    XmTextReplace(window->textArea, left, right, text);
    XtFree(text);
}

void FillSelection(WindowInfo *window)
{
    char *text, *filledText;
    XmTextPosition left, right, insertPos;
    short nCols;
    int leftMargin;
    
    /* Find the range of characters (left and right) to fill.  If there is a
       selection, use it but extend it to encompass whole lines.  If there is
       no selection, find the paragraph containing the insertion cursor */
    if (XmTextGetSelectionPosition(window->textArea, &left, &right) &&
    	    left != right) {
	if (!findSelectedLines(window->lastFocus, &left, &right))
    	    return;
	XmTextSetSelection(window->lastFocus, left, right, CurrentTime);
    } else {
	insertPos = XmTextGetInsertionPosition(window->textArea);
	left = findParagraphStart(window->textArea, insertPos);
	right = findParagraphEnd(window->textArea, insertPos);
	if (left == right) {
    	    XBell(TheDisplay, 100);
    	    return;
	}
	    
    }
    
    /* Make a copy of the text within the range */
    text = GetTextRange(window->textArea, left, right);

    /* Find the right margin by getting the width in characters of the window */
    XtVaGetValues(window->textArea, XmNcolumns, &nCols, NULL);
    
    /* Find the left margin by scanning the text */
    leftMargin = findLeftMargin(text, window->tabDist);
    
    /* Fill the text */
    filledText = fillParagraph(text, leftMargin, nCols, window->tabDist);
    	
    /* Replace the text in the window */
    XmTextReplace(window->textArea, left, right, filledText);
    XtFree(text);
    XtFree(filledText);
}

/*
** Extend the selection to encompass complete lines.  If there is no selection,
** return the line that the insert position is on.  Will return False
** if it can't find a newline within MAX_LINE_LENGTH of the ends of the
** selection
*/
static int findSelectedLines(Widget widget, XmTextPosition *startPos,
	XmTextPosition *endPos)
{
    int selStart, selEnd;
    
    /* get selection, if no text selected, use current insert position */
    if (!GetSelection(widget, &selStart, &selEnd)) {
    	GET_ONE_RSRC(widget, XmNcursorPosition, &selStart);
	selEnd = selStart;
    }
    *startPos = selStart;
    *endPos = selEnd;
    return ExtendToWholeLines(widget, startPos, endPos);
}
    
/*
** Extend a range of text positions to encompass complete lines.  Will return
** FALSE  if it can't find a newline within MAX_LINE_LENGTH before *startPos
** or after *endPos.
*/
int ExtendToWholeLines(Widget widget, XmTextPosition *startPos,
	XmTextPosition *endPos)
{
    int selStart = *startPos, selEnd = *endPos;
    int rangeStart, rangeEnd, searchLen, searchPos;
    char *searchText;

    /* if the selection is empty, extend it so the search code below
       will find a whole line rather than a single newline character */
    if (selEnd == selStart)
    	selEnd++;

    /* get a copy of the text at least one line further in each direction
       from the selected text or the insertion point */
    rangeStart = selStart - MAX_LINE_LENGTH;
    if (rangeStart < 0) rangeStart = 0;
    rangeEnd = selEnd + MAX_LINE_LENGTH;
    if (rangeEnd > TextLength(widget)) rangeEnd = TextLength(widget);
    searchText = GetTextRange(widget, rangeStart, rangeEnd);
    searchLen = rangeEnd - rangeStart;
    
    /* search backward from the start of the selection for a newline */
    for (searchPos=selStart-rangeStart-1; searchPos>=0; searchPos--) {
	if (searchText[searchPos] == '\n') {
	    *startPos = searchPos + rangeStart + 1;
	    break;
	}
    }
    if (searchPos < 0) {
    	if (rangeStart == 0) {
	    /* didn't find a newline, because this is start of text */
	    *startPos = 0;
	} else {
	    DialogF(DF_ERR, widget, 1, "Line too long", "OK");
	    XtFree(searchText);
	    return FALSE;
	}
    }
    
    /* search forward from the end of the selection for a newline */
    for (searchPos=selEnd-rangeStart-1; searchPos<=searchLen; searchPos++) {
	if (searchText[searchPos] == '\n') {
	    *endPos = searchPos + rangeStart + 1;
	    break;
	}
    }
    if (searchPos > searchLen) {
    	if (rangeEnd == TextLength(widget)) {
	    /* didn't find a newline, because this is the end of the text */
	    *endPos = rangeEnd;
	} else {
	    DialogF(DF_ERR, widget, 1, "Line too long", "OK");
	    XtFree(searchText);
	    return FALSE;
	}
    }
    XtFree(searchText);
    return TRUE;
}

/*
** shift lines left and right in a multi-line text string.  Returns the
** shifted text in memory that must be freed by the caller with XtFree.
*/
static char *shiftText(char *text, int direction, int tabsAllowed, int tabDist,
	int byTabs, int *newLen)
{
    char line[MAX_LINE_LENGTH];
    char *shiftedText;
    char *textPtr, *linePtr, *shiftedPtr;
    int bufLen;
    
    /*
    ** Allocate memory for shifted string.  Shift left adds a maximum
    ** of 6 characters per line (remove one tab, add 7 spaces).  Shift right
    ** adds a maximum of 1 character per line.
    */
    if (direction == SHIFT_RIGHT)
        bufLen = strlen(text) + countLines(text) + 1;
    else
        bufLen = strlen(text) + countLines(text) * 6 + 1;
    shiftedText = (char *)XtMalloc(bufLen);
    
    /*
    ** break into lines and call shiftLine on each
    */
    linePtr = line;
    textPtr = text;
    shiftedPtr = shiftedText;
    while (TRUE) {
	if (*textPtr=='\n' || *textPtr=='\0') {
	    *linePtr = '\0';
	    if (direction == SHIFT_RIGHT && byTabs)
	        shiftLineRightByTab(line);
	    else if (direction == SHIFT_RIGHT)
		shiftLineRight(line, tabsAllowed, tabDist);
	    else
		shiftLineLeft(line, tabDist, byTabs ? tabDist : 1);
	    strcpy(shiftedPtr, line);
	    shiftedPtr += strlen(line);
	    if (*textPtr == '\0') {
	        /* terminate string & exit loop at end of text */
	    	*shiftedPtr = '\0';
		break;
	    } else {
	    	/* move the newline from text to shifted text */
		*shiftedPtr++ = *textPtr++;
	    }
	    /* start line over */
	    linePtr = line;
	} else {
	    /* build up line */
	    *linePtr++ = *textPtr++;
	}
    }
    *newLen = shiftedPtr - shiftedText;
    return shiftedText;
}

static void shiftLineRight(char *line, int tabsAllowed, int tabDist)
{
    char lineIn[MAX_LINE_LENGTH];
    char *lineInPtr, *lineOutPtr;
    int whiteWidth;
    
    strcpy(lineIn, line);
    lineInPtr = lineIn;
    lineOutPtr = line;
    whiteWidth = 0;
    while (TRUE) {
        if (*lineInPtr == '\0') {
	    /* nothing on line, wipe it out */
	    *line = '\0';
	    return;
        } else if (*lineInPtr == ' ') {
	    /* white space continues with tab, advance to next tab stop */
	    whiteWidth++;
	    *lineOutPtr++ = *lineInPtr++;
	} else if (*lineInPtr == '\t') {
	    /* white space continues with tab, advance to next tab stop */
	    whiteWidth = nextTab(whiteWidth, tabDist);
	    *lineOutPtr++ = *lineInPtr++;
	} else {
	    /* end of white space, add a space */
	    *lineOutPtr++ = ' ';
	    whiteWidth++;
	    /* if that puts us at a tab stop, change last 8 spaces into a tab */
	    if (tabsAllowed && atTabStop(whiteWidth, tabDist)) {
		lineOutPtr -= tabDist;
		*lineOutPtr++ = '\t';
	    }
	    /* move remainder of line */
    	    while (*lineInPtr!='\0')
		*lineOutPtr++ = *lineInPtr++;
	    *lineOutPtr = '\0';
	    return;
	}
    }
}

static void shiftLineRightByTab(char *line)
{
    char *linePtr;
    int length = strlen(line);
    
    /* don't shift blank lines (lines with only whitespace do get shifted) */
    if (*line == '\0')
    	return;
    
    /* just add a tab to the beginning of the line */
    for (linePtr = &line[length]; linePtr>=line; linePtr--)
    	*(linePtr+1) = *linePtr;
    *line = '\t';
}

static void shiftLineLeft(char *line, int tabDist, int nChars)
{
    char lineIn[MAX_LINE_LENGTH];
    int i, whiteWidth, lastWhiteWidth, whiteGoal;
    char *lineInPtr, *lineOutPtr;
    
    strcpy(lineIn, line);
    lineInPtr = lineIn;
    lineOutPtr = line;
    whiteWidth = 0;
    lastWhiteWidth = 0;
    while (TRUE) {
        if (*lineInPtr == '\0') {
	    /* nothing on line, wipe it out */
	    *line = '\0';
	    return;
        } else if (*lineInPtr == ' ') {
	    /* white space continues with space, advance one character */
	    whiteWidth++;
	    *lineOutPtr++ = *lineInPtr++;
	} else if (*lineInPtr == '\t') {
	    /* white space continues with tab, advance to next tab stop	    */
	    /* save the position, though, in case we need to remove the tab */
	    lastWhiteWidth = whiteWidth;
	    whiteWidth = nextTab(whiteWidth, tabDist);
	    *lineOutPtr++ = *lineInPtr++;
	} else {
	    /* end of white space, remove nChars characters */
	    for (i=1; i<=nChars; i++) {
		if (lineOutPtr != line) {
		    if (*(lineOutPtr-1) == ' ') {
			/* end of white space is a space, just remove it */
			lineOutPtr--;
		    } else {
	    		/* end of white space is a tab, remove it and add
	    		   back spaces */
			lineOutPtr--;
			whiteGoal = whiteWidth - i;
			whiteWidth = lastWhiteWidth;
			while (whiteWidth < whiteGoal) {
			    *lineOutPtr++ = ' ';
			    whiteWidth++;
			}
		    }
		}
	    }
	    /* move remainder of line */
    	    while (*lineInPtr!='\0')
		*lineOutPtr++ = *lineInPtr++;
	    /* add a null */
	    *lineOutPtr = '\0';
	    return;
	}
    }
}
       
static int atTabStop(int pos, int tabDist)
{
    return (pos%tabDist == 0);
}

static int nextTab(int pos, int tabDist)
{
    return (pos/tabDist)*tabDist + tabDist;
}

static int countLines(char *text)
{
    int count = 1;
    
    while(*text != '\0') {
    	if (*text++ == '\n') {
	    count++;
	}
    }
    return count;
}

/*
** Find the implied left margin of a text string (the number of columns to the
** first non-whitespace character on any line).
*/
static int findLeftMargin(char *text, int tabDist)
{
    char *c;
    int col = 0, leftMargin = INT_MAX;
    int inMargin = True;
    
    for (c=text; *c!='\0'; c++) {
    	if (*c == '\t') {
    	    col += tabDist - (col % tabDist);
    	} else if (*c == ' ') {
    	    col++;
    	} else if (*c == '\n') {
    	    col = 0;
    	    inMargin = True;
    	} else {
    	    /* non-whitespace */
    	    if (col < leftMargin && inMargin)
    	    	leftMargin = col;
    	    inMargin = False;
    	}
    }
    
    /* if no text is found, the leftMargin will never be set */
    if (leftMargin == INT_MAX)
    	return 0;
    
    return leftMargin;
}

/*
** Arrange text to fill the space between margins
*/
static char *fillParagraph(char *text, int leftMargin, int rightMargin,
	int tabDist)
{
    char *cleanedText, *outText, *indentString, *outPtr, *c, *b;
    int marginWidth = rightMargin - leftMargin;
    int i, col, cleanedLen, indentLen, nLines = 1;
    int inWhitespace, inMargin;
    
    /* remove leading spaces, convert newlines to spaces */
    cleanedText = XtMalloc(strlen(text)+1);
    outPtr = cleanedText;
    inMargin = True;
    for (c=text; *c!='\0'; c++) {
    	if (*c == '\t' || *c == ' ') {
    	    if (!inMargin)
    	    	*outPtr++ = *c;
    	} else if (*c == '\n') {
    	    if (inMargin) {
    	    	/* a newline before any text separates paragraphs, so leave
    	    	   it in, back up, and convert the previous space back to \n */
    	    	if (outPtr > cleanedText && *(outPtr-1) == ' ')
    	    	    *(outPtr-1) = '\n';
    	    	*outPtr++ = '\n';
    	    	nLines +=2;
    	    } else
    	    	*outPtr++ = ' ';
    	    inMargin = True;
    	} else {
    	    *outPtr++ = *c;
    	    inMargin = False;
    	}
    }
    cleanedLen = outPtr - cleanedText;
    *outPtr = '\0';
    
    /* Put back newlines breaking text at word boundaries within the margins.
       Algorithm: scan through characters, counting columns, and when the
       margin width is exceeded, search backward for beginning of the word
       and convert the last whitespace character into a newline */
    col = 0;
    for (c=cleanedText; *c!='\0'; c++) {
    	if (*c == '\t')
    	    col += tabDist - (col % tabDist);
    	else if (*c == '\n')
    	    col = 0;
    	else
    	    col++;
    	if (col > marginWidth) {
    	    inWhitespace = True;
    	    for (b=c; *b!='\n' && b>=cleanedText; b--) {
    	    	if (*b == '\t' || *b == ' ') {
    	    	    if (!inWhitespace) {
    	    		*b = '\n';
    	    		nLines++;
    	    		col = c - b;
    	    		break;
    	    	    }
    	    	} else {
    	    	    inWhitespace = False;
    	    	}
    	    }
    	}
    }
    nLines++;
    
    /* produce a string to prepend to lines to indent them to the left margin */
    indentString = XtMalloc(sizeof(char) * leftMargin / tabDist + 9);
    outPtr = indentString;
    for (i=0; i<leftMargin/tabDist; i++)
    	*outPtr++ = '\t';
    for (i=0; i<leftMargin%tabDist; i++)
    	*outPtr++ = ' ';
    indentLen = outPtr - indentString;
    *outPtr = '\0';
    
    /* allocate memory for the finished string */
    outText = XtMalloc(sizeof(char) * (cleanedLen + indentLen * nLines + 1));
    outPtr = outText;
    
    /* prepend the indent string to each line of the filled text */
    strncpy(outPtr, indentString, indentLen);
    outPtr += indentLen;
    for (c=cleanedText; *c!='\0'; c++) {
    	*outPtr++ = *c;
    	if (*c == '\n') {
    	    strncpy(outPtr, indentString, indentLen);
    	    outPtr += indentLen;
    	}
    }
    
    /* convert any trailing space to newline.  Add terminating null */
    if (*(outPtr-1) == ' ')
    	*(outPtr-1) = '\n';
    *outPtr = '\0';
    
    /* clean up, return result */
    XtFree(cleanedText);
    XtFree(indentString);
    return outText;
}

/*
** Find the boundaries of the paragraph containing pos
*/
static XmTextPosition findParagraphStart(Widget textW, XmTextPosition pos)
{
    XmTextPosition bufStartPos, p, paraStartPos, searchStartPos;
    char *buf, *c;
    int haveNewline = False;
    
    /*
    ** loop getting 255 byte chunks of from text widget and searching them
    ** backwards for two newlines in a row (or separated only by white-space)
    */
    if (pos == 0)
    	return pos;
    searchStartPos = pos-1;
    while (True) {

	/* Get the next 255 character block of text */
	bufStartPos = searchStartPos-255 >= 0 ? searchStartPos-255 : 0;
	buf = GetTextRange(textW, bufStartPos, pos);

	/* Scan backwards to find two newlines separated only by white-space */
	for (c= &buf[searchStartPos-bufStartPos], p=searchStartPos;
		p>=bufStartPos; c--, p--) {
    	    if (p == 0) {
    		XtFree(buf);
    		return p;
    	    } else if (*c == '\n') {
    		if (haveNewline) {
    		    XtFree(buf);
    		    return paraStartPos;
    		} else {
    	    	    haveNewline = True;
    	    	    paraStartPos = p+1;
    		}
    	    } else if (*c != ' ' && *c != '\t')
    		haveNewline = False;
	}
	XtFree(buf);
	searchStartPos = p;
    }
}
static XmTextPosition findParagraphEnd(Widget textW, XmTextPosition pos)
{
    XmTextPosition p, paraEndPos, searchStartPos, bufEndPos;
    XmTextPosition lastPos = XmTextGetLastPosition(textW);
    char *buf, *c;
    int haveNewline = False;
    
    /*
    ** loop getting 255 byte chunks of from text widget and searching them
    ** for two newlines in a row (or separated only by white-space)
    */
    searchStartPos = pos;
    while (True) {

	/* Get the next 255 character block of text */
	bufEndPos = searchStartPos+255 > lastPos ? lastPos : searchStartPos+255;
	buf = GetTextRange(textW, searchStartPos, bufEndPos);

	/* Scan the block for two newlines separated only by white-space */
	for (c=buf, p=searchStartPos; *c!='\0'; c++, p++) {
    	    if (*c == '\n') {
    		if (haveNewline) {
    		    XtFree(buf);
    		    return paraEndPos;
    		} else {
    	    	    haveNewline = True;
    	    	    paraEndPos = p;
    		}
    	    } else if (*c != ' ' && *c != '\t')
    		haveNewline = False;
	}
	XtFree(buf);
	if (p >= lastPos)
	    return lastPos;
	searchStartPos = p;
    }
}

/*******************************************************************************
*									       *
* undo.c -- Nirvana Editor undo command					       *
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
*******************************************************************************/
static char SCCSID[] = "@(#)undo.c	1.8     8/21/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "undo.h"
#include "search.h"
#include "window.h"

#define FORWARD 1
#define REVERSE 2

static void addUndoItem(WindowInfo *window, UndoInfo *undo);
static void addRedoItem(WindowInfo *window, UndoInfo *redo);
static void removeUndoItem(WindowInfo *window);
static void removeRedoItem(WindowInfo *window);
static void appendDeletedText(WindowInfo *window, char *deletedText,
	int deletedLen, int direction);
static void trimUndoList(WindowInfo *window, int maxLength);
static int determineUndoType(int nInserted, int nDeleted);
static void freeUndoRecord(UndoInfo *undo);

void Undo(WindowInfo *window)
{
    UndoInfo *undo = window->undo;
    
    /* return if nothing to undo */
    if (undo == NULL)
    	return;
    
    /* BufReplace will eventually call SaveUndoInformation.  This is mostly
       good because it makes accumulating redo operations easier, however
       SaveUndoInformation needs to know that it is being called in the context
       of an undo.  The inUndo field in the undo record indicates that this
       record is in the process of being undone. */
    undo->inUndo = True;
    
    /* use the saved undo information to reverse changes */
    BufReplace(window->buffer, undo->startPos, undo->endPos,
    	    (undo->oldText != NULL ? undo->oldText : ""));
    
    /* position the cursor in the focus pane after the changed text
       to show the user where the undo was done */
    TextSetCursorPos(window->lastFocus, undo->startPos +
    	    (undo->oldText != NULL ? strlen(undo->oldText) : 0));
    
    /* restore the file's unmodified status if the file was unmodified
       when the change being undone was originally made */	    
    if (undo->restoresToSaved)
    	SetWindowModified(window, False);
    
    /* free the undo record and remove it from the chain */
    removeUndoItem(window);
}

void Redo(WindowInfo *window)
{
    UndoInfo *redo = window->redo;
    
    /* return if nothing to redo */
    if (window->redo == NULL)
    	return;

    /* BufReplace will eventually call SaveUndoInformation.  To indicate
       to SaveUndoInformation that this is the context of a redo operation,
       we set the inUndo indicator in the redo record */
    redo->inUndo = True;
    
    /* use the saved redo information to reverse changes */
    BufReplace(window->buffer, redo->startPos, redo->endPos,
    	    (redo->oldText != NULL ? redo->oldText : ""));
    
    /* position the cursor in the focus pane after the changed text
       to show the user where the redo was done */
    TextSetCursorPos(window->lastFocus, redo->startPos +
    	    (redo->oldText != NULL ? strlen(redo->oldText) : 0));
    
    /* restore the file's unmodified status if the file was unmodified
       when the change being redone was originally made */	    
    if (redo->restoresToSaved)
    	SetWindowModified(window, False);
    
    /* remove the redo record from the chain and free it */
    removeRedoItem(window);
}


/*
** SaveUndoInformation stores away the changes made to the text buffer.  As a
** side effect, it also increments the autoSave operation and character counts
** since it needs to do the classification anyhow.
*/
/* Note: This routine must be kept efficient.  It is called for every character
   typed. */
void SaveUndoInformation(WindowInfo *window, int pos, int nInserted,
	int nDeleted, char *deletedText)
{
    int newType, oldType;
    UndoInfo *u, *undo = window->undo;
    int isUndo = (undo != NULL && undo->inUndo);
    int isRedo = (window->redo != NULL && window->redo->inUndo);
    
    /* redo operations become invalid once the user begins typing or does
       other editing.  If this is not a redo or undo operation and a redo
       list still exists, clear it and dim the redo menu item */
    if (!(isUndo || isRedo) && window->redo != NULL)
    	ClearRedoList(window);

    /* figure out what kind of editing operation this is, and recall
       what the last one was */
    newType = determineUndoType(nInserted, nDeleted);
    if (newType == UNDO_NOOP)
    	return;
    oldType = (undo == NULL || isUndo) ? UNDO_NOOP : undo->type;
        
    /*
    ** Check for continuations of single character operations.  These are
    ** accumulated so a whole insertion or deletion can be undone, rather
    ** than just the last character that the user typed.  If the window
    ** is currently in an unmodified state, don't accumulate operations
    ** across the save, so the user can undo back to the unmodified state.
    */
    if (window->fileChanged) {
    
	/* normal sequential character insertion */
	if (  ((oldType == ONE_CHAR_INSERT || oldType == ONE_CHAR_REPLACE)
    	       && newType == ONE_CHAR_INSERT) && (pos == undo->endPos)) {
	    undo->endPos++;
	    window->autoSaveCharCount++;
	    return;
	}

	/* overstrike mode replacement */
	if ((oldType == ONE_CHAR_REPLACE && newType == ONE_CHAR_REPLACE) &&
    		   (pos == undo->endPos)) {
    	    appendDeletedText(window, deletedText, nDeleted, FORWARD);
	    undo->endPos++;
	    window->autoSaveCharCount++;
	    return;
	}

	/* forward delete */
	if ((oldType==ONE_CHAR_DELETE && newType==ONE_CHAR_DELETE) &&
    		   (pos==undo->startPos)) {
    	    appendDeletedText(window, deletedText, nDeleted, FORWARD);
    	    return;
	}

	/* reverse delete */
	if ((oldType==ONE_CHAR_DELETE && newType==ONE_CHAR_DELETE) &&
    		   (pos == undo->startPos-1)) {
    	    appendDeletedText(window, deletedText, nDeleted, REVERSE);
	    undo->startPos--;
	    undo->endPos--;
	    return;
	}
    }
    
    /*
    ** The user has started a new operation, create a new undo record
    ** and save the new undo data.
    */
    undo = (UndoInfo *)XtMalloc(sizeof(UndoInfo));
    undo->oldLen = 0;
    undo->oldText = NULL;
    undo->type = newType;
    undo->inUndo = False;
    undo->restoresToSaved = False;
    undo->startPos = pos;
    undo->endPos = pos + nInserted;

    /* if text was deleted, save it */
    if (nDeleted > 0) {
	undo->oldLen = nDeleted + 1;	/* +1 is for null at end */
	undo->oldText = XtMalloc(nDeleted + 1);
	strcpy(undo->oldText, deletedText);
    }
    
    /* increment the operation count for the autosave feature */
    window->autoSaveOpCount++;

    /* if the window is currently unmodified, remove the previous
       restoresToSaved marker, and set it on this record */
    if (!window->fileChanged) {
    	undo->restoresToSaved = True;
	for (u=window->undo; u!=NULL; u=u->next)
    	    u->restoresToSaved = False;
    }
    	
    /* Add the new record to the undo list  unless SaveUndoInfo is
       saving information generated by an Undo operation itself, in
       which case, add the new record to the redo list. */
    if (isUndo)
	addRedoItem(window, undo);
    else
	addUndoItem(window, undo);
}

/*
** ClearUndoList, ClearRedoList
**
** Functions for clearing all of the information off of the undo or redo
** lists and adjusting the edit menu accordingly
*/
void ClearUndoList(WindowInfo *window)
{
    while (window->undo != NULL)
    	removeUndoItem(window);
}
void ClearRedoList(WindowInfo *window)
{
    while (window->redo != NULL)
    	removeRedoItem(window);
}

/*
** Add an undo record (already allocated by the caller) to the window's undo
** list if the item pushes the undo operation or character counts past the
** limits, trim the undo list to an acceptable length.
*/
static void addUndoItem(WindowInfo *window, UndoInfo *undo)
{
    
    /* Make the undo menu item sensitive now that there's something to undo */
    if (window->undo == NULL)
    	XtSetSensitive(window->undoItem, True);

    /* Add the item to the beginning of the list */
    undo->next = window->undo;
    window->undo = undo;
    
    /* Increment the operation and memory counts */
    window->undoOpCount++;
    window->undoMemUsed += undo->oldLen;
    
    /* Trim the list if it exceeds any of the limits */
    if (window->undoOpCount > UNDO_OP_LIMIT)
    	trimUndoList(window, UNDO_OP_TRIMTO);
    if (window->undoMemUsed > UNDO_WORRY_LIMIT)
    	trimUndoList(window, UNDO_WORRY_TRIMTO);
    if (window->undoMemUsed > UNDO_PURGE_LIMIT)
    	trimUndoList(window, UNDO_PURGE_TRIMTO);
}

/*
** Add an item (already allocated by the caller) to the window's redo list.
*/
static void addRedoItem(WindowInfo *window, UndoInfo *redo)
{
    /* Make the redo menu item sensitive now that there's something to redo */
    if (window->redo == NULL)
    	XtSetSensitive(window->redoItem, True);

    /* Add the item to the beginning of the list */
    redo->next = window->redo;
    window->redo = redo;
}

/*
** Pop (remove and free) the current (front) undo record from the undo list
*/
static void removeUndoItem(WindowInfo *window)
{
    UndoInfo *undo = window->undo;
    
    if (undo == NULL)
    	return;
    
    /* Decrement the operation and memory counts */
    window->undoOpCount--;
    window->undoMemUsed -= undo->oldLen;
    
    /* Remove and free the item */
    window->undo = undo->next;
    freeUndoRecord(undo);
    
    /* if there are no more undo records left, dim the Undo menu item */
    if (window->undo == NULL)
    	XtSetSensitive(window->undoItem, False);
}

/*
** Pop (remove and free) the current (front) redo record from the redo list
*/
static void removeRedoItem(WindowInfo *window)
{
    UndoInfo *redo = window->redo;
    
    /* Remove and free the item */
    window->redo = redo->next;
    freeUndoRecord(redo);
    
    /* if there are no more redo records left, dim the Redo menu item */
    if (window->redo == NULL)
    	XtSetSensitive(window->redoItem, False);
}

/*
** Add deleted text to the beginning or end
** of the text saved for undoing the last operation.  This routine is intended
** for continuing of a string of one character deletes or replaces, but will
** work with more than one character.
*/
static void appendDeletedText(WindowInfo *window, char *deletedText,
	int deletedLen, int direction)
{
    UndoInfo *undo = window->undo;
    char *comboText;

    /* re-allocate, adding space for the new character(s) */
    comboText = XtMalloc(undo->oldLen + deletedLen);

    /* copy the new character and the already deleted text to the new memory */
    if (direction == FORWARD) {
    	strcpy(comboText, undo->oldText);
    	strcat(comboText, deletedText);
    } else {
	strcpy(comboText, deletedText);
	strcat(comboText, undo->oldText);
    }

    /* keep track of the additional memory now used by the undo list */
    window->undoMemUsed++;

    /* free the old saved text and attach the new */
    XtFree(undo->oldText);
    undo->oldText = comboText;
    undo->oldLen += deletedLen;
}

/*
** Trim records off of the END of the undo list to reduce it to length
** maxLength
*/
static void trimUndoList(WindowInfo *window, int maxLength)
{
    int i;
    UndoInfo *u, *lastRec;
    
    if (window->undo == NULL)
    	return;

    /* Find last item on the list to leave intact */
    for (i=1, u=window->undo; i<maxLength && u!=NULL; i++, u=u->next);
    if (u == NULL)
    	return;
    
    /* Trim off all subsequent entries */
    lastRec = u;
    while (lastRec->next != NULL) {
	u = lastRec->next;
	lastRec->next = u->next;
    	window->undoOpCount--;
    	window->undoMemUsed -= u->oldLen;
    	freeUndoRecord(u);
    }
}
  
static int determineUndoType(int nInserted, int nDeleted)
{
    int textDeleted, textInserted;
    
    textDeleted = (nDeleted > 0);
    textInserted = (nInserted > 0);
    
    if (textInserted && !textDeleted) {
    	/* Insert */
	if (nInserted == 1)
	    return ONE_CHAR_INSERT;
	else
	    return BLOCK_INSERT;
    } else if (textInserted && textDeleted) {
    	/* Replace */
	if (nInserted == 1)
	    return ONE_CHAR_REPLACE;
	else
	    return BLOCK_REPLACE;
    } else if (!textInserted && textDeleted) {
    	/* Delete */
	if (nDeleted == 1)
	    return ONE_CHAR_DELETE;
	else
	    return BLOCK_DELETE;
    } else {
    	/* Nothing deleted or inserted */
	return UNDO_NOOP;
    }
}

static void freeUndoRecord(UndoInfo *undo)
{
    if (undo == NULL)
    	return;
    	
    XtFree(undo->oldText);
    XtFree((char *)undo);
}

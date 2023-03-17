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
static char SCCSID[] = "@(#)undo.c	1.7     3/8/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Text.h>
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
static void appendDeletedText(WindowInfo *window, XmTextVerifyPtr mod,
				    int direction);
static void trimUndoList(WindowInfo *window, int maxLength);
static int determineUndoType(XmTextVerifyPtr mod);
static void freeUndoRecord(UndoInfo *undo);

void Undo(WindowInfo *window)
{
    UndoInfo *undo = window->undo;
    
    /* return if nothing to undo */
    if (undo == NULL)
    	return;
    
    /* XmTextReplace will eventually call SaveUndoInformation.  This is mostly
       good because it makes accumulating redo operations easier, however
       SaveUndoInformation needs to know that it is being called in the context
       of an undo.  The inUndo field in the undo record indicates that this
       record is in the process of being undone. */
    undo->inUndo = True;
    
    /* use the saved undo information to reverse changes */
    XmTextReplace(window->textArea, undo->startPos, undo->endPos,undo->oldText);
    
    /* free the undo record and remove it from the chain */
    removeUndoItem(window);
}

void Redo(WindowInfo *window)
{
    UndoInfo *redo = window->redo;
    
    /* return if nothing to redo */
    if (window->redo == NULL)
    	return;

    /* XmTextReplace will eventually call SaveUndoInformation.  To indicate
       to SaveUndoInformation that this is the context of a redo operation,
       we set the inUndo indicator in the redo record */
    redo->inUndo = True;
    
    /* use the saved redo information to reverse changes */
    XmTextReplace(window->textArea, redo->startPos, redo->endPos,redo->oldText);
    
    /* remove the redo record from the chain and free it */
    removeRedoItem(window);
}


/*
** SaveUndoInformation stores away the contents of the text widget's modify
** verify information in a form useable by Undo.  As a side effect, it also
** increments the autoSave operation and character counts since it needs to
** do the classification anyhow.
*/
/* Some Notes:
   1) this assumes the format of the text returned in mod is FMT8BIT
      It would be strange if it wasn't, since you give the text widget strings
      in standard c format, I can't imagine it converting them to FMT16BIT to
      pass them back to you.
   2) This routine must be kept efficient.  It is called for every character
      typed. */
void SaveUndoInformation(WindowInfo *window, XmTextVerifyPtr mod)
{
    int newType, oldType, nInserted, nDeleted;
    UndoInfo *temp, *undo = window->undo;
    int isUndo = (undo != NULL && undo->inUndo);
    int isRedo = (window->redo != NULL && window->redo->inUndo);
    
    /* redo operations become invalid once the user begins typing or does
       other editing.  If this is not a redo or undo operation and a redo
       list still exists, clear it and dim the redo menu item */
    if (!(isUndo || isRedo) && window->redo != NULL)
    	ClearRedoList(window);

    /* figure out what kind of editing operation this is, and recall
       what the last one was */
    newType = determineUndoType(mod);
    if (newType == UNDO_NOOP)
    	return;
    oldType = (undo == NULL || isUndo) ? UNDO_NOOP : undo->type;
        
    /*
    ** Check for continuations of single character operations.  These are
    ** accumulated so a whole insertion or deletion can be undone, rather
    ** than just the last character that the user typed.
    */ 
    /* normal sequential character insertion */
    if (  ((oldType == ONE_CHAR_INSERT || oldType == ONE_CHAR_REPLACE)
    	   && newType == ONE_CHAR_INSERT) && (mod->startPos == undo->endPos)) {
	undo->endPos++;
	window->autoSaveCharCount++;
	return;
    }
    
    /* overstrike mode replacement */
    if ((oldType == ONE_CHAR_REPLACE && newType == ONE_CHAR_REPLACE) &&
    	       (mod->startPos == undo->endPos)) {
    	appendDeletedText(window, mod, FORWARD);
	undo->endPos++;
	window->autoSaveCharCount++;
	return;
    }
    
    /* forward delete */
    if ((oldType==ONE_CHAR_DELETE && newType==ONE_CHAR_DELETE) &&
    	       (mod->startPos==undo->startPos)) {
    	appendDeletedText(window, mod, FORWARD);
    	return;
    }
    
    /* reverse delete */
    if ((oldType==ONE_CHAR_DELETE && newType==ONE_CHAR_DELETE) &&
    	       (mod->startPos == undo->startPos-1)) {
    	appendDeletedText(window, mod, REVERSE);
	undo->startPos--;
	undo->endPos--;
	return;
    }
    
    /* delete followed immediatly by insert at same point.  Lump
       these together to form a replace.  This happens alot in Motif 1.0,
       not as much in 1.1 (single character typed in pending delete
       selection still produces two modify verify callbacks in 1.1) */
    if ((oldType==ONE_CHAR_DELETE || oldType==BLOCK_DELETE) &&
    	       (newType==ONE_CHAR_INSERT || newType==BLOCK_INSERT) &&
    	       (mod->startPos == undo->startPos)) {
    	undo->endPos += mod->text->length;
    	undo->type = (newType == ONE_CHAR_INSERT) ?
    	    ONE_CHAR_REPLACE : BLOCK_REPLACE;
    	return;
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
    nInserted = mod->text->length;
    nDeleted = mod->endPos - mod->startPos;
    undo->startPos = mod->startPos;
    undo->endPos = mod->startPos + nInserted;

    /* if text was deleted, save it */
    if (nDeleted > 0) {
	undo->oldLen = nDeleted + 1;	/* +1 is for null at end */
	undo->oldText = 
	    GetTextRange(window->textArea, mod->startPos, mod->endPos);
    }
    
    /* increment the operation count for the autosave feature */
    window->autoSaveOpCount++;

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
** get the text that is about to be deleted and add to the beginning or end
** of the text saved for undoing the last operation.  This routine is intended
** for continuing of a string of one character deletes or replaces, but will
** work with more than one character.
*/
static void appendDeletedText(WindowInfo *window, XmTextVerifyPtr mod,
				    int direction)
{
    UndoInfo *undo = window->undo;
    char *newText, *comboText;
    int newLen;

    /* get the text that is about to be consumed */
    newText = GetTextRange(window->textArea, mod->startPos,  mod->endPos);
    newLen = mod->endPos - mod->startPos;

    /* re-allocate space for saved text, adding 1 for the new character */
    comboText = XtMalloc(undo->oldLen + newLen);

    /* copy the new character and the already deleted text to the new memory */
    if (direction == FORWARD) {
    	strcpy(comboText, undo->oldText);
    	strcat(comboText, newText);
    } else {
	strcpy(comboText, newText);
	strcat(comboText, undo->oldText);
    }

    /* keep track of the additional memory now used by the undo list */
    window->undoMemUsed++;

    /* free the old saved text and attach the new */
    XtFree(undo->oldText);
    undo->oldText = comboText;
    undo->oldLen += newLen;

    /* dispose of the memory allocated by GetTextRange */
    XtFree(newText);
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
  
static int determineUndoType(XmTextVerifyPtr mod)
{
    int textDeleted, textInserted, nInserted, nDeleted;
    
    nInserted = mod->text->length;
    nDeleted = mod->endPos - mod->startPos;
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

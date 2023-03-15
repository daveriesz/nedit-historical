/*******************************************************************************
*									       *
* nedit.h -- Nirvana Editor common include file				       *
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
/* SCCS ID: nedit.h 1.15 8/26/94 */

/* Tuning parameters */
#define SEARCHMAX 511		/* Maximum length of search/replace strings */
#define MAX_SEARCH_HISTORY 100	/* Maximum length of search string history */
#define MAX_PANES 6		/* Max # of ADDITIONAL text editing panes
				   that can be added to a window */
#ifndef VMS
#define AUTOSAVE_CHAR_LIMIT 30	/* number of characters user can type before
				   NEdit generates a new backup file */
#else
#define AUTOSAVE_CHAR_LIMIT 80	/* set higher on VMS becaus saving is slower */
#endif /*VMS*/
#define AUTOSAVE_OP_LIMIT 8	/* number of distinct editing operations user
				   can do before NEdit gens. new backup file */
#define MAX_LINE_LENGTH	2048	/* the maximum allowable line len. for several
				   operations (shift lines, auto indent) */
#define MAX_FONT_LEN 100	/* maximum length for a font name */
#define APP_NAME "nedit"	/* application name for loading resources */
#define APP_CLASS "NEdit"	/* application class for loading resources */

/* The accumulated list of undo operations can potentially consume huge
   amounts of memory.  These tuning parameters determine how much undo infor-
   mation is retained.  Normally, the list is kept between UNDO_OP_LIMIT and
   UNDO_OP_TRIMTO in length (when the list reaches UNDO_OP_LIMIT, it is
   trimmed to UNDO_OP_TRIMTO then allowed to grow back to UNDO_OP_LIMIT).
   When there are very large amounts of saved text held in the list,
   UNDO_WORRY_LIMIT and UNDO_PURGE_LIMIT take over and cause the list to
   be trimmed back further to keep its size down. */
#define UNDO_PURGE_LIMIT 15000000 /* If undo list gets this large (in bytes),
				     trim it to length of UNDO_PURGE_TRIMTO */
#define UNDO_PURGE_TRIMTO 1	  /* Amount to trim the undo list in a purge */
#define UNDO_WORRY_LIMIT 2000000  /* If undo list gets this large (in bytes),
				     trim it to length of UNDO_WORRY_TRIMTO */
#define UNDO_WORRY_TRIMTO 5	  /* Amount to trim the undo list when memory
				     use begins to get serious */
#define UNDO_OP_LIMIT 400	  /* normal limit for length of undo list */
#define UNDO_OP_TRIMTO 200	  /* size undo list is normally trimmed to
				     when it exceeds UNDO_OP_TRIMTO in length */

#define CHARSET (XmStringCharSet)XmSTRING_DEFAULT_CHARSET

#define MKSTRING(string) \
	XmStringCreateLtoR(string, XmSTRING_DEFAULT_CHARSET)
	
#define SET_ONE_RSRC(widget, name, newValue) \
{ \
    static Arg args[1] = {{name, (XtArgVal)0}}; \
    args[0].value = (XtArgVal)newValue; \
    XtSetValues(widget, args, 1); \
}	

#define GET_ONE_RSRC(widget, name, valueAddr) \
{ \
    static Arg args[1] = {{name, (XtArgVal)0}}; \
    args[0].value = (XtArgVal)valueAddr; \
    XtGetValues(widget, args, 1); \
}

typedef struct _UndoInfo {
    struct _UndoInfo *next;		/* pointer to the next undo record */
    int		type;
    int		startPos;
    int		endPos;
    int 	oldLen;
    char	*oldText;
    char	inUndo;			/* flag to indicate undo command on
    					   this record in progress.  Redirects
    					   SaveUndoInfo to save the next mod-
    					   ifications on the redo list instead
    					   of the undo list. */
} UndoInfo;

typedef struct _WindowInfo {
    struct _WindowInfo *next;
    Widget	shell;			/* application shell of window */
    Widget	splitPane;		/* paned win. for splitting text area */
    Widget	textArea;		/* the first text editing area widget */
    Widget	textPanes[MAX_PANES];	/* additional ones created on demand */
    Widget	lastFocus;		/* the last pane to have kbd. focus */
    Widget	statsLine;		/* file stats information display */
    Widget	replaceDlog;		/* replace dialog */
    Widget	replaceText;		/* replace dialog setable widgets... */
    Widget	replaceWithText;
    Widget	replaceLiteralBtn;
    Widget	replaceCaseBtn;
    Widget	replaceRegExpBtn;
    Widget	replaceBtns;
    Widget	replaceBtn;
    Widget	replaceInSelBtn;
    Widget	replaceSearchTypeBox;
    Widget	findDlog;		/* find dialog */
    Widget	findText;		/* find dialog setable widgets... */
    Widget	findLiteralBtn;
    Widget	findCaseBtn;
    Widget	findRegExpBtn;
    Widget	findBtns;
    Widget	findBtn;
    Widget	findSearchTypeBox;
    Widget	closeItem;		/* menu bar setable widgets... */
    Widget	undoItem;
    Widget	redoItem;
    Widget	findDefItem;
    Widget	windowMenuPane;
    Widget	filterMenuPane;
    Widget	autoIndentDefItem;
    Widget	autoSaveDefItem;
    Widget	wrapTextDefItem;
    Widget	showMatchingDefItem;
    Widget	searchDlogsDefItem;
    Widget	reposDlogsDefItem;
    Widget	statsLineDefItem;
    Widget	searchLiteralDefItem;
    Widget	searchCaseSenseDefItem;
    Widget	searchRegexDefItem;
    Widget	size24x80DefItem;
    Widget	size40x80DefItem;
    Widget	size60x80DefItem;
    Widget	size80x80DefItem;
    Widget	sizeCustomDefItem;
    Widget	splitWindowItem;
    Widget	closePaneItem;
    char	filename[MAXPATHLEN];	/* name component of file being edited*/
    char	path[MAXPATHLEN];	/* path component of file being edited*/
    int		fileMode;		/* permissions of file being edited */
    UndoInfo	*undo;			/* info for undoing last operation */
    UndoInfo	*redo;			/* info for redoing last undone op */
    int		nPanes;			/* number of additional text editing
    					   areas, created by splitWindow */
    int		autoSaveCharCount;	/* count of single characters typed
    					   since last backup file generated */
    int		autoSaveOpCount;	/* count of editing operations "" */
    int		undoOpCount;		/* count of stored undo operations */
    int		undoMemUsed;		/* amount of memory (in bytes)
    					   dedicated to the undo list */
    char	fontName[MAX_FONT_LEN];	/* name of the text font in use */
    XmFontList	fontList;		/* fontList for the font */
    int		tabDist;		/* number of characters in a tab stop */
    XtIntervalId flashTimeoutID;	/* timer procedure id for getting rid
    					   of highlighted matching paren.  Non-
    					   zero val. means highlight is drawn */
    XmTextPosition flashPos;		/* position saved for erasing matching
    					   paren highlight (if one is drawn) */
    Widget	flashPane;		/* pane match marker is drawn in */
    Boolean	filenameSet;		/* is the window still "Untitled"? */ 
    Boolean	fileChanged;		/* has window been modified? */
    Boolean	readOnly;		/* is current file read only? */
    Boolean	autoIndent;		/* is autoindent turned on? */
    Boolean	autoSave;		/* is autosave turned on? */
    Boolean	wrap;			/* is text wrapping turned on? */
    Boolean	overstrike;		/* is overstrike mode turned on ? */
    Boolean	showMatching;		/* is paren matching mode on? */
    Boolean	ignoreModify;		/* ignore modifications to text area */
    Boolean	windowMenuValid;	/* is window menu is up to date? */
    int		rHistIndex, fHistIndex;	/* stupid globals for find and replace
    					   dialogs */
} WindowInfo;

extern WindowInfo *WindowList;
extern Display *TheDisplay;

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "../util/DialogF.h"
#include "textBuf.h"
#include "nedit.h"
#include "window.h"
#include "macro.h"

/* Maximum number of actions in a macro and args in 
   an action (to simplify the reader) */
#define MAX_MACRO_ACTIONS 1024
#define MAX_ACTION_ARGS 40

static void actionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams);
static int isMouseAction(char *action);
static int isRedundantAction(char *action);
static int isIgnoredAction(char *action);
static char *parseAction(char **inPtr, argList **argL, char **errMsg);
static argList *parseArgList(char **inPtr, char **errMsg);
static void freeArgList(argList *argL);
static char *parseArg(char **inPtr, char **errMsg);
static char *escapeQuotes(char *string);

/* List of actions to ignore when learning a macro sequence */
static char* IgnoredActions[] = {"focusIn", "focusOut"};

/* List of actions intended to be attached to mouse buttons, which the user
   must be warned can't be recorded in a learn/replay sequence */
static char* MouseActions[] = {"grab-focus", "extend-adjust", "extend-start",
	"extend-end", "secondary-adjust", "secondary-start", "move-destination",
	"move-to", "copy-to", "exchange"};

/* List of actions to not record because they 
   generate further actions, more suitable for recording */
static char* RedundantActions[] = {"open-dialog", "save-as-dialog",
	"include-file-dialog", "load-tags-file-dialog", "find-dialog",
	"replace-dialog", "goto-line-number-dialog", "control-code-dialog",
	"filter-selection-dialog", "execute-command-dialog",
	"macro-menu-command"};

/* The current macro to execute on Replay command */
static char *ReplayMacro = NULL;

/* Buffer where macro commands are recorded in Learn mode */
static textBuffer *MacroRecordBuf = NULL;

/* Action Hook id for recording actions for Learn mode */
static XtActionHookId MacroRecordActionHook = 0;

/* Window where macro recording is taking place */
static WindowInfo *MacroRecordWindow = NULL;

void BeginLearn(WindowInfo *window)
{
    WindowInfo *win;
    
    /* If we're already in learn mode, return */
    if (MacroRecordActionHook != 0)
    	return;
    
    /* dim the inappropriate menus and items, and undim finish and cancel */
    for (win=WindowList; win!=NULL; win=win->next)
	XtSetSensitive(win->learnItem, False);
    XtSetSensitive(window->finishLearnItem, True);
    XtSetSensitive(window->cancelLearnItem, True);
    
    /* Mark the window where learn mode is happening */
    MacroRecordWindow = window;
    
    /* Allocate a text buffer for accumulating the macro strings */
    MacroRecordBuf = BufCreate();
    
    /* Add the action hook for recording the actions */
    MacroRecordActionHook =
    	    XtAppAddActionHook(XtWidgetToApplicationContext(window->shell),
    	    actionHook, window);
    
    /* Put up the learn-mode banner */
    SetModeMessage(window,
    	    "Learn Mode -- Press Alt+K to finish, Ctrl+. to cancel");
}

void FinishLearn(void)
{
    WindowInfo *win;
    
    /* If we're not in learn mode, return */
    if (MacroRecordActionHook == 0)
    	return;
    
    /* Remove the action hook */
    XtRemoveActionHook(MacroRecordActionHook);
    MacroRecordActionHook = 0;
    
    /* Free the old learn/replay sequence */
    if (ReplayMacro != NULL)
    	XtFree(ReplayMacro);
    
    /* Store the finished action for the replay menu item */
    ReplayMacro = BufGetAll(MacroRecordBuf);
    
    /* Free the buffer used to accumulate the macro sequence */
    BufFree(MacroRecordBuf);
    
    /* Undim the menu items dimmed during learn */
    for (win=WindowList; win!=NULL; win=win->next)
	XtSetSensitive(win->learnItem, True);
    XtSetSensitive(MacroRecordWindow->finishLearnItem, False);
    XtSetSensitive(MacroRecordWindow->cancelLearnItem, False);
    
    /* Undim the replay and paste-macro buttons */
    for (win=WindowList; win!=NULL; win=win->next) {
    	XtSetSensitive(win->replayItem, True);
	if (win->macroCmdDialog)
    	    XtSetSensitive(win->macroPasteReplayBtn, True); 
    }
    
    /* Clear learn-mode banner */
    ClearModeMessage(MacroRecordWindow);
}

void CancelLearn(void)
{
    WindowInfo *win;
    
    /* If we're not in learn mode, return */
    if (MacroRecordActionHook == 0)
    	return;

    /* Remove the action hook */
    XtRemoveActionHook(MacroRecordActionHook);
    MacroRecordActionHook = 0;
    
    /* Free the macro under construction */
    BufFree(MacroRecordBuf);
    
    /* Undim the menu items dimmed during learn */
    for (win=WindowList; win!=NULL; win=win->next)
	XtSetSensitive(win->learnItem, True);
    XtSetSensitive(MacroRecordWindow->finishLearnItem, False);
    XtSetSensitive(MacroRecordWindow->cancelLearnItem, False);
    
    /* Clear learn-mode banner */
    ClearModeMessage(MacroRecordWindow);
}

/*
** Execute the learn/replay sequence stored in "window"
*/
void Replay(WindowInfo *window)
{
    XKeyEvent event;
    actionList *actionL;
    char *macroPtr, *errMsg;
    int i;
    
    if (ReplayMacro == NULL)
    	return;
    
    /* Parse the replay macro (it's stored in text form) */
    macroPtr = ReplayMacro;
    actionL = ParseActionList(&macroPtr, &errMsg);
    if (actionL == NULL) {
    	fprintf(stderr,
    		"NEdit internal error, learn/replay macro syntax error: %s\n",
    		errMsg);
    	return;
    }
    
    /* Create a fake event with a timestamp suitable for actions which need
       timestamps */
    event.type = KeyPress;
    event.time = XtLastTimestampProcessed(XtDisplay(window->shell));
    
    /* Execute the actions in the macro */
    for (i=0; i<actionL->nActions; i++) {
    	XtCallActionProc(window->lastFocus, actionL->actions[i],
    		(XEvent *)&event, actionL->args[i]->args,
    		actionL->args[i]->nArgs);
    }
    FreeActionList(actionL);
}

/*
** Executes a macro string "macro" using the lastFocus pane in "window"
*/
void DoMacro(WindowInfo *window, char *macro)
{
    XKeyEvent event;
    actionList *actionL;
    char *macroPtr, *errStr, *errMsg;
    int reportLen, i;
    
    /* Parse the macro and report errors if it fails */
    macroPtr = macro;
    actionL = ParseActionList(&macroPtr, &errMsg);
    if (actionL == NULL) {
    	reportLen = macroPtr + 1 - macro;
    	if (reportLen > 160) {
    	    reportLen = 160;
    	    errStr = XtMalloc(reportLen + 4);
    	    strcpy(errStr, "...");
    	    strncpy(errStr+3, macroPtr - reportLen, reportLen);
    	    errStr[reportLen+3] = '\0';
    	} else {
    	    errStr = XtMalloc(reportLen + 1);
    	    strncpy(errStr, macro, reportLen);
    	    errStr[reportLen] = '\0';
   	}
    	DialogF(DF_ERR, window->shell, 1, "Error parsing macro: %s\n%s<<<---\n",
    		 "Dismiss", errMsg, errStr);
    	XtFree(errStr);
    	return;
    }
    
    /* Create a fake event with just a timestamp, suitable
       for actions which need timestamps */
    event.type = KeyPress;
    event.time = XtLastTimestampProcessed(XtDisplay(window->shell));
    
    /* Execute the actions in the macro */
    for (i=0; i<actionL->nActions; i++)
    	XtCallActionProc(window->lastFocus, actionL->actions[i],
    		(XEvent *)&event, actionL->args[i]->args,
    		actionL->args[i]->nArgs);
    FreeActionList(actionL);
}

/*
** Get the current Learn/Replay macro in text form.  Returned string is a
** pointer to the stored macro and should not be freed by the caller (and
** will cease to exist when the next replay macro is installed)
*/
char *GetReplayMacro(void)
{
    return ReplayMacro;
}

/*
** Parse a string in to a list of action names and parameters, optionally
** surrounded by braces "{}" in string pointed to by inPtr. If successful,
** returns the actions in an actionList data structure, which should be freed
** with FreeActionList.  If unsuccessful, returns NULL with a (statically
** allocated) message in "errMsg", and inPtr pointing to the location of
** the error.
**
** Note: parser leaks memory on errors
*/
actionList *ParseActionList(char **inPtr, char **errMsg)
{
    char *action, *actions[MAX_MACRO_ACTIONS];
    argList *argL, *argLists[MAX_MACRO_ACTIONS];
    actionList *actionL;
    int nActions = 0;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    /* skip over optional brace */
    if (**inPtr == '{')
    	(*inPtr)++;

    /* parse actions until end of string or end brace */
    while (True) {
    	*inPtr += strspn(*inPtr, " \t\n");
    	if (**inPtr == '\0')
    	    break;
    	if (nActions == MAX_MACRO_ACTIONS) {
    	    fprintf(stderr,"NEdit: max number of actions per macro exceeded\n");
    	    return NULL;
    	}
    	action = parseAction(inPtr, &argL, errMsg);
    	if (action == NULL)
    	    return NULL;
    	actions[nActions] = action;
    	argLists[nActions++] = argL;
    	*inPtr += strspn(*inPtr, " \t\n");
    	if (**inPtr == '}' || **inPtr == '\0')
    	    break;
    }
    
    /* skip over optional end brace */
    if (**inPtr == '}')
    	(*inPtr)++;
    
    /* allocate an actionList data structure to return the results */
    actionL = (actionList *)XtMalloc(sizeof(actionList));
    actionL->nActions = nActions;
    if (nActions == 0) {
    	actionL->actions = NULL;
    	actionL->args = NULL;
    } else {
    	actionL->actions = (char **)XtMalloc(sizeof(char *) * nActions);
    	memcpy(actionL->actions, actions, sizeof(char *) * nActions);
    	actionL->args = (argList **)XtMalloc(sizeof(argList *) * nActions);
    	memcpy(actionL->args, argLists, sizeof(argList *) * nActions);
    }
    return actionL;
}

void FreeActionList(actionList *actionL)
{
    int i;
    
    for (i=0; i<actionL->nActions; i++) {
        XtFree(actionL->actions[i]);
        freeArgList(actionL->args[i]);
    }
    if (actionL->nActions != 0) {
    	XtFree((char *)actionL->actions);
    	XtFree((char *)actionL->args);
    }
    XtFree((char *)actionL);
}

/*
** Return a string representation of the actions in action list "actionL",
** indented by prefixing "indentStr" to each line output
*/
char *ActionListToString(actionList *actionL, char *indentStr)
{
    argList *argL;
    int a, i;
    textBuffer *buf;
    char *retStr, *quoteEscapedParam;
    
    buf = BufCreate();
    for (a=0; a<actionL->nActions; a++) {
	BufInsert(buf, buf->length, indentStr);
	BufInsert(buf, buf->length, actionL->actions[a]);
	BufInsert(buf, buf->length, "(");
	argL = actionL->args[a];
	for (i=0; i<argL->nArgs; i++) {
    	    BufInsert(buf, buf->length, "\"");
    	    quoteEscapedParam = escapeQuotes(argL->args[i]);
    	    BufInsert(buf, buf->length, quoteEscapedParam);
    	    XtFree(quoteEscapedParam);
    	    BufInsert(buf, buf->length, "\", ");
    	}
	if (argL->nArgs != 0)
    	    BufRemove(buf, buf->length-2, buf->length);
	BufInsert(buf, buf->length, ")\n");
    }
    retStr = BufGetAll(buf);
    BufFree(buf);
    return retStr;
}

/*
** Macro recording action hook
*/
static void actionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams)
{
    WindowInfo *window;
    textBuffer *buf;
    char chars[20], *charList[1], *quoteEscapedParam;
    KeySym keysym;
    int i, nChars, nParams;
    
    /* Select only actions in text panes in the window for which this
       action hook is recording macros (from clientData).  Also ignore
       non-operational actions and dialogs, and beep on un-recordable
       operations which require a mouse position */
    for (window=WindowList; window!=NULL; window=window->next) {
	if (window->textArea == w)
	    break;
	for (i=0; i<window->nPanes; i++) {
    	    if (window->textPanes[i] == w)
    	    	break;
	}
	if (i < window->nPanes)
	    break;
    }
    if (window == NULL || window != (WindowInfo *)clientData ||
    	    isIgnoredAction(actionName) || isRedundantAction(actionName))
    	return;
    if (isMouseAction(actionName)) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    
    /* Convert self-insert actions, to insert-string */
    if (!strcmp(actionName, "self-insert")) {
    	actionName = "insert-string";
	nChars = XLookupString((XKeyEvent *)event, chars, 19, &keysym, NULL);
	if (nChars == 0)
    	    return;
    	chars[nChars] = '\0';
    	charList[0] = chars;
    	params = charList;
    	nParams = 1;
    } else
    	nParams = *numParams;
    	
    /* Record the action and its parameters */
    buf = MacroRecordBuf;
    BufInsert(buf, buf->length, actionName);
    BufInsert(buf, buf->length, "(");
    for (i=0; i<nParams; i++) {
    	BufInsert(buf, buf->length, "\"");
    	quoteEscapedParam = escapeQuotes(params[i]);
    	BufInsert(buf, buf->length, quoteEscapedParam);
    	XtFree(quoteEscapedParam);
    	BufInsert(buf, buf->length, "\", ");
    }
    if (nParams != 0)
    	BufRemove(buf, buf->length-2, buf->length);
    BufInsert(buf, buf->length, ")\n");
}

static int isMouseAction(char *action)
{
    int i;
    
    for (i=0; i<XtNumber(MouseActions); i++)
    	if (!strcmp(action, MouseActions[i]))
    	    return True;
    return False;
}

static int isRedundantAction(char *action)
{
    int i;
    
    for (i=0; i<XtNumber(RedundantActions); i++)
    	if (!strcmp(action, RedundantActions[i]))
    	    return True;
    return False;
}

static int isIgnoredAction(char *action)
{
    int i;
    
    for (i=0; i<XtNumber(IgnoredActions); i++)
    	if (!strcmp(action, IgnoredActions[i]))
    	    return True;
    return False;
}

/*
** Parse an action, including name and arguments.  If successful, returns
** the action name as the function value (an allocated string), and an
** argList data structure containing the arguments and argument count in
** "argL".  If unsuccessful, returns NULL with (statically allocated)
** message in "errMsg".
*/
static char *parseAction(char **inPtr, argList **argL, char **errMsg)
{
    char *nameStart, *nameEnd, *name;
    int nameLen;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");

    /* read the action name */
    nameStart = *inPtr;
    while (True) {
    	if (**inPtr == '\0') {
    	    *errMsg = "end of macro while expecting action";
    	    return False;
    	} else if (**inPtr == '(' || **inPtr == ' ' || **inPtr == '\t') {
    	    break;
    	} else if (!isalnum(**inPtr) && **inPtr != '-') {
    	    *errMsg = "only letters, digits, and - allowed in action names";
    	    return NULL;
    	} else
    	    (*inPtr)++;
    }
    nameEnd = *inPtr;
    if (nameEnd == nameStart) {
    	*errMsg = "expecting action name";
    	return NULL;
    }
    
    /* read the argument list */
    *argL = parseArgList(inPtr, errMsg);
    if (*argL == NULL)
    	return NULL;
    
    /* allocate and return a string with a copy of the action name */
    nameLen = nameEnd - nameStart;
    name = XtMalloc(nameLen+1);
    strncpy(name, nameStart, nameLen);
    name[nameLen] = '\0';
    return name;
}

/*
** Parse an argument list, including both parenthesis.  If successful,
** returns an argList data structure containing the arguments and
** argument count.  If unsuccessful, returns NULL with (statically
** allocated) message in "errMsg".
*/
static argList *parseArgList(char **inPtr, char **errMsg)
{
    char *arg, *args[MAX_ACTION_ARGS];
    argList *argL;
    int nArgs = 0;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    /* look for initial paren */
    if (**inPtr == '\0') {
    	*errMsg = "end of macro while looking for argument list";
    	return False;
    } else if (**inPtr != '(') {
    	*errMsg = "expecting argument list";
    	return False;
    }
    (*inPtr)++;
    
    /* parse the arguments */
    while (True) {
    	*inPtr += strspn(*inPtr, " \t");
    	if (**inPtr == '\0') {
    	    *errMsg = "end of macro inside argument list";
    	    return NULL;
	} else if (**inPtr == ')')
    	    break;
    	if (nArgs == MAX_ACTION_ARGS) {
    	    fprintf(stderr,"NEdit: max number of arguments exceeded\n");
    	    return NULL;
    	}
    	arg = parseArg(inPtr, errMsg);
    	if (arg == NULL)
    	    return NULL;
    	args[nArgs++] = arg;
    	*inPtr += strspn(*inPtr, " \t");
    	if (**inPtr == '\0') {
    	    *errMsg = "end of macro inside argument list";
    	    return NULL;
	} else if (**inPtr == ')') {
    	    break;
    	} else if (**inPtr == ',') {
    	    (*inPtr)++;
    	} else {
    	    *errMsg = "incorrect argument list syntax";
    	    return NULL;
    	}
    }
    
    /* remove the end paren */
    (*inPtr)++;
    
    /* allocate an arglist data structure to return */
    argL = (argList *)XtMalloc(sizeof(argList));
    if (nArgs == 0) {
    	argL->args = NULL;
    } else {
	argL->args = (char **)XtMalloc(sizeof(char *) * nArgs);
	memcpy(argL->args, args, sizeof(char *) * nArgs);
    }
    argL->nArgs = nArgs;
    return argL;
}

static void freeArgList(argList *argL)
{
    int i;
    
    for (i=0; i<argL->nArgs; i++)
    	XtFree(argL->args[i]);
    if (argL->nArgs != 0)
    	XtFree((char *)argL->args);
    XtFree((char *)argL);
}

/*
** parse an individual argument in an argument list.  Anything between
** double quotes is acceptable.  Returns allocated string containing
** argument minus quotes if successful.  Otherwise returns NULL with
** (statically allocated) message in "errMsg".
*/
static char *parseArg(char **inPtr, char **errMsg)
{
    char *outStr, *outPtr, *c;
    int escaped = False;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    /* look for initial quote */
    if (**inPtr == '\0') {
    	*errMsg = "end of macro while parsing argument";
    	return NULL;
    } else if (**inPtr != '\"') {
    	*errMsg = "bad argument";
    	return NULL;
    }
    (*inPtr)++;
    
    /* calculate max length and allocate returned string */
    for (c= *inPtr; ; c++) {
    	if (*c == '\0') {
    	    *errMsg = "end of macro while parsing argument";
    	    return NULL;
    	} else if (*c == '\\' && !escaped)
    	    escaped = True;
    	else if (*c == '"' && !escaped)
    	    break;
    	else
    	    escaped = False;
    }
    outStr = XtMalloc(c - *inPtr + 1);
    
    /* copy string up to end quote */
    outPtr = outStr;
    while (True) {
    	if (**inPtr == '\"')
    	    break;
    	else if (**inPtr == '\\')
    	    (*inPtr)++;
    	*outPtr++ = *(*inPtr)++;
    }
    *outPtr = '\0';
    
    /* return the argument in outStr */
    (*inPtr)++;
    return outStr;
}

/*
** Replace double quote characters in string with \" and backslashes with \\.
** Returns an allocated string which must be freed by the caller with XtFree.
*/
static char *escapeQuotes(char *string)
{
    char *c, *outStr, *outPtr;
    int length = 0;

    /* calculate length and allocate returned string */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '"' || *c == '\\')
    	    length++;
    	length++;
    }
    outStr = XtMalloc(length + 1);
    outPtr = outStr;
    
    /* add backslashes */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '"' || *c == '\\')
    	    *outPtr++ = '\\';
    	*outPtr++ = *c;
    }
    *outPtr = '\0';
    return outStr;
}

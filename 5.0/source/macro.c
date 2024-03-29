#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#ifdef VMS
#include "../util/VMSparam.h"
#include <types.h>
#include <stat.h>
#include <unixio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#endif /*VMS*/
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleB.h>
#include <Xm/DialogS.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "macro.h"
#include "preferences.h"
#include "interpret.h"
#include "parse.h"
#include "search.h"
#include "shell.h"
#include "userCmds.h"
#include "selection.h"

#define AUTO_LOAD_MACRO_FILE_NAME ".neditmacro"
	
/* Maximum number of actions in a macro and args in 
   an action (to simplify the reader) */
#define MAX_MACRO_ACTIONS 1024
#define MAX_ACTION_ARGS 40

/* How long to wait (msec) before putting up Macro Command banner */
#define BANNER_WAIT_TIME 6000

/* Data attached to window during shell command execution with
   information for controling and communicating with the process */
typedef struct {
    XtIntervalId bannerTimeoutID;
    XtWorkProcId continueWorkProcID;
    char bannerIsUp;
    Program *program;
    RestartData *context;
    Widget dialog;
} macroCmdInfo;

static void cancelLearn(void);
static void runMacro(WindowInfo *window, Program *prog);
static void finishMacroCmdExecution(WindowInfo *window);
static void repeatOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void repeatCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void learnActionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams);
static void lastActionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams);
char *actionToString(char *actionName, XEvent *event, String *params,
	Cardinal numParams);
static int isMouseAction(char *action);
static int isRedundantAction(char *action);
static int isIgnoredAction(char *action);
static int readCheckMacroString(Widget dialogParent, char *string,
	WindowInfo *runWindow, char *errIn, char **errPos);
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id);
static Boolean continueWorkProc(XtPointer clientData);
static int escapeStringChars(char *fromString, char *toString);
static int escapedStringLength(char *string);
static int lengthMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int minMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int maxMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int focusWindowMS(WindowInfo *window, DataValue *argList, int nArgs,
      DataValue *result, char **errMsg);
static int getRangeMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int getCharacterMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int replaceRangeMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int replaceSelectionMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int getSelectionMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int replaceInStringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int replaceSubstringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int readFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int writeFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int appendFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int writeOrAppendFile(int append, WindowInfo *window,
    	DataValue *argList, int nArgs, DataValue *result, char **errMsg);
static int substringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int searchMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int searchStringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int setCursorPosMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int beepMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectRectangleMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int tPrintMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int shellCmdMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int dialogMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static void dialogBtnCB(Widget w, XtPointer clientData, XtPointer callData);
static void dialogCloseCB(Widget w, XtPointer clientData, XtPointer callData);
static int stringDialogMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static void stringDialogBtnCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static void stringDialogCloseCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static int cursorMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int fileNameMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int filePathMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int lengthMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectionStartMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectionEndMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectionLeftMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int selectionRightMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int wrapMarginMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int tabDistMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int useTabsMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int languageModeMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg);
static int readSearchArgs(DataValue *argList, int nArgs, int*searchDirection,
	int *searchType, int *wrap, char **errMsg);
static int wrongNArgsErr(char **errMsg);
static int tooFewArgsErr(char **errMsg);
static int readIntArg(DataValue dv, int *result, char **errMsg);
static int readStringArg(DataValue dv, char **result, char *stringStorage,
    	char **errMsg);

/* Built-in subroutines and variables for the macro language */
#define N_MACRO_SUBRS 25
static BuiltInSubr MacroSubrs[N_MACRO_SUBRS] = {lengthMS, getRangeMS, tPrintMS,
    	dialogMS, stringDialogMS, replaceRangeMS, replaceSelectionMS,
    	setCursorPosMS, getCharacterMS, minMS, maxMS, searchMS,
    	searchStringMS, substringMS, replaceSubstringMS, readFileMS,
    	writeFileMS, appendFileMS, beepMS, getSelectionMS,
	replaceInStringMS, selectMS, selectRectangleMS, focusWindowMS,
	shellCmdMS};
static char *MacroSubrNames[N_MACRO_SUBRS] = {"length", "get_range", "t_print",
    	"dialog", "string_dialog", "replace_range", "replace_selection",
    	"set_cursor_pos", "get_character", "min", "max", "search",
        "search_string", "substring", "replace_substring", "read_file",
        "write_file", "append_file", "beep", "get_selection",
	"replace_in_string", "select", "select_rectangle", "focus_window",
	"shell_command"};
#define N_SPECIAL_VARS 12
static BuiltInSubr SpecialVars[N_SPECIAL_VARS] = {cursorMV, fileNameMV,
    	filePathMV, lengthMV, selectionStartMV, selectionEndMV,
    	selectionLeftMV, selectionRightMV, wrapMarginMV, tabDistMV,
	useTabsMV, languageModeMV};
static char *SpecialVarNames[N_SPECIAL_VARS] = {"$cursor", "$file_name",
    	"$file_path", "$text_length", "$selection_start", "$selection_end",
    	"$selection_left", "$selection_right", "$wrap_margin", "$tab_dist",
	"$use_tabs", "$language_mode"};

/* Global symbols for returning values from built-in functions */
#define N_RETURN_GLOBALS 4
enum retGlobalSyms {STRING_DIALOG_BUTTON, SEARCH_END, READ_STATUS,
	SHELL_CMD_STATUS};
static char *ReturnGlobalNames[N_RETURN_GLOBALS] = {"$string_dialog_button",
    	"$search_end", "$read_status", "$shell_cmd_status"};
static Symbol *ReturnGlobals[N_RETURN_GLOBALS];

/* List of actions not useful when learning a macro sequence (also see below) */
static char* IgnoredActions[] = {"focusIn", "focusOut"};

/* List of actions intended to be attached to mouse buttons, which the user
   must be warned can't be recorded in a learn/replay sequence */
static char* MouseActions[] = {"grab_focus", "extend_adjust", "extend_start",
	"extend_end", "secondary_or_drag_adjust", "secondary_adjust",
	"secondary_or_drag_start", "secondary_start", "move_destination",
	"move_to", "move_to_or_end_drag", "copy_to", "copy_to_or_end_drag",
	"exchange", "process_bdrag", "mouse_pan"};

/* List of actions to not record because they 
   generate further actions, more suitable for recording */
static char* RedundantActions[] = {"open_dialog", "save_as_dialog",
	"include_file_dialog", "load_tags_file_dialog", "find_dialog",
	"replace_dialog", "goto_line_number_dialog", "control_code_dialog",
	"filter_selection_dialog", "execute_command_dialog", "repeat_dialog",
	"revert_to_saved_dialog"};

/* The last command executed (used by the Repeat command) */
static char *LastCommand = NULL;

/* The current macro to execute on Replay command */
static char *ReplayMacro = NULL;

/* Buffer where macro commands are recorded in Learn mode */
static textBuffer *MacroRecordBuf = NULL;

/* Action Hook id for recording actions for Learn mode */
static XtActionHookId MacroRecordActionHook = 0;

/* Window where macro recording is taking place */
static WindowInfo *MacroRecordWindow = NULL;

/* Arrays for translating escape characters in escapeStringChars */
static char ReplaceChars[] = "\\\"ntbrfav";
static char EscapeChars[] = "\\\"\n\t\b\r\f\a\v";

/* Widgets and global data for Repeat dialog */
static int DoneWithRepeatDialog;
static WindowInfo *RepeatDialogForWindow;
static Widget RepeatText, RepeatLastCmdToggle, RepeatLearnToggle;

/*
** Install built-in macro subroutines and special variables for accessing
** editor information
*/
void RegisterMacroSubroutines(void)
{
    static DataValue subrPtr = {NO_TAG, {0}}, noValue = {NO_TAG, {0}};
    int i;
    
    /* Install symbols for built-in routines and variables, with pointers
       to the appropriate c routines to do the work */
    for (i=0; i<N_MACRO_SUBRS; i++) {
    	subrPtr.val.ptr = (void *)MacroSubrs[i];
    	InstallSymbol(MacroSubrNames[i], C_FUNCTION_SYM, subrPtr);
    }
    for (i=0; i<N_SPECIAL_VARS; i++) {
    	subrPtr.val.ptr = (void *)SpecialVars[i];
    	InstallSymbol(SpecialVarNames[i], PROC_VALUE_SYM, subrPtr);
    }
    
    /* Define global variables used for return values, remember their
       locations so they can be set without a LookupSymbol call */
    for (i=0; i<N_RETURN_GLOBALS; i++)
    	ReturnGlobals[i] = InstallSymbol(ReturnGlobalNames[i], GLOBAL_SYM,
    	    	noValue);
}

void BeginLearn(WindowInfo *window)
{
    WindowInfo *win;
    XmString s;
    
    /* If we're already in learn mode, return */
    if (MacroRecordActionHook != 0)
    	return;
    
    /* dim the inappropriate menus and items, and undim finish and cancel */
    for (win=WindowList; win!=NULL; win=win->next)
	XtSetSensitive(win->learnItem, False);
    XtSetSensitive(window->finishLearnItem, True);
    XtVaSetValues(window->cancelMacroItem, XmNlabelString,
    	    s=XmStringCreateSimple("Cancel Learn"), 0);
    XmStringFree(s);
    XtSetSensitive(window->cancelMacroItem, True);
    
    /* Mark the window where learn mode is happening */
    MacroRecordWindow = window;
    
    /* Allocate a text buffer for accumulating the macro strings */
    MacroRecordBuf = BufCreate();
    
    /* Add the action hook for recording the actions */
    MacroRecordActionHook =
    	    XtAppAddActionHook(XtWidgetToApplicationContext(window->shell),
    	    learnActionHook, window);
    
    /* Put up the learn-mode banner */
    SetModeMessage(window,
    	    "Learn Mode -- Press Alt+K to finish, Ctrl+. to cancel");
}

void AddLastCommandActionHook(XtAppContext context)
{
    XtAppAddActionHook(context, lastActionHook, NULL);
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
    XtSetSensitive(MacroRecordWindow->cancelMacroItem, False);
    
    /* Undim the replay and paste-macro buttons */
    for (win=WindowList; win!=NULL; win=win->next)
    	XtSetSensitive(win->replayItem, True);
    DimPasteReplayBtns(True);
    
    /* Clear learn-mode banner */
    ClearModeMessage(MacroRecordWindow);
}

/*
** Cancel Learn mode, or macro execution (they're bound to the same menu item)
*/
void CancelMacroOrLearn(WindowInfo *window)
{
    if (MacroRecordActionHook != 0)
    	cancelLearn();
    else if (window->macroCmdData != NULL)
    	AbortMacroCommand(window);
}

static void cancelLearn(void)
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
    XtSetSensitive(MacroRecordWindow->cancelMacroItem, False);
    
    /* Clear learn-mode banner */
    ClearModeMessage(MacroRecordWindow);
}

/*
** Execute the learn/replay sequence stored in "window"
*/
void Replay(WindowInfo *window)
{
    Program *prog;
    char *errMsg, *stoppedAt;
    
    if (ReplayMacro == NULL)
    	return;
    
    /* Parse the replay macro (it's stored in text form) and compile it into
       an executable program "prog" */
    prog = ParseMacro(ReplayMacro, &errMsg, &stoppedAt);
    if (prog == NULL) {
    	fprintf(stderr,
    		"NEdit internal error, learn/replay macro syntax error: %s\n",
    		errMsg);
    	return;
    }

    /* run the executable program */
    runMacro(window, prog);
}

/*
** Read the .neditmacro file if one exists
*/
void ReadMacroInitFile(WindowInfo *window)
{
    char fullName[MAXPATHLEN];
    
#ifdef VMS
    sprintf(fullName, "%s%s", "SYS$LOGIN:", AUTO_LOAD_MACRO_FILE_NAME);
#else
    sprintf(fullName, "%s/%s", getenv("HOME"), AUTO_LOAD_MACRO_FILE_NAME);
#endif /*VMS*/
    ReadMacroFile(window, fullName, False);
}

/*
** Read an NEdit macro file.  Extends the syntax of the macro parser with
** define keyword, and allows intermixing of defines with immediate actions.
*/
int ReadMacroFile(WindowInfo *window, char *fileName, int warnNotExist)
{
    struct stat statbuf;
    FILE *fp;
    int fileLen, readLen, result;
    char *fileString;
    
    /* Read the whole file into fileString */
    if ((fp = fopen(fileName, "r")) == NULL) {
    	if (warnNotExist)
	    DialogF(DF_ERR, window->shell, 1, "Can't open macro file %s",
    	    	    "dismiss", fileName);
    	return False;
    }
    if (fstat(fileno(fp), &statbuf) != 0) {
	DialogF(DF_ERR, window->shell, 1, "Can't read macro file %s",
    	    	"dismiss", fileName);
	fclose(fp);
	return False;
    }
    fileLen = statbuf.st_size;
    fileString = XtMalloc(fileLen+1);  /* +1 = space for null */
    readLen = fread(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "Error reading macro file %s: %s",
    	    	"dismiss", fileName,
#ifdef VMS
    	    	strerror(errno, vaxc$errno));
#else
    	    	strerror(errno));
#endif
	XtFree(fileString);
	fclose(fp);
	return False;
    }
    fclose(fp);
    fileString[readLen] = 0;

    /* Parse fileString */
    result = readCheckMacroString(window->shell, fileString, window, fileName,
	    NULL);
    XtFree(fileString);
    return result;
}

/*
** Parse and execute a macro string including macro definitions.  Report
** parsing errors in a dialog posted over window->shell.
*/
int ReadMacroString(WindowInfo *window, char *string, char *errIn)
{   
    return readCheckMacroString(window->shell, string, window, errIn, NULL);
}  

/*
** Check a macro string containing definitions for errors.  Returns True
** if macro compiled successfully.  Returns False and puts up
** a dialog explaining if macro did not compile successfully.
*/  
int CheckMacroString(Widget dialogParent, char *string, char *errIn,
	char **errPos)
{
    return readCheckMacroString(dialogParent, string, NULL, errIn, errPos);
}    

/*
** Parse and optionally execute a macro string including macro definitions.
** Report parsing errors in a dialog posted over dialogParent, using the
** string errIn to identify the entity being parsed (filename, macro string,
** etc.).  If runWindow is specified, runs the macro against the window.  If
** runWindow is passed as NULL, does parse only.  If errPos is non-null, 
** returns a pointer to the error location in the string.
*/
static int readCheckMacroString(Widget dialogParent, char *string,
	WindowInfo *runWindow, char *errIn, char **errPos)
{
    char *stoppedAt, *inPtr, *namePtr, *errMsg;
    char subrName[MAX_SYM_LEN];
    Program *prog;
    Symbol *sym;
    DataValue subrPtr;
    
    inPtr = string;
    while (*inPtr != '\0') {
    	
    	/* skip over white space and comments */
	while (*inPtr==' ' || *inPtr=='\t' || *inPtr=='\n'|| *inPtr=='#') {
	    if (*inPtr == '#')
	    	while (*inPtr != '\n' && *inPtr != '\0') inPtr++;
	    else
	    	inPtr++;
	}
	if (*inPtr == '\0')
	    break;
	
	/* look for define keyword, and compile and store defined routines */
	if (!strncmp(inPtr, "define", 6) && (inPtr[6]==' ' || inPtr[6]=='\t')) {
	    inPtr += 6;
	    inPtr += strspn(inPtr, " \t\n");
	    namePtr = subrName;
	    while (isalnum(*inPtr) || *inPtr == '_')
	    	*namePtr++ = *inPtr++;
	    *namePtr = '\0';
	    inPtr += strspn(inPtr, " \t\n");
	    if (*inPtr != '{') {
	    	if (errPos != NULL) *errPos = stoppedAt;
		return ParseError(dialogParent, string, inPtr,
	    	    	errIn, "expected '{'");
	    }
	    prog = ParseMacro(inPtr, &errMsg, &stoppedAt);
	    if (prog == NULL) {
	    	if (errPos != NULL) *errPos = stoppedAt;
	    	return ParseError(dialogParent, string, stoppedAt,
	    	    	errIn, errMsg);
	    }
	    if (runWindow != NULL) {
		sym = LookupSymbol(subrName);
		if (sym == NULL) {
		    subrPtr.val.ptr = prog;
		    sym = InstallSymbol(subrName, MACRO_FUNCTION_SYM, subrPtr);
		} else {
	    	    if (sym->type == MACRO_FUNCTION_SYM)
		    	FreeProgram((Program *)sym->value.val.ptr);
		    else
			sym->type = MACRO_FUNCTION_SYM;
	    	    sym->value.val.ptr = prog;
		}
	    }
	    inPtr = stoppedAt;
	
	/* parse and execute immediate (outside of any define) macro commands
	   and WAIT for them to finish executing before proceeding */
	} else {
	    prog = ParseMacro(inPtr, &errMsg, &stoppedAt);
	    if (prog == NULL) {
	    	if (errPos != NULL) *errPos = stoppedAt;
    	    	return ParseError(dialogParent, string, stoppedAt,
	    	    	errIn, errMsg);
	    }
	    if (runWindow != NULL) {
    	    	XEvent nextEvent;	 
	        runMacro(runWindow, prog);
		while (runWindow->macroCmdData != NULL) {
		    XtAppNextEvent(XtWidgetToApplicationContext(
			    runWindow->shell),  &nextEvent);
		    XtDispatchEvent(&nextEvent);
		}
	    }
	    inPtr = stoppedAt;
    	}
    }
    return True;
}

/*
** Run a pre-compiled macro, changing the interface state to reflect that
** a macro is running, and handling preemption, resumption, and cancellation.
** frees prog when macro execution is complete;
*/
static void runMacro(WindowInfo *window, Program *prog)
{
    DataValue result;
    char *errMsg;
    int stat;
    macroCmdInfo *cmdData;
    XmString s;
    
    /* If a macro is already running, just call the program as a subroutine,
       instead of starting a new one, so we don't have to keep a separate
       context, and the macros will serialize themselves automatically */
    if (window->macroCmdData != NULL) {
    	RunMacroAsSubrCall(prog);
	return;
    }
    
    /* put up a watch cursor over the waiting window */
    BeginWait(window->shell);
    
    /* enable the cancel menu item */
    XtVaSetValues(window->cancelMacroItem, XmNlabelString,
    	    s=XmStringCreateSimple("Cancel Macro"), 0);
    XmStringFree(s);
    XtSetSensitive(window->cancelMacroItem, True);

    /* Create a data structure for passing macro execution information around
       amongst the callback routines which will process i/o and completion */
    cmdData = (macroCmdInfo *)XtMalloc(sizeof(macroCmdInfo));
    window->macroCmdData = cmdData;
    cmdData->bannerIsUp = False;
    cmdData->program = prog;
    cmdData->context = NULL;
    cmdData->continueWorkProcID = 0;
    cmdData->dialog = NULL;
    
    /* Set up timer proc for putting up banner when macro takes too long */
    cmdData->bannerTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), BANNER_WAIT_TIME,
    	    bannerTimeoutProc, window);
    
    /* Begin macro execution */
    stat = ExecuteMacro(window, prog, 0, NULL, &result, &cmdData->context,
    	    &errMsg);
    if (stat == MACRO_ERROR) {
    	finishMacroCmdExecution(window);
    	DialogF(DF_ERR, window->shell, 1, "Error executing macro: %s",
    	    	"Dismiss", errMsg);
    	return;
    }
    if (stat == MACRO_DONE) {
    	finishMacroCmdExecution(window);
    	return;
    }
    if (stat == MACRO_TIME_LIMIT) {
	ResumeMacroExecution(window);
	return;
    }
    /* (stat == MACRO_PREEMPT) Macro was preempted */
}

/*
** Continue with macro execution after preemption.  Called by the routines
** whose actions cause preemption when they have completed their lengthy tasks.
** Re-establishes macro execution work proc.  Window must be the window in
** which the macro is executing (the window to which macroCmdData is attached),
** and not the window to which operations are focused.
*/
void ResumeMacroExecution(WindowInfo *window)
{
    macroCmdInfo *cmdData = (macroCmdInfo *)window->macroCmdData;
    
    if (cmdData != NULL)
	cmdData->continueWorkProcID = XtAppAddWorkProc(
	    	XtWidgetToApplicationContext(window->shell),
	    	continueWorkProc, window);
}

/*
** Cancel the macro command in progress
*/
void AbortMacroCommand(WindowInfo *window)
{
    if (window->macroCmdData == NULL)
    	return;
    
    /* If there's both a macro and a shell command executing, the shell command
       must have been called from the macro.  When called from a macro, shell
       commands don't put up cancellation controls of their own, but rely
       instead on the macro cancellation mechanism (here) */
#ifndef VMS
    if (window->shellCmdData != NULL)
    	AbortShellCommand(window);
#endif
    
    /* Free the continuation */
    FreeRestartData(((macroCmdInfo *)window->macroCmdData)->context);
    
    /* Kill the macro command */
    finishMacroCmdExecution(window);
}

/*
** Clean up after the execution of a macro command: free memory, and restore
** the user interface state.
*/
static void finishMacroCmdExecution(WindowInfo *window)
{
    macroCmdInfo *cmdData = window->macroCmdData;
    XmString s;
    WindowInfo *win;

    /* Cancel pending timeout and work proc */
    if (cmdData->bannerTimeoutID != 0)
    	XtRemoveTimeOut(cmdData->bannerTimeoutID);
    if (cmdData->continueWorkProcID != 0)
    	XtRemoveWorkProc(cmdData->continueWorkProcID);
    
    /* Clean up waiting-for-macro-command-to-complete mode */
    EndWait(window->shell);
    XtVaSetValues(window->cancelMacroItem, XmNlabelString,
    	    s=XmStringCreateSimple("Cancel Learn"), 0);
    XmStringFree(s);
    XtSetSensitive(window->cancelMacroItem, False);
    if (cmdData->bannerIsUp)
    	ClearModeMessage(window);

    /* If a dialog was up, get rid of it */
    if (cmdData->dialog != NULL)
    	XtDestroyWidget(XtParent(cmdData->dialog));

    /* Free execution information */
    FreeProgram(cmdData->program);
    XtFree((char *)cmdData);
    window->macroCmdData = NULL;

    /* If no other macros are executing, do garbage collection */
    for (win=WindowList; win!=NULL; win=win->next)
	if (window->macroCmdData != NULL)
	    return;
    GarbageCollectStrings();
}

/*
** Executes macro string "macro" using the lastFocus pane in "window".
** Reports errors via a dialog posted over "window", integrating the name
** "errInName" into the message to help identify the source of the error.
*/
void DoMacro(WindowInfo *window, char *macro, char *errInName)
{
    Program *prog;
    char *errMsg, *stoppedAt, *tMacro;
    int macroLen;
    
    /* Add a terminating newline (which command line users are likely to omit
       since they are typically invoking a single routine) */
    macroLen = strlen(macro);
    tMacro = XtMalloc(strlen(macro)+2);
    strncpy(tMacro, macro, macroLen);
    tMacro[macroLen] = '\n';
    tMacro[macroLen+1] = '\0';
    
    /* Parse the macro and report errors if it fails */
    prog = ParseMacro(tMacro, &errMsg, &stoppedAt);
    if (prog == NULL) {
    	ParseError(window->shell, tMacro, stoppedAt, errInName, errMsg);
	XtFree(tMacro);
    	return;
    }
    XtFree(tMacro);

    /* run the executable program (prog is freed upon completion) */
    runMacro(window, prog);
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
** Present the user a dialog for "Repeat" command
*/
void RepeatDialog(WindowInfo *window)
{
    Widget form, selBox, timesLabel, radioBox;
    Arg selBoxArgs[2];
    char *lastCmdLabel, *parenChar;
    XmString s1;
    int cmdNameLen;

    if (LastCommand == NULL) {
    	DialogF(DF_WARN, window->shell, 1,
	    	"No previous commands or learn/\nreplay sequences to repeat",
		"Dismiss");
	return;
    }
    
    /* make a label for the Last command item of the dialog, which includes
       the last executed action name */
    parenChar = strchr(LastCommand, '(');
    if (parenChar == NULL)
	return;
    cmdNameLen = parenChar-LastCommand;
    lastCmdLabel = XtMalloc(16 + cmdNameLen);
    strcpy(lastCmdLabel, "Last command (");
    strncpy(&lastCmdLabel[14], LastCommand, cmdNameLen);
    strcpy(&lastCmdLabel[14 + cmdNameLen], ")");
    
    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(window->shell, "repeat", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, repeatOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, repeatCancelCB, NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Repeat", 0);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, 0);

    radioBox = XtVaCreateManagedWidget("radioBox", xmRowColumnWidgetClass, form,
    	    XmNradioBehavior, True,
	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM, 0);
    RepeatLastCmdToggle = XtVaCreateManagedWidget("lastCmdToggle",
    	    xmToggleButtonWidgetClass, radioBox, XmNset, True,
	    XmNlabelString, s1=XmStringCreateSimple(lastCmdLabel),
	    XmNmnemonic, 'c', 0);
    XmStringFree(s1);
    XtFree(lastCmdLabel);
    RepeatLearnToggle = XtVaCreateManagedWidget("learnReplayToggle",
    	    xmToggleButtonWidgetClass, radioBox, XmNset, False,
	    XmNlabelString,
	    	s1=XmStringCreateSimple("Learn/Replay sequence"),
	    XmNmnemonic, 'l',
	    XmNsensitive, ReplayMacro != NULL, 0);
    XmStringFree(s1);

    RepeatText = XtVaCreateManagedWidget("repeatText", xmTextWidgetClass, form,
    	    XmNcolumns, 5,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, radioBox,
    	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 50, 0);
    RemapDeleteKey(RepeatText);
    timesLabel = XtVaCreateManagedWidget("timesLabel",
    	    xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("Times"),
	    XmNmnemonic, 'T',
    	    XmNuserData, RepeatText,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, radioBox,
    	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 50,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, RepeatText, 0);
    XmStringFree(s1);

    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* Set initial focus */
#if XmVersion >= 1002
    XtVaSetValues(form, XmNinitialFocus, RepeatText, 0);
#endif
    
    /* put up dialog and wait for user to press ok or cancel */
    RepeatDialogForWindow = window;
    DoneWithRepeatDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithRepeatDialog)
        XtAppProcessEvent(XtWidgetToApplicationContext(window->shell), XtIMAll);
    
    XtDestroyWidget(selBox);
}

static void repeatOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int nTimes;
    char nTimesStr[25];
    char *params[2];
    
    /* Find out from the dialog which command to repeat, and how many times to
       repeat it, and call the action routine repeat_macro to do the work */
    if (GetIntTextWarn(RepeatText, &nTimes, "number of times", True) != 
	    TEXT_READ_OK)
   	return;
    sprintf(nTimesStr, "%d", nTimes);
    params[0] = nTimesStr;
    if (XmToggleButtonGetState(RepeatLastCmdToggle)) {
	if (LastCommand == NULL)
	    return;
	params[1] = CopyAllocatedString(LastCommand);
    } else {
	if (ReplayMacro == NULL)
	    return;
	params[1] = CopyAllocatedString(ReplayMacro);
    }
    XtCallActionProc(RepeatDialogForWindow->lastFocus, "repeat_macro",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 2);
    XtFree(params[1]);
    DoneWithRepeatDialog = True;
}

static void repeatCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithRepeatDialog = True;
}

/*
** Dispatches a macro to which repeats macro command in "command" "nTimes".
** Note that as with most macro routines, this returns BEFORE the macro is
** finished executing
*/
void RepeatMacro(WindowInfo *window, char *command, int nTimes)
{
    Program *prog;
    char *errMsg, *stoppedAt, *loopedCmd;

    if (command == NULL)
	return;
    
    /* Wrap a for loop and counter around the command */
    loopedCmd = XtMalloc(strlen(command) + 22);
    sprintf(loopedCmd, "for(i=0;i<%d;i++){%s\n}\n", nTimes, command);
    
    /* Parse the resulting macro into an executable program "prog" */
    prog = ParseMacro(loopedCmd, &errMsg, &stoppedAt);
    if (prog == NULL) {
	fprintf(stderr, "NEdit internal error, repeat macro syntax wrong: %s\n",
    		errMsg);
    	return;
    }
    XtFree(loopedCmd);

    /* run the executable program */
    runMacro(RepeatDialogForWindow, prog);
}

/*
** Macro recording action hook for Learn/Replay, added temporarily during
** learn.
*/
static void learnActionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams)
{
    WindowInfo *window;
    int i;
    char *actionString;
    
    /* Select only actions in text panes in the window for which this
       action hook is recording macros (from clientData). */
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
    if (window == NULL || window != (WindowInfo *)clientData)
    	return;
    
    /* beep on un-recordable operations which require a mouse position, to
       remind the user that the action was not recorded */
    if (isMouseAction(actionName)) {
    	XBell(XtDisplay(w), 0);
    	return;
    }
    
    /* Record the action and its parameters */
    actionString = actionToString(actionName, event, params, *numParams);
    if (actionString != NULL) {
	BufInsert(MacroRecordBuf, MacroRecordBuf->length, actionString);
	XtFree(actionString);
    }
}

/*
** Permanent action hook for remembering last action for possible replay
*/
static void lastActionHook(Widget w, XtPointer clientData, String actionName,
	XEvent *event, String *params, Cardinal *numParams)
{
    WindowInfo *window;
    int i;
    char *actionString;
    
    /* Find the window to which this action belongs */
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
    if (window == NULL)
    	return;

    /* The last action is recorded for the benefit of repeating the last
       action.  Don't record repeat_macro and wipe out the real action */
    if (!strcmp(actionName, "repeat_macro"))
	return;
        
    /* Record the action and its parameters */
    actionString = actionToString(actionName, event, params, *numParams);
    if (actionString != NULL) {
	if (LastCommand != NULL)
	    XtFree(LastCommand);
	LastCommand = actionString;
    }
}

/*
** Create a macro string to represent an invocation of an action routine.
** Returns NULL for non-operational or un-recordable actions.
*/
char *actionToString(char *actionName, XEvent *event, String *params,
	Cardinal numParams)
{
    char chars[20], *charList[1], *outStr, *outPtr;
    KeySym keysym;
    int i, nChars, nParams, length, nameLength;
    
    if (isIgnoredAction(actionName) || isRedundantAction(actionName) ||
	    isMouseAction(actionName))
    	return NULL;
    
    /* Convert self_insert actions, to insert_string */
    if (!strcmp(actionName, "self_insert") ||
    	    !strcmp(actionName, "self-insert")) {
    	actionName = "insert_string";
	nChars = XLookupString((XKeyEvent *)event, chars, 19, &keysym, NULL);
	if (nChars == 0)
	    return NULL;
    	chars[nChars] = '\0';
    	charList[0] = chars;
    	params = charList;
    	nParams = 1;
    } else
    	nParams = numParams;
    	
    /* Figure out the length of string required */
    nameLength = strlen(actionName);
    length = nameLength + 3;
    for (i=0; i<nParams; i++)
	length += escapedStringLength(params[i]) + 4;
    
    /* Allocate the string and copy the information to it */
    outPtr = outStr = XtMalloc(length + 1);
    strcpy(outPtr, actionName);
    outPtr += nameLength;
    *outPtr++ = '(';
    for (i=0; i<nParams; i++) {
	*outPtr++ = '\"';
	outPtr += escapeStringChars(params[i], outPtr);
	*outPtr++ = '\"'; *outPtr++ = ','; *outPtr++ = ' ';
    }
    if (nParams != 0)
	outPtr -= 2;
    *outPtr++ = ')'; *outPtr++ = '\n'; *outPtr++ = '\0';
    return outStr;
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
** Timer proc for putting up the "Macro Command in Progress" banner if
** the process is taking too long.
*/
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    
    cmdData->bannerIsUp = True;
    SetModeMessage(window,
    	    "Macro Command in Progress -- Press Ctrl+. to Cancel");
    cmdData->bannerTimeoutID = 0;
}

/*
** Work proc for continuing execution of a preempted macro.
**
** Xt WorkProcs are designed to run first-in first-out, which makes them
** very bad at sharing time between competing tasks.  For this reason, it's
** usually bad to use work procs anywhere where their execution is likely to
** overlap.  Using a work proc instead of a timer proc (which I usually
** prefer) here means macros will probably share time badly, but we're more
** interested in making the macros cancelable, and in continuing other work
** than having users run a bunch of them at once together.
*/
static Boolean continueWorkProc(XtPointer clientData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    char *errMsg;
    int stat;
    DataValue result;
    
    stat = ContinueMacro(cmdData->context, &result, &errMsg);
    if (stat == MACRO_ERROR) {
    	finishMacroCmdExecution(window);
    	DialogF(DF_ERR, window->shell, 1, "Error executing macro: %s",
    	    	"Dismiss", errMsg);
    	return True;
    } else if (stat == MACRO_DONE) {
    	finishMacroCmdExecution(window);
    	return True;
    } else if (stat == MACRO_PREEMPT) {
	cmdData->continueWorkProcID = 0;
    	return True;
    }
    
    /* Macro exceeded time slice, re-schedule it */
    if (stat != MACRO_TIME_LIMIT)
    	return True; /* shouldn't happen */
    return False;
}

/*
** Copy fromString to toString replacing special characters in strings, such
** that they can be read back by the macro parser's string reader.  i.e. double
** quotes are replaced by \", backslashes are replaced with \\, C-std control
** characters like \n are replaced with their backslash counterparts.  This
** routine should be kept reasonably in sync with yylex in parse.y.  Companion
** routine escapedStringLength predicts the length needed to write the string
** when it is expanded with the additional characters.  Returns the number
** of characters to which the string expanded.
*/
static int escapeStringChars(char *fromString, char *toString)
{
    char *e, *c, *outPtr = toString;
    
    /* substitute escape sequences */
    for (c=fromString; *c!='\0'; c++) {
    	for (e=EscapeChars; *e!='\0'; e++) {
    	    if (*c == *e) {
    		*outPtr++ = '\\';
    		*outPtr++ = ReplaceChars[e-EscapeChars];
		break;
	    }
	}
	if (*e == '\0')
    	   *outPtr++ = *c;
    }
    *outPtr = '\0';
    return outPtr - toString;
}

/*
** Predict the length of a string needed to hold a copy of "string" with
** special characters replaced with escape sequences by escapeStringChars.
*/
static int escapedStringLength(char *string)
{
    char *c, *e;
    int length = 0;

    /* calculate length and allocate returned string */
    for (c=string; *c!='\0'; c++) {
    	for (e=EscapeChars; *e!='\0'; e++) {
	    if (*c == *e) {
    		length++;
		break;
	    }
	}
    	length++;
    }
    return length;
}

/*
** Built-in macro subroutine for getting the length of a string
*/
static int lengthMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char *string, stringStorage[25];
    
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage, errMsg))
	return False;
    result->tag = INT_TAG;
    result->val.n = strlen(string);
    return True;
}

/*
** Built-in macro subroutines for min and max
*/
static int minMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int minVal, value, i;
    
    if (nArgs == 1)
    	return tooFewArgsErr(errMsg);
    if (!readIntArg(argList[0], &minVal, errMsg))
    	return False;
    for (i=0; i<nArgs; i++) {
	if (!readIntArg(argList[i], &value, errMsg))
    	    return False;
    	minVal = value < minVal ? value : minVal;
    }
    result->tag = INT_TAG;
    result->val.n = minVal;
    return True;
}
static int maxMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int maxVal, value, i;
    
    if (nArgs == 1)
    	return tooFewArgsErr(errMsg);
    if (!readIntArg(argList[0], &maxVal, errMsg))
    	return False;
    for (i=0; i<nArgs; i++) {
	if (!readIntArg(argList[i], &value, errMsg))
    	    return False;
    	maxVal = value > maxVal ? value : maxVal;
    }
    result->tag = INT_TAG;
    result->val.n = maxVal;
    return True;
}

static int focusWindowMS(WindowInfo *window, DataValue *argList, int nArgs,
      DataValue *result, char **errMsg)
{
    char stringStorage[25], *string;
    WindowInfo *w;
    char fullname[MAXPATHLEN];

    /* Read the argument representing the window to focus to, and translate
       it into a pointer to a real WindowInfo */
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage, errMsg))
    	return False;
    else if (!strcmp(string, "last"))
	w = WindowList;
    else if (!strcmp(string, "next"))
	w = window->next;
    else {
	for (w=WindowList; w != NULL; w = w->next) {
	    sprintf(fullname, "%s%s", w->path, w->filename);
	    if (!strcmp(string, fullname))
		break;
	}
    }
    
    /* If no matching window was found, return empty string and do nothing */
    if (w == NULL) {
	result->tag = STRING_TAG;
	result->tag = STRING_TAG;
	result->val.str = AllocString(1);
	result->val.str[0] = '\0';
	return True;
    }

    /* Change the focused window to the requested one */
    SetMacroFocusWindow(w);

    /* Return the name of the window */
    result->tag = STRING_TAG;
    result->val.str = AllocString(strlen(w->path)+strlen(w->filename)+1);
    sprintf(result->val.str, "%s%s", w->path, w->filename);
    return True;
}

/*
** Built-in macro subroutine for getting text from the current window's text
** buffer
*/
static int getRangeMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int from, to;
    textBuffer *buf = window->buffer;
    char *rangeText;
    
    /* Validate arguments and convert to int */
    if (nArgs != 2)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &from, errMsg))
    	return False;
    if (!readIntArg(argList[1], &to, errMsg))
	return False;
    if (from < 0) from = 0;
    if (from > buf->length) from = buf->length;
    if (to < 0) to = 0;
    if (to > buf->length) to = buf->length;
    if (from > to) {int temp = from; from = to; to = temp;}
    
    /* Copy text from buffer (this extra copy could be avoided if textBuf.c
       provided a routine for writing into a pre-allocated string) */
    result->tag = STRING_TAG;
    result->val.str = AllocString(to - from + 1);
    rangeText = BufGetRange(buf, from, to);
    BufUnsubstituteNullChars(rangeText, buf);
    strcpy(result->val.str, rangeText);
    XtFree(rangeText);
    return True;
}

/*
** Built-in macro subroutine for getting a single character at the position
** given, from the current window
*/
static int getCharacterMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int pos;
    textBuffer *buf = window->buffer;
    
    /* Validate argument and convert it to int */
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &pos, errMsg))
    	return False;
    if (pos < 0) pos = 0;
    if (pos > buf->length) pos = buf->length;
    
    /* Return the character in a pre-allocated string) */
    result->tag = STRING_TAG;
    result->val.str =  AllocString(2);
    result->val.str[0] = BufGetCharacter(buf, pos);
    result->val.str[1] = '\0';
    BufUnsubstituteNullChars(result->val.str, buf);
    return True;
}

/*
** Built-in macro subroutine for replacing text in the current window's text
** buffer
*/
static int replaceRangeMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int from, to;
    char stringStorage[25], *string;
    textBuffer *buf = window->buffer;
    
    /* Validate arguments and convert to int */
    if (nArgs != 3)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &from, errMsg))
    	return False;
    if (!readIntArg(argList[1], &to, errMsg))
	return False;
    if (!readStringArg(argList[2], &string, stringStorage, errMsg))
    	return False;
    if (from < 0) from = 0;
    if (from > buf->length) from = buf->length;
    if (to < 0) to = 0;
    if (to > buf->length) to = buf->length;
    if (from > to) {int temp = from; from = to; to = temp;}
    
    /* Don't allow modifications if the window is read-only */
    if (window->readOnly) {
	XBell(XtDisplay(window->shell), 0);
	result->tag = NO_TAG;
	return True;
    }
    
    /* There are no null characters in the string (because macro strings
       still have null termination), but if the string contains the
       character used by the buffer for null substitution, it could
       theoretically become a null.  In the highly unlikely event that
       all of the possible substitution characters in the buffer are used
       up, stop the macro and tell the user of the failure */
    if (!BufSubstituteNullChars(string, strlen(string), window->buffer)) {
	*errMsg = "Too much binary data in file";
	return False;
    }

    /* Do the replace */
    BufReplace(buf, from, to, string);
    result->tag = NO_TAG;
    return True;
}

/*
** Built-in macro subroutine for replacing the primary-selection selected
** text in the current window's text buffer
*/
static int replaceSelectionMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char stringStorage[25], *string;
    
    /* Validate argument and convert to string */
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage, errMsg))
    	return False;
     
    /* There are no null characters in the string (because macro strings
       still have null termination), but if the string contains the
       character used by the buffer for null substitution, it could
       theoretically become a null.  In the highly unlikely event that
       all of the possible substitution characters in the buffer are used
       up, stop the macro and tell the user of the failure */
    if (!BufSubstituteNullChars(string, strlen(string), window->buffer)) {
	*errMsg = "Too much binary data in file";
	return False;
    }
    
    /* Do the replace */
    BufReplaceSelected(window->buffer, string);
    result->tag = NO_TAG;
    return True;
}

/*
** Built-in macro subroutine for getting the text currently selected by
** the primary selection in the current window's text buffer, or in any
** part of screen if "any" argument is given
*/
static int getSelectionMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char *selText;
    XEvent nextEvent;	 

    /* Read argument list to check for "any" keyword, and get the appropriate
       selection */
    if (nArgs != 0 && nArgs != 1)
      	return wrongNArgsErr(errMsg);
    if (nArgs == 1) {
        if (argList[0].tag != STRING_TAG || strcmp(argList[0].val.str, "any")) {
	    *errMsg = "unrecognized argument to %s";
	    return False;
    	}
	selText = GetAnySelection(window);
	if (selText == NULL)
	    selText = XtNewString("");
    } else {
	selText = BufGetSelectionText(window->buffer);
    	BufUnsubstituteNullChars(selText, window->buffer);
    }
	
    /* Return the text as an allocated string */
    result->tag = STRING_TAG;
    result->val.str = AllocString(strlen(selText) + 1);
    strcpy(result->val.str, selText);
    XtFree(selText);
    return True;
}

/*
** Built-in macro subroutine for replacing a substring within another string
*/
static int replaceSubstringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int from, to, length, replaceLen, outLen;
    char stringStorage[2][25], *string, *replStr;
    
    /* Validate arguments and convert to int */
    if (nArgs != 4)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage[1], errMsg))
    	return False;
    if (!readIntArg(argList[1], &from, errMsg))
    	return False;
    if (!readIntArg(argList[2], &to, errMsg))
	return False;
    if (!readStringArg(argList[3], &replStr, stringStorage[1], errMsg))
    	return False;
    length = strlen(string);
    if (from < 0) from = 0;
    if (from > length) from = length;
    if (to < 0) to = 0;
    if (to > length) to = length;
    if (from > to) {int temp = from; from = to; to = temp;}
    
    /* Allocate a new string and do the replacement */
    replaceLen = strlen(replStr);
    outLen = length - (to - from) + replaceLen;
    result->tag = STRING_TAG;
    result->val.str = AllocString(outLen+1);
    strncpy(result->val.str, string, from);
    strncpy(&result->val.str[from], replStr, replaceLen);
    strncpy(&result->val.str[from + replaceLen], &string[to], length - to);
    result->val.str[outLen] = '\0';
    return True;
}

/*
** Built-in macro subroutine for getting a substring of a string
*/
static int substringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int from, to, length;
    char stringStorage[25], *string;
    
    /* Validate arguments and convert to int */
    if (nArgs != 3)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage, errMsg))
    	return False;
    if (!readIntArg(argList[1], &from, errMsg))
    	return False;
    if (!readIntArg(argList[2], &to, errMsg))
	return False;
    length = strlen(string);
    if (from < 0) from = 0;
    if (from > length) from = length;
    if (to < 0) to = 0;
    if (to > length) to = length;
    if (from > to) {int temp = from; from = to; to = temp;}
    
    /* Allocate a new string and copy the sub-string into it */
    result->tag = STRING_TAG;
    result->val.str = AllocString(to - from + 1);
    strncpy(result->val.str, &string[from], to - from);
    result->val.str[to - from] = '\0';
    return True;
}

/*
** Built-in macro subroutine for reading the contents of a text file into
** a string.  On success, returns 1 in $readStatus, and the contents of the
** file as a string in the subroutine return value.  On failure, returns
** the empty string "" and an 0 $readStatus.
*/
static int readFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char stringStorage[25], *name;
    struct stat statbuf;
    FILE *fp;
    int readLen;
    
    /* Validate arguments and convert to int */
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &name, stringStorage, errMsg))
    	return False;
    
    /* Read the whole file into an allocated string */
    if ((fp = fopen(name, "r")) == NULL)
    	goto errorNoClose;
    if (fstat(fileno(fp), &statbuf) != 0)
    	goto error;
    result->tag = STRING_TAG;
    result->val.str = AllocString(statbuf.st_size+1);
    readLen = fread(result->val.str, sizeof(char), statbuf.st_size, fp);
    if (ferror(fp))
	goto error;
    result->val.str[readLen] = '\0';
    fclose(fp);
    
    /* Return the results */
    ReturnGlobals[READ_STATUS]->value.tag = INT_TAG;
    ReturnGlobals[READ_STATUS]->value.val.n = True;
    return True;

error:
    fclose(fp);

errorNoClose:
    ReturnGlobals[READ_STATUS]->value.tag = INT_TAG;
    ReturnGlobals[READ_STATUS]->value.val.n = False;
    result->tag = STRING_TAG;
    result->val.str = AllocString(1);
    result->val.str[0] = '\0';
    return True;
}

/*
** Built-in macro subroutines for writing or appending a string (parameter $1)
** to a file named in parameter $2. Returns 1 on successful write, or 0 if
** unsuccessful.
*/
static int writeFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    return writeOrAppendFile(False, window, argList, nArgs, result, errMsg);
}
static int appendFileMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    return writeOrAppendFile(True, window, argList, nArgs, result, errMsg);
}
static int writeOrAppendFile(int append, WindowInfo *window,
    	DataValue *argList, int nArgs, DataValue *result, char **errMsg)
{
    char stringStorage[2][25], *name, *string;
    FILE *fp;
    
    /* Validate argument */
    if (nArgs != 2)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage[1], errMsg))
    	return False;
    if (!readStringArg(argList[1], &name, stringStorage[0], errMsg))
    	return False;
    
    /* open the file */
    if ((fp = fopen(name, append ? "a" : "w")) == NULL) {
	result->tag = INT_TAG;
	result->val.n = False;
	return True;
    }
    
    /* write the string to the file */
    fwrite(string, sizeof(char), strlen(string), fp);
    if (ferror(fp)) {
	fclose(fp);
	result->tag = INT_TAG;
	result->val.n = False;
	return True;
    }
    fclose(fp);
    
    /* return the status */
    result->tag = INT_TAG;
    result->val.n = True;
    return True;
}

/*
** Built-in macro subroutine for searching silently in a window without
** dialogs, beeps, or changes to the selection.  Arguments are: $1: string to
** search for, $2: starting position. Optional arguments may include the
** strings: "wrap" to make the search wrap around the beginning or end of the
** string, "backward" or "forward" to change the search direction ("forward" is
** the default), "literal", "case" or "regex" to change the search type
** (default is "literal").
**
** Returns the starting position of the match, or -1 if nothing matched.
** also returns the ending position of the match in $searchEndPos
*/
static int searchMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    DataValue newArgList[9];
    int retVal;
    
    /* Use the search string routine, by adding the buffer contents as
       the string argument */
    if (nArgs > 8)
    	return wrongNArgsErr(errMsg);
    newArgList[0].tag = STRING_TAG;
    newArgList[0].val.str = BufGetAll(window->buffer);
    memcpy(&newArgList[1], argList, nArgs * sizeof(DataValue));
    retVal = searchStringMS(window, newArgList, nArgs+1, result, errMsg);
    XtFree(newArgList[0].val.str);
    return retVal;
}

/*
** Built-in macro subroutine for searching a string.  Arguments are $1:
** string to search in, $2: string to search for, $3: starting position.
** Optional arguments may include the strings: "wrap" to make the search
** wrap around the beginning or end of the string, "backward" or "forward"
** to change the search direction ("forward" is the default), "literal",
** "case" or "regex" to change the search type (default is "literal").
**
** Returns the starting position of the match, or -1 if nothing matched.
** also returns the ending position of the match in $searchEndPos
*/
static int searchStringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int beginPos, wrap, direction, found, foundStart, foundEnd, type;
    char stringStorage[2][25], *string, *searchStr;
    
    /* Validate arguments and convert to proper types */
    if (nArgs < 3)
    	return tooFewArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage[0], errMsg))
    	return False;
    if (!readStringArg(argList[1], &searchStr, stringStorage[1], errMsg))
    	return False;
    if (!readIntArg(argList[2], &beginPos, errMsg))
    	return False;
    if (!readSearchArgs(&argList[3], nArgs-3, &direction, &type, &wrap, errMsg))
    	return False;
    
    /* Do the search */
    found = SearchString(string, searchStr, direction, type, wrap,
    	    beginPos, &foundStart, &foundEnd, GetWindowDelimiters(window));
    
    /* Return the results */
    ReturnGlobals[SEARCH_END]->value.tag = INT_TAG;
    ReturnGlobals[SEARCH_END]->value.val.n = found ? foundEnd : 0;
    result->tag = INT_TAG;
    result->val.n = found ? foundStart : -1;
    return True;
}

/*
** Built-in macro subroutine for replacing all occurences of a search string in
** a string with a replacement string.  Arguments are $1: string to search in,
** $2: string to search for, $3: replacement string. Argument $4 is an optional
** search type: one of "literal", "case" or "regex" (default is "literal").
**
** Returns a new string with all of the replacements done, or an empty string
** ("") if no occurences were found.
*/
static int replaceInStringMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char stringStorage[3][25], *string, *searchStr, *replaceStr;
    char *argStr, *replacedStr;
    int searchType = SEARCH_LITERAL, copyStart, copyEnd;
    int replacedLen, replaceEnd;
    
    /* Validate arguments and convert to proper types */
    if (nArgs < 3 || nArgs > 5)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &string, stringStorage[0], errMsg))
    	return False;
    if (!readStringArg(argList[1], &searchStr, stringStorage[1], errMsg))
    	return False;
    if (!readStringArg(argList[2], &replaceStr, stringStorage[2], errMsg))
    	return False;
    if (nArgs == 4) {
	if (!readStringArg(argList[3], &argStr, stringStorage[2], errMsg))
    	    return False;
    	if (!strcmp(argStr, "literal"))
    	    searchType = SEARCH_LITERAL;
    	else if (!strcmp(argStr, "case"))
    	    searchType = SEARCH_CASE_SENSE;
    	else if (!strcmp(argStr, "regex"))
    	    searchType = SEARCH_REGEX;
    	else {
    	    *errMsg = "unrecognized argument to %s";
    	    return False;
    	}
    }
    
    /* Do the replace */
    replacedStr = ReplaceAllInString(string, searchStr, replaceStr, searchType,
	    &copyStart, &copyEnd, &replacedLen, GetWindowDelimiters(window));
    
    /* Return the results */
    result->tag = STRING_TAG;
    if (replacedStr == NULL) {
    	result->val.str = AllocString(1);
	result->val.str[0] = '\0';
    } else {
	replaceEnd = copyStart + replacedLen;
	result->val.str = AllocString(replaceEnd + strlen(&string[copyEnd])+1);
	strncpy(result->val.str, string, copyStart);
	strcpy(&result->val.str[copyStart], replacedStr);
	strcpy(&result->val.str[replaceEnd], &string[copyEnd]);
	XtFree(replacedStr);
    }
    return True;
}

static int readSearchArgs(DataValue *argList, int nArgs, int *searchDirection,
	int *searchType, int *wrap, char **errMsg)
{
    int i;
    char *argStr, stringStorage[9][25];
    
    *wrap = False;
    *searchDirection = SEARCH_FORWARD;
    *searchType = SEARCH_LITERAL;
    for (i=0; i<nArgs; i++) {
    	if (!readStringArg(argList[i], &argStr, stringStorage[i], errMsg))
    	    return False;
    	else if (!strcmp(argStr, "wrap"))
    	    *wrap = True;
    	else if (!strcmp(argStr, "backward"))
    	    *searchDirection = SEARCH_BACKWARD;
    	else if (!strcmp(argStr, "forward"))
    	    *searchDirection = SEARCH_FORWARD;
    	else if (!strcmp(argStr, "literal"))
    	    *searchType = SEARCH_LITERAL;
    	else if (!strcmp(argStr, "case"))
    	    *searchType = SEARCH_CASE_SENSE;
    	else if (!strcmp(argStr, "regex"))
    	    *searchType = SEARCH_REGEX;
    	else {
    	    *errMsg = "unrecognized argument to %s";
    	    return False;
    	}
    }
    return True;
}

static int setCursorPosMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int pos;

    /* Get argument and convert to int */
    if (nArgs != 1)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &pos, errMsg))
    	return False;
    
    /* Set the position */
    TextSetCursorPos(window->lastFocus, pos);
    result->tag = NO_TAG;
    return True;
}

static int selectMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int start, end;

    /* Get arguments and convert to int */
    if (nArgs != 2)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &start, errMsg))
    	return False;
    if (!readIntArg(argList[1], &end, errMsg))
    	return False;
    
    /* Make the selection */
    BufSelect(window->buffer, start, end);
    result->tag = NO_TAG;
    return True;
}

static int selectRectangleMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int start, end, left, right;

    /* Get arguments and convert to int */
    if (nArgs != 4)
    	return wrongNArgsErr(errMsg);
    if (!readIntArg(argList[0], &start, errMsg))
    	return False;
    if (!readIntArg(argList[1], &end, errMsg))
    	return False;
    if (!readIntArg(argList[2], &left, errMsg))
    	return False;
    if (!readIntArg(argList[3], &right, errMsg))
    	return False;
    
    /* Make the selection */
    BufRectSelect(window->buffer, start, end, left, right);
    result->tag = NO_TAG;
    return True;
}

/*
** Macro subroutine to ring the bell
*/
static int beepMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    if (nArgs != 0)
    	return wrongNArgsErr(errMsg);
    XBell(XtDisplay(window->shell), 0);
    result->tag = NO_TAG;
    return True;
}

static int tPrintMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char stringStorage[25], *string;
    int i;
    
    if (nArgs == 0)
    	return tooFewArgsErr(errMsg);
    for (i=0; i<nArgs; i++) {
	if (!readStringArg(argList[i], &string, stringStorage, errMsg))
	    return False;
	printf("%s%s", string, i==nArgs-1 ? "" : " ");
    }
    result->tag = NO_TAG;
    return True;
}

static int shellCmdMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char stringStorage[2][25], *cmdString, *inputString;

    if (nArgs != 2)
    	return wrongNArgsErr(errMsg);
    if (!readStringArg(argList[0], &cmdString, stringStorage[0], errMsg))
    	return False;
    if (!readStringArg(argList[1], &inputString, stringStorage[1], errMsg))
    	return False;
	
#ifdef VMS
    *errMsg = "Shell commands not supported under VMS";
    return False;
#else
    ShellCmdToMacroString(window, cmdString, inputString);
    result->tag = INT_TAG;
    result->val.n = 0;
    return True;
#endif /*VMS*/
}

/*
** Method used by ShellCmdToMacroString (called by shellCmdMS), for returning
** macro string and exit status after the execution of a shell command is
** complete.  (Sorry about the poor modularity here, it's just not worth
** teaching other modules about macro return globals, since other than this,
** they're not used outside of macro.c)
*/
void ReturnShellCommandOutput(WindowInfo *window, char *outText, int status)
{
    DataValue retVal;
    macroCmdInfo *cmdData = window->macroCmdData;
    
    if (cmdData == NULL)
    	return;
    retVal.tag = STRING_TAG;
    retVal.val.str = AllocString(strlen(outText)+1);
    strcpy(retVal.val.str, outText);
    ModifyReturnedValue(cmdData->context, retVal);
    ReturnGlobals[SHELL_CMD_STATUS]->value.tag = INT_TAG;
    ReturnGlobals[SHELL_CMD_STATUS]->value.val.n = status;
}

static int dialogMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    macroCmdInfo *cmdData;
    char stringStorage[9][25], *btnLabels[8], *message;
    Widget shell, dialog, btn;
    int i, nBtns;
    XmString s1, s2;
    
    /* Ignore the focused window passed as the function argument and put
       the dialog up over the window which is executing the macro */
    window = MacroRunWindow();
    cmdData = window->macroCmdData;
    
    /* Read and check the arguments.  The first being the dialog message,
       and the rest being the button labels */
    if (nArgs == 0) {
    	*errMsg = "%s subroutine called with no arguments";
    	return False;
    }
    if (!readStringArg(argList[0], &message, stringStorage[0], errMsg))
	return False;
    for (i=1; i<nArgs; i++)
	if (!readStringArg(argList[i], &btnLabels[i-1], stringStorage[i],
	    	errMsg))
	    return False;
    if (nArgs == 1) {
    	btnLabels[0] = "Dismiss";
    	nBtns = 1;
    } else
    	nBtns = nArgs - 1;

    /* Create the message box dialog widget and its dialog shell parent */
    shell = XtVaCreateWidget("macroDialogShell", xmDialogShellWidgetClass,
    	    window->shell, XmNtitle, "", 0);
    AddMotifCloseCallback(shell, dialogCloseCB, window);
    dialog = XtVaCreateWidget("macroDialog", xmMessageBoxWidgetClass,
    	    shell, XmNmessageString, s1=MKSTRING(message),
    	    XmNokLabelString, s2=XmStringCreateSimple(btnLabels[0]), 0);
    XtAddCallback(dialog, XmNokCallback, dialogBtnCB, window);
    XtVaSetValues(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
    	    XmNuserData, (XtPointer)1, 0);
    XmStringFree(s1);
    XmStringFree(s2);
    cmdData->dialog = dialog;

    /* Unmanage default buttons, except for "OK" */
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

    /* Add user specified buttons (1st is already done) */
    for (i=1; i<nBtns; i++) {
    	btn = XtVaCreateManagedWidget("mdBtn", xmPushButtonWidgetClass, dialog,
    	    	XmNlabelString, s1=XmStringCreateSimple(btnLabels[i]),
    	    	XmNuserData, (XtPointer)(i+1), 0);
    	XtAddCallback(btn, XmNactivateCallback, dialogBtnCB, window);
    	XmStringFree(s1);
    }
    
    /* Put up the dialog */
    ManageDialogCenteredOnPointer(dialog);
    
    /* Stop macro execution until the dialog is complete */
    PreemptMacro();
    
    /* Return placeholder result.  Value will be changed by button callback */
    result->tag = INT_TAG;
    result->val.n = 0;
    return True;
}

static void dialogBtnCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    XtPointer userData;
    DataValue retVal;
    
    /* Return the index of the button which was pressed (stored in the userData
       field of the button widget).  The 1st button, being a gadget, is not
       returned in w. */
    if (cmdData == NULL)
    	return; /* shouldn't happen */
    if (XtClass(w) == xmPushButtonWidgetClass) {
	XtVaGetValues(w, XmNuserData, &userData, 0);
	retVal.val.n = (int)userData;
    } else
    	retVal.val.n = 1;
    retVal.tag = INT_TAG;
    ModifyReturnedValue(cmdData->context, retVal);

    /* Pop down the dialog */
    XtDestroyWidget(XtParent(cmdData->dialog));
    cmdData->dialog = NULL;

    /* Continue preempted macro execution */
    ResumeMacroExecution(window);
}

static void dialogCloseCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    DataValue retVal;
    
    /* Return 0 to show that the dialog was closed via the window close box */
    retVal.val.n = 0;
    retVal.tag = INT_TAG;
    ModifyReturnedValue(cmdData->context, retVal);

    /* Pop down the dialog */
    XtDestroyWidget(XtParent(cmdData->dialog));
    cmdData->dialog = NULL;

    /* Continue preempted macro execution */
    ResumeMacroExecution(window);
}

static int stringDialogMS(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    macroCmdInfo *cmdData;
    char stringStorage[9][25], *btnLabels[8], *message;
    Widget shell, dialog, btn;
    int i, nBtns;
    XmString s1, s2;
    
    /* Ignore the focused window passed as the function argument and put
       the dialog up over the window which is executing the macro */
    window = MacroRunWindow();
    cmdData = window->macroCmdData;

    /* Read and check the arguments.  The first being the dialog message,
       and the rest being the button labels */
    if (nArgs == 0) {
    	*errMsg = "%s subroutine called with no arguments";
    	return False;
    }
    if (!readStringArg(argList[0], &message, stringStorage[0], errMsg))
	return False;
    for (i=1; i<nArgs; i++)
	if (!readStringArg(argList[i], &btnLabels[i-1], stringStorage[i],
	    	errMsg))
	    return False;
    if (nArgs == 1) {
    	btnLabels[0] = "Dismiss";
    	nBtns = 1;
    } else
    	nBtns = nArgs - 1;

    /* Create the selection box dialog widget and its dialog shell parent */
    shell = XtVaCreateWidget("macroDialogShell", xmDialogShellWidgetClass,
    	    window->shell, XmNtitle, "", 0);
    AddMotifCloseCallback(shell, stringDialogCloseCB, window);
    dialog = XtVaCreateWidget("macroStringDialog", xmSelectionBoxWidgetClass,
    	    shell, XmNselectionLabelString, s1=MKSTRING(message),
    	    XmNokLabelString, s2=XmStringCreateSimple(btnLabels[0]),
    	    XmNdialogType, XmDIALOG_PROMPT, 0);
    XtAddCallback(dialog, XmNokCallback, stringDialogBtnCB, window);
    XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
    	    XmNuserData, (XtPointer)1, 0);
    XmStringFree(s1);
    XmStringFree(s2);
    cmdData->dialog = dialog;

    /* Unmanage unneded widgets */
    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

    /* Add user specified buttons (1st is already done).  Selection box
       requires a place-holder widget to be added before buttons can be
       added, that's what the separator below is for */
    XtVaCreateWidget("x", xmSeparatorWidgetClass, dialog, 0);
    for (i=1; i<nBtns; i++) {
    	btn = XtVaCreateManagedWidget("mdBtn", xmPushButtonWidgetClass, dialog,
    	    	XmNlabelString, s1=XmStringCreateSimple(btnLabels[i]),
    	    	XmNuserData, (XtPointer)(i+1), 0);
    	XtAddCallback(btn, XmNactivateCallback, stringDialogBtnCB, window);
    	XmStringFree(s1);
    }
    
    /* Put up the dialog */
    ManageDialogCenteredOnPointer(dialog);
    
    /* Stop macro execution until the dialog is complete */
    PreemptMacro();
    
    /* Return placeholder result.  Value will be changed by button callback */
    result->tag = INT_TAG;
    result->val.n = 0;
    return True;
}

static void stringDialogBtnCB(Widget w, XtPointer clientData,
    	XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    XtPointer userData;
    DataValue retVal;
    char *text;
    int btnNum;

    /* shouldn't happen, but would crash if it did */
    if (cmdData == NULL)
    	return; 

    /* Return the string entered in the selection text area */
    text = XmTextGetString(XmSelectionBoxGetChild(cmdData->dialog,
    	    XmDIALOG_TEXT));
    retVal.tag = STRING_TAG;
    retVal.val.str = AllocString(strlen(text)+1);
    strcpy(retVal.val.str, text);
    XtFree(text);
    ModifyReturnedValue(cmdData->context, retVal);
    
    /* Find the index of the button which was pressed (stored in the userData
       field of the button widget).  The 1st button, being a gadget, is not
       returned in w. */
    if (XtClass(w) == xmPushButtonWidgetClass) {
	XtVaGetValues(w, XmNuserData, &userData, 0);
	btnNum = (int)userData;
    } else
    	btnNum = 1;
    
    /* Return the button number in the global variable $string_dialog_button */
    ReturnGlobals[STRING_DIALOG_BUTTON]->value.tag = INT_TAG;
    ReturnGlobals[STRING_DIALOG_BUTTON]->value.val.n = btnNum;

    /* Pop down the dialog */
    XtDestroyWidget(XtParent(cmdData->dialog));
    cmdData->dialog = NULL;

    /* Continue preempted macro execution */
    ResumeMacroExecution(window);
}

static void stringDialogCloseCB(Widget w, XtPointer clientData,
    	XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    macroCmdInfo *cmdData = window->macroCmdData;
    DataValue retVal;

    /* shouldn't happen, but would crash if it did */
    if (cmdData == NULL)
    	return; 

    /* Return an empty string */
    retVal.tag = STRING_TAG;
    retVal.val.str = AllocString(1);
    retVal.val.str[0] = '\0';
    ModifyReturnedValue(cmdData->context, retVal);
    
    /* Return button number 0 in the global variable $string_dialog_button */
    ReturnGlobals[STRING_DIALOG_BUTTON]->value.tag = INT_TAG;
    ReturnGlobals[STRING_DIALOG_BUTTON]->value.val.n = 0;

    /* Pop down the dialog */
    XtDestroyWidget(XtParent(cmdData->dialog));
    cmdData->dialog = NULL;

    /* Continue preempted macro execution */
    ResumeMacroExecution(window);
}

static int cursorMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = TextGetCursorPos(window->lastFocus);
    return True;
}

static int fileNameMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = STRING_TAG;
    result->val.str = AllocString(strlen(window->filename) + 1);
    strcpy(result->val.str, window->filename);
    return True;
}

static int filePathMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = STRING_TAG;
    result->val.str = AllocString(strlen(window->path) + 1);
    strcpy(result->val.str, window->path);
    return True;
}

static int lengthMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = window->buffer->length;
    return True;
}

static int selectionStartMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = window->buffer->primary.selected ?
    	    window->buffer->primary.start : -1;
    return True;
}

static int selectionEndMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = window->buffer->primary.selected ?
    	    window->buffer->primary.end : -1;
    return True;
}

static int selectionLeftMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    selection *sel = &window->buffer->primary;
    
    result->tag = INT_TAG;
    result->val.n = sel->selected && sel->rectangular ? sel->rectStart : -1;
    return True;
}

static int selectionRightMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    selection *sel = &window->buffer->primary;
    
    result->tag = INT_TAG;
    result->val.n = sel->selected && sel->rectangular ? sel->rectEnd : -1;
    return True;
}

static int wrapMarginMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    int margin, nCols;
    
    XtVaGetValues(window->textArea, textNcolumns, &nCols,
    	    textNwrapMargin, &margin, 0);
    result->tag = INT_TAG;
    result->val.n = margin == 0 ? nCols : margin;
    return True;
}

static int tabDistMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = window->buffer->tabDist;
    return True;
}

static int useTabsMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    result->tag = INT_TAG;
    result->val.n = window->buffer->useTabs;
    return True;
}

static int languageModeMV(WindowInfo *window, DataValue *argList, int nArgs,
    	DataValue *result, char **errMsg)
{
    char *lmName = LanguageModeName(window->languageMode);
    
    if (lmName == NULL)
    	lmName = "Plain";
    result->tag = STRING_TAG;
    result->val.str = AllocString(strlen(lmName) + 1);
    strcpy(result->val.str, lmName);
    return True;
}

static int wrongNArgsErr(char **errMsg)
{
    *errMsg = "wrong number of arguments to function %s";
    return False;
}

static int tooFewArgsErr(char **errMsg)
{
    *errMsg = "too few arguments to function %s";
    return False;
}


/*
** Get an integer value from a tagged DataValue structure.  Return True
** if conversion succeeded, and store result in *result, otherwise
** return False with an error message in *errMsg.
*/
static int readIntArg(DataValue dv, int *result, char **errMsg)
{
    char *c;
    
    if (dv.tag == INT_TAG) {
    	*result = dv.val.n;
    	return True;
    } else if (dv.tag == STRING_TAG) {
	for (c=dv.val.str; *c != '\0'; c++) {
    	    if (!(isdigit(*c) || *c != ' ' || *c != '\t')) {
    		goto typeError;
    	    }
    	}
	sscanf(dv.val.str, "%d", result);
	return True;
    }
    
typeError:
    *errMsg = "%s called with non-integer argument";
    return False;
}

/*
** Get an string value from a tagged DataValue structure.  Return True
** if conversion succeeded, and store result in *result, otherwise
** return False with an error message in *errMsg.  If an integer value
** is converted, write the string in the space provided by "stringStorage",
** which must be large enough to handle ints of the maximum size.
*/
static int readStringArg(DataValue dv, char **result, char *stringStorage,
    	char **errMsg)
{
    if (dv.tag == STRING_TAG) {
    	*result = dv.val.str;
    	return True;
    } else if (dv.tag == INT_TAG) {
	sprintf(stringStorage, "%d", dv.val.n);
	*result = stringStorage;
	return True;
    }
    *errMsg = "%s called with unknown object";
    return False;
}

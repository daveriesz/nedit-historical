/*******************************************************************************
*									       *
* shell.c -- Nirvana Editor shell command execution			       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version.							               *
* 									       *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
* December, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef notdef
#ifdef IBM
#define NBBY 8
#include <sys/select.h>
#endif
#include <time.h>
#endif
#ifdef __EMX__
#include <process.h>
#endif
#include <errno.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "file.h"
#include "shell.h"
#include "macro.h"
#include "interpret.h"

/* Tuning parameters */
#define IO_BUF_SIZE 4096	/* size of buffers for collecting cmd output */
#define MAX_SHELL_CMD_LEN 1024	/* max length of a shell command (should be
				   eliminated, but substitutePercent needs) */
#define MAX_OUT_DIALOG_ROWS 30	/* max height of dialog for command output */
#define MAX_OUT_DIALOG_COLS 80	/* max width of dialog for command output */
#define OUTPUT_FLUSH_FREQ 1000	/* how often (msec) to flush output buffers
    	    	    	    	   when process is taking too long */
#define BANNER_WAIT_TIME 6000	/* how long to wait (msec) before putting up
    	    	    	    	   Shell Command Executing... banner */

/* flags for issueCommand */
#define ACCUMULATE 1
#define ERROR_DIALOGS 2
#define REPLACE_SELECTION 4
#define RELOAD_FILE_AFTER 8
#define OUTPUT_TO_DIALOG 16
#define OUTPUT_TO_STRING 32

/* element of a buffer list for collecting output from shell processes */
typedef struct bufElem {
    struct bufElem *next;
    int length;
    char contents[IO_BUF_SIZE];
} buffer;

/* data attached to window during shell command execution with
   information for controling and communicating with the process */
typedef struct {
    int flags;
    int stdinFD, stdoutFD, stderrFD;
    pid_t childPid;
    XtInputId stdinInputID, stdoutInputID, stderrInputID;
    buffer *outBufs, *errBufs;
    char *input;
    char *inPtr;
    Widget textW;
    int leftPos, rightPos;
    int inLength;
    XtIntervalId bannerTimeoutID, flushTimeoutID;
    char bannerIsUp;
    char fromMacro;
} shellCmdInfo;

static void issueCommand(WindowInfo *window, char *command, char *input,
	int inputLen, int flags, Widget textW, int replaceLeft,
	int replaceRight, int fromMacro);
static void stdoutReadProc(XtPointer clientData, int *source, XtInputId *id);
static void stderrReadProc(XtPointer clientData, int *source, XtInputId *id);
static void stdinWriteProc(XtPointer clientData, int *source, XtInputId *id);
static void finishCmdExecution(WindowInfo *window, int terminatedOnError);
static pid_t forkCommand(Widget parent, char *command, char *cmdDir,
	int *stdinFD, int *stdoutFD, int *stderrFD);
static void addOutput(buffer **bufList, buffer *buf);
static char *coalesceOutput(buffer **bufList, int *length);
static void freeBufList(buffer **bufList);
static void removeTrailingNewlines(char *string);
static void createOutputDialog(Widget parent, char *text);
static void destroyOutDialogCB(Widget w, XtPointer callback, XtPointer closure);
static void measureText(char *text, int wrapWidth, int *rows, int *cols);
static void truncateString(char *string, int length);
static int substitutePercent(char *outStr, char *inStr, char *subsStr,
	int outLen);
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void flushTimeoutProc(XtPointer clientData, XtIntervalId *id);

/*
** Filter the current selection through shell command "command".  The selection
** is removed, and replaced by the output from the command execution.  Failed
** command status and output to stderr are presented in dialog form.
*/
void FilterSelection(WindowInfo *window, char *command, int fromMacro)
{
    int left, right, textLen;
    char *text;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* Get the selection and the range in character positions that it
       occupies.  Beep and return if no selection */
    text = BufGetSelectionText(window->buffer);
    if (*text == '\0') {
	XtFree(text);
	XBell(TheDisplay, 0);
	return;
    }
    textLen = strlen(text);
    BufUnsubstituteNullChars(text, window->buffer);
    left = window->buffer->primary.start;
    right = window->buffer->primary.end;
    
    /* Issue the command and collect its output */
    issueCommand(window, command, text, textLen, ACCUMULATE | ERROR_DIALOGS |
	    REPLACE_SELECTION, window->lastFocus, left, right, fromMacro);
}

/*
** Execute shell command "command", depositing the result at the current
** insert position or in the current selection if if the window has a
** selection.
*/
void ExecShellCommand(WindowInfo *window, char *command, int fromMacro)
{
    int left, right, flags = 0;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }
    
    /* get the selection or the insert position */
    if (GetSimpleSelection(window->buffer, &left, &right))
    	flags = ACCUMULATE | REPLACE_SELECTION;
    else
    	left = right = TextGetCursorPos(window->lastFocus);
    
    /* issue the command */
    issueCommand(window, command, NULL, 0, flags, window->lastFocus, left,
	    right, fromMacro);
}

/*
** Execute shell command "command", on input string "input", depositing the
** in a macro string (via a call back to ReturnShellCommandOutput).
*/
void ShellCmdToMacroString(WindowInfo *window, char *command, char *input)
{
    char *inputCopy;
    
    /* Make a copy of the input string for issueCommand to hold and free
       upon completion */
    inputCopy = *input == '\0' ? NULL : XtNewString(input);
    
    /* fork the command and begin processing input/output */
    issueCommand(window, command, inputCopy, strlen(input),
	    ACCUMULATE | OUTPUT_TO_STRING, NULL, 0, 0, True);
}

/*
** Execute the line of text where the the insertion cursor is positioned
** as a shell command.
*/
void ExecCursorLine(WindowInfo *window, int fromMacro)
{
    char *cmdText;
    int left, right, insertPos;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* get all of the text on the line with the insert position */
    if (!GetSimpleSelection(window->buffer, &left, &right)) {
	left = right = TextGetCursorPos(window->lastFocus);
	left = BufStartOfLine(window->buffer, left);
	right = BufEndOfLine(window->buffer, right);
	insertPos = right;
    } else
    	insertPos = BufEndOfLine(window->buffer, right);
    cmdText = BufGetRange(window->buffer, left, right);
    BufUnsubstituteNullChars(cmdText, window->buffer);
    
    /* insert a newline after the entire line */
    BufInsert(window->buffer, insertPos, "\n");

    /* issue the command */
    issueCommand(window, cmdText, NULL, 0, 0, window->lastFocus, insertPos+1,
	    insertPos+1, fromMacro);
    XtFree(cmdText);
}

/*
** Do a shell command, with the options allowed to users (input source,
** output destination, save first and load after) in the shell commands
** menu.
*/
void DoShellMenuCmd(WindowInfo *window, char *command, int input, int output,
	int outputReplacesInput, int saveFirst, int loadAfter, int fromMacro) 
{
    int flags = 0;
    char *text;
    char subsCommand[MAX_SHELL_CMD_LEN], fullName[MAXPATHLEN];
    int left, right, textLen;
    WindowInfo *inWindow = window;
    Widget outWidget;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* Substitute the current file name for % in the shell command */
    strcpy(fullName, window->path);
    strcat(fullName, window->filename);
    if (!substitutePercent(subsCommand, command, fullName,
    	    MAX_SHELL_CMD_LEN)) {
    	DialogF(DF_ERR, window->shell, 1,
	   "Shell command is too long due to\nfilename substitutions with '%%'",
	    "OK");
	return;
    }
    	
    /* Get the command input as a text string.  If there is input, errors
      shouldn't be mixed in with output, so set flags to ERROR_DIALOGS */
    if (input == FROM_SELECTION) {
	text = BufGetSelectionText(window->buffer);
	if (*text == '\0') {
    	    XtFree(text);
    	    XBell(TheDisplay, 0);
    	    return;
    	}
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else if (input == FROM_WINDOW) {
	text = BufGetAll(window->buffer);
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else if (input == FROM_EITHER) {
	text = BufGetSelectionText(window->buffer);
	if (*text == '\0') {
	    XtFree(text);
	    text = BufGetAll(window->buffer);
    	}
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else /* FROM_NONE */
    	text = NULL;
    
    /* If the buffer was substituting another character for ascii-nuls,
       put the nuls back in before exporting the text */
    if (text != NULL) {
	textLen = strlen(text);
	BufUnsubstituteNullChars(text, window->buffer);
    } else
	textLen = 0;
    
    /* Assign the output destination.  If output is to a new window,
       create it, and run the command from it instead of the current
       one, to free the current one from waiting for lengthy execution */
    if (output == TO_DIALOG) {
    	outWidget = NULL;
	flags |= OUTPUT_TO_DIALOG;
    	left = right = 0;
    } else if (output == TO_NEW_WINDOW) {
    	EditNewFile(NULL, False, NULL);
    	outWidget = WindowList->textArea;
	inWindow = WindowList;
    	left = right = 0;
    } else { /* TO_SAME_WINDOW */
    	outWidget = window->lastFocus;
    	if (outputReplacesInput && input != FROM_NONE) {
    	    if (input == FROM_WINDOW) {
    		left = 0;
    		right = window->buffer->length;
    	    } else if (input == FROM_SELECTION) {
    	    	GetSimpleSelection(window->buffer, &left, &right);
	        flags |= ACCUMULATE | REPLACE_SELECTION;
    	    } else if (input == FROM_EITHER) {
    	    	if (GetSimpleSelection(window->buffer, &left, &right))
	            flags |= ACCUMULATE | REPLACE_SELECTION;
	        else {
	            left = 0;
	            right = window->buffer->length;
	        }
	    }
    	} else {
	    if (GetSimpleSelection(window->buffer, &left, &right))
	        flags |= ACCUMULATE | REPLACE_SELECTION;
	    else
    		left = right = TextGetCursorPos(window->lastFocus);
    	}
    }
    
    /* If the command requires the file be saved first, save it */
    if (saveFirst) {
    	if (!SaveWindow(window)) {
    	    if (input != FROM_NONE)
    		XtFree(text);
    	    return;
	}
    }
    
    /* If the command requires the file to be reloaded after execution, set
       a flag for issueCommand to deal with it when execution is complete */
    if (loadAfter)
    	flags |= RELOAD_FILE_AFTER;
    	
    /* issue the command */
    issueCommand(inWindow, subsCommand, text, textLen, flags, outWidget, left,
	    right, fromMacro);
}

/*
** Cancel the shell command in progress
*/
void AbortShellCommand(WindowInfo *window)
{
    shellCmdInfo *cmdData = window->shellCmdData;

    if (cmdData == NULL)
    	return;
    kill(- cmdData->childPid, SIGTERM);
    finishCmdExecution(window, True);
}

/*
** Issue a shell command and feed it the string "input".  Output can be
** directed either to text widget "textW" where it replaces the text between
** the positions "replaceLeft" and "replaceRight", to a separate pop-up dialog
** (OUTPUT_TO_DIALOG), or to a macro-language string (OUTPUT_TO_STRING).  If
** "input" is NULL, no input is fed to the process.  If an input string is
** provided, it is freed when the command completes.  Flags:
**
**   ACCUMULATE     	Causes output from the command to be saved up until
**  	    	    	the command completes.
**   ERROR_DIALOGS  	Presents stderr output separately in popup a dialog,
**  	    	    	and also reports failed exit status as a popup dialog
**  	    	    	including the command output.
**   REPLACE_SELECTION  Causes output to replace the selection in textW.
**   RELOAD_FILE_AFTER  Causes the file to be completely reloaded after the
**  	    	    	command completes.
**   OUTPUT_TO_DIALOG   Send output to a pop-up dialog instead of textW
**   OUTPUT_TO_STRING   Output to a macro-language string instead of a text
**  	    	    	widget or dialog.
**
** REPLACE_SELECTION, ERROR_DIALOGS, and OUTPUT_TO_STRING can only be used
** along with ACCUMULATE (these operations can't be done incrementally).
*/
static void issueCommand(WindowInfo *window, char *command, char *input,
	int inputLen, int flags, Widget textW, int replaceLeft,
	int replaceRight, int fromMacro)
{
    int stdinFD, stdoutFD, stderrFD;
    XtAppContext context = XtWidgetToApplicationContext(window->shell);
    shellCmdInfo *cmdData;
    pid_t childPid;
    
    /* verify consistency of input parameters */
    if ((flags & ERROR_DIALOGS || flags & REPLACE_SELECTION ||
	    flags & OUTPUT_TO_STRING) && !(flags & ACCUMULATE))
    	return;
    
    /* a shell command called from a macro must be executed in the same
       window as the macro, regardless of where the output is directed,
       so the user can cancel them as a unit */
    if (fromMacro)
    	window = MacroRunWindow();
    
    /* put up a watch cursor over the waiting window */
    if (!fromMacro)
    	BeginWait(window->shell);
    
    /* enable the cancel menu item */
    if (!fromMacro)
    	XtSetSensitive(window->cancelShellItem, True);

    /* fork the subprocess and issue the command */
    childPid = forkCommand(window->shell, command, window->path, &stdinFD,
	    &stdoutFD, (flags & ERROR_DIALOGS) ? &stderrFD : NULL);
    
    /* set the pipes connected to the process for non-blocking i/o */
    if (fcntl(stdinFD, F_SETFL, O_NONBLOCK) < 0)
    	perror("NEdit: Internal error (fcntl)");
    if (fcntl(stdoutFD, F_SETFL, O_NONBLOCK) < 0)
    	perror("NEdit: Internal error (fcntl1)");
    if (flags & ERROR_DIALOGS) {
	if (fcntl(stderrFD, F_SETFL, O_NONBLOCK) < 0)
    	    perror("NEdit: Internal error (fcntl2)");
    }
    
    /* if there's nothing to write to the process' stdin, close it now */
    if (input == NULL)
    	close(stdinFD);
    
    /* Create a data structure for passing process information around
       amongst the callback routines which will process i/o and completion */
    cmdData = (shellCmdInfo *)XtMalloc(sizeof(shellCmdInfo));
    window->shellCmdData = cmdData;
    cmdData->flags = flags;
    cmdData->stdinFD = stdinFD;
    cmdData->stdoutFD = stdoutFD;
    cmdData->stderrFD = stderrFD;
    cmdData->childPid = childPid;
    cmdData->outBufs = NULL;
    cmdData->errBufs = NULL;
    cmdData->input = input;
    cmdData->inPtr = input;
    cmdData->textW = textW;
    cmdData->bannerIsUp = False;
    cmdData->fromMacro = fromMacro;
    cmdData->leftPos = replaceLeft;
    cmdData->rightPos = replaceRight;
    cmdData->inLength = inputLen;
    
    /* Set up timer proc for putting up banner when process takes too long */
    if (fromMacro)
    	cmdData->bannerTimeoutID = 0;
    else
    	cmdData->bannerTimeoutID = XtAppAddTimeOut(context, BANNER_WAIT_TIME,
    	    	bannerTimeoutProc, window);

    /* Set up timer proc for flushing output buffers periodically */
    if ((flags & ACCUMULATE) || textW == NULL)
    	cmdData->flushTimeoutID = 0;
    else
	cmdData->flushTimeoutID = XtAppAddTimeOut(context, OUTPUT_FLUSH_FREQ,
	    	flushTimeoutProc, window);
    	
    /* set up callbacks for activity on the file descriptors */
    cmdData->stdoutInputID = XtAppAddInput(context, stdoutFD,
    	    (XtPointer)XtInputReadMask, stdoutReadProc, window);
    if (input != NULL)
    	cmdData->stdinInputID = XtAppAddInput(context, stdinFD,
    	    	(XtPointer)XtInputWriteMask, stdinWriteProc, window);
    else
    	cmdData->stdinInputID = 0;
    if (flags & ERROR_DIALOGS)
	cmdData->stderrInputID = XtAppAddInput(context, stderrFD,
    		(XtPointer)XtInputReadMask, stderrReadProc, window);
    else
    	cmdData->stderrInputID = 0;
    
    /* If this was called from a macro, preempt the macro untill shell
       command completes */
    if (fromMacro)
    	PreemptMacro();
}

/*
** Called when the shell sub-process stdout stream has data.  Reads data into
** the "outBufs" buffer chain in the window->shellCommandData data structure.
*/
static void stdoutReadProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    buffer *buf;
    int nRead;

    /* read from the process' stdout stream */
    buf = (buffer *)XtMalloc(sizeof(buffer));
    nRead = read(cmdData->stdoutFD, buf->contents, IO_BUF_SIZE);
    
    /* error in read */
    if (nRead == -1) { /* error */
	if (errno != EWOULDBLOCK && errno != EAGAIN) {
	    perror("NEdit: Error reading shell command output");
	    XtFree((char *)buf);
	    finishCmdExecution(window, True);
	}
	return;
    }
    
    /* end of data.  If the stderr stream is done too, execution of the
       shell process is complete, and we can display the results */
    if (nRead == 0) {
    	XtFree((char *)buf);
    	XtRemoveInput(cmdData->stdoutInputID);
    	cmdData->stdoutInputID = 0;
    	if (cmdData->stderrInputID == 0)
    	    finishCmdExecution(window, False);
    	return;
    }
    
    /* characters were read successfully, add buf to linked list of buffers */
    buf->length = nRead;
    addOutput(&cmdData->outBufs, buf);
}

/*
** Called when the shell sub-process stderr stream has data.  Reads data into
** the "errBufs" buffer chain in the window->shellCommandData data structure.
*/
static void stderrReadProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    buffer *buf;
    int nRead;
    
    /* read from the process' stderr stream */
    buf = (buffer *)XtMalloc(sizeof(buffer));
    nRead = read(cmdData->stderrFD, buf->contents, IO_BUF_SIZE);
    
    /* error in read */
    if (nRead == -1) {
	if (errno != EWOULDBLOCK && errno != EAGAIN) {
	    perror("NEdit: Error reading shell command error stream");
	    XtFree((char *)buf);
	    finishCmdExecution(window, True);
	}
	return;
    }
    
    /* end of data.  If the stdout stream is done too, execution of the
       shell process is complete, and we can display the results */
    if (nRead == 0) {
    	XtFree((char *)buf);
    	XtRemoveInput(cmdData->stderrInputID);
    	cmdData->stderrInputID = 0;
    	if (cmdData->stdoutInputID == 0)
    	    finishCmdExecution(window, False);
    	return;
    }
    
    /* characters were read successfully, add buf to linked list of buffers */
    buf->length = nRead;
    addOutput(&cmdData->errBufs, buf);
}

/*
** Called when the shell sub-process stdin stream is ready for input.  Writes
** data from the "input" text string passed to issueCommand.
*/
static void stdinWriteProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    int nWritten;

    nWritten = write(cmdData->stdinFD, cmdData->inPtr, cmdData->inLength);
    if (nWritten == -1) {
	if (errno == EPIPE) {
	    /* Just shut off input to broken pipes.  User is likely feeding
	       it to a command which does not take input */
	    XtRemoveInput(cmdData->stdinInputID);
	    cmdData->stdinInputID = 0;
    	    close(cmdData->stdinFD);
    	    cmdData->inPtr = NULL;
    	} else if (errno != EWOULDBLOCK && errno != EAGAIN) {
    	    perror("NEdit: Write to shell command failed");
    	    finishCmdExecution(window, True);
    	}
    } else {
	cmdData->inPtr += nWritten;
	cmdData->inLength -= nWritten;
	if (cmdData->inLength <= 0) {
	    XtRemoveInput(cmdData->stdinInputID);
	    cmdData->stdinInputID = 0;
    	    close(cmdData->stdinFD);
    	    cmdData->inPtr = NULL;
    	}
    }
}

/*
** Timer proc for putting up the "Shell Command in Progress" banner if
** the process is taking too long.
*/
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    
    cmdData->bannerIsUp = True;
    SetModeMessage(window,
    	    "Shell Command in Progress -- Press Ctrl+. to Cancel");
    cmdData->bannerTimeoutID = 0;
}

/*
** Timer proc for flushing output buffers periodically when the process
** takes too long.
*/
static void flushTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    textBuffer *buf = TextGetBuffer(cmdData->textW);
    int len;
    char *outText;
    
    /* shouldn't happen, but it would be bad if it did */
    if (cmdData->textW == NULL)
    	return;

    outText = coalesceOutput(&cmdData->outBufs, &len);
    if (len != 0) {
	if (BufSubstituteNullChars(outText, len, buf)) {
	    BufReplace(buf, cmdData->leftPos, cmdData->rightPos, outText);
	    TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
	    cmdData->leftPos += len;
	    cmdData->rightPos = cmdData->leftPos;
	} else
	    fprintf(stderr, "NEdit: Too much binary data\n");
    }
    XtFree(outText);

    /* re-establish the timer proc (this routine) to continue processing */
    cmdData->flushTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell),
    	    OUTPUT_FLUSH_FREQ, flushTimeoutProc, clientData);
}

/*
** Clean up after the execution of a shell command sub-process and present
** the output/errors to the user as requested in the initial issueCommand
** call.  If "terminatedOnError" is true, don't bother trying to read the
** output, just close the i/o descriptors, free the memory, and restore the
** user interface state.
*/
static void finishCmdExecution(WindowInfo *window, int terminatedOnError)
{
    shellCmdInfo *cmdData = window->shellCmdData;
    textBuffer *buf;
    int status, failure, errorReport, reselectStart, outTextLen, errTextLen;
    int resp, cancel = False, fromMacro = cmdData->fromMacro;
    char *outText, *errText;

    /* Cancel any pending i/o on the file descriptors */
    if (cmdData->stdoutInputID != 0)
    	XtRemoveInput(cmdData->stdoutInputID);
    if (cmdData->stdinInputID != 0)
    	XtRemoveInput(cmdData->stdinInputID);
    if (cmdData->stderrInputID != 0)
    	XtRemoveInput(cmdData->stderrInputID);

    /* Close any file descriptors remaining open */
    close(cmdData->stdoutFD);
    if (cmdData->flags & ERROR_DIALOGS)
    	close(cmdData->stderrFD);
    if (cmdData->inPtr != NULL)
    	close(cmdData->stdinFD);

    /* Free the provided input text */
    if (cmdData->input != NULL)
	XtFree(cmdData->input);
    
    /* Cancel pending timeouts */
    if (cmdData->flushTimeoutID != 0)
    	XtRemoveTimeOut(cmdData->flushTimeoutID);
    if (cmdData->bannerTimeoutID != 0)
    	XtRemoveTimeOut(cmdData->bannerTimeoutID);
    
    /* Clean up waiting-for-shell-command-to-complete mode */
    if (!cmdData->fromMacro) {
	EndWait(window->shell);
	XtSetSensitive(window->cancelShellItem, False);
	if (cmdData->bannerIsUp)
    	    ClearModeMessage(window);
    }
    
    /* If the process was killed or became inaccessable, give up */
    if (terminatedOnError) {
	freeBufList(&cmdData->outBufs);
	freeBufList(&cmdData->errBufs);
    	waitpid(cmdData->childPid, &status, 0);
	goto cmdDone;
    }

    /* Assemble the output from the process' stderr and stdout streams into
       null terminated strings, and free the buffer lists used to collect it */
    outText = coalesceOutput(&cmdData->outBufs, &outTextLen);
    if (cmdData->flags & ERROR_DIALOGS)
    	errText = coalesceOutput(&cmdData->errBufs, &errTextLen);

    /* Wait for the child process to complete and get its return status */
    waitpid(cmdData->childPid, &status, 0);
    
    /* Present error and stderr-information dialogs.  If a command returned
       error output, or if the process' exit status indicated failure,
       present the information to the user. */
    if (cmdData->flags & ERROR_DIALOGS) {
	failure = WIFEXITED(status) && WEXITSTATUS(status) != 0;
	errorReport = *errText != '\0';
	if (failure && errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_WARN, window->shell, 2, "%s",
    	    	    "Cancel", "Proceed", errText);
    	    cancel = resp == 1;
	} else if (failure) {
    	    truncateString(outText, DF_MAX_MSG_LENGTH-70);
    	    resp = DialogF(DF_WARN, window->shell, 2,
    	       "Command reported failed exit status.\nOutput from command:\n%s",
    		    "Cancel", "Proceed", outText);
    	    cancel = resp == 1;
	} else if (errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_INF, window->shell, 2, "%s",
    	    	    "Proceed", "Cancel", errText);
    	    cancel = resp == 2;
	}
	XtFree(errText);
	if (cancel) {
	    XtFree(outText);
    	    goto cmdDone;
	}
    }
    
    /* If output is to a dialog, present the dialog.  Otherwise insert the
       (remaining) output in the text widget as requested, and move the
       insert point to the end */
    if (cmdData->flags & OUTPUT_TO_DIALOG) {
    	removeTrailingNewlines(outText);
	if (*outText != '\0')
    	    createOutputDialog(window->shell, outText);
    } else if (cmdData->flags & OUTPUT_TO_STRING) {
    	ReturnShellCommandOutput(window,outText, WEXITSTATUS(status));
    } else {
	buf = TextGetBuffer(cmdData->textW);
	if (!BufSubstituteNullChars(outText, outTextLen, buf)) {
	    fprintf(stderr,"NEdit: Too much binary data in shell cmd output\n");
	    outText[0] = '\0';
	}
	if (cmdData->flags & REPLACE_SELECTION) {
	    reselectStart = buf->primary.rectangular ? -1 : buf->primary.start;
	    BufReplaceSelected(buf, outText);
	    TextSetCursorPos(cmdData->textW, buf->cursorPosHint);
	    if (reselectStart != -1)
	    	BufSelect(buf, reselectStart, reselectStart + strlen(outText));
	} else {
	    BufReplace(buf, cmdData->leftPos, cmdData->rightPos, outText);
	    TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
	}
    }

    /* If the command requires the file to be reloaded afterward, reload it */
    if (cmdData->flags & RELOAD_FILE_AFTER)
    	RevertToSaved(window);

    /* Command is complete, free data structure and continue macro execution */
    XtFree(outText);
cmdDone:
    XtFree((char *)cmdData);
    window->shellCmdData = NULL;
    if (fromMacro)
    	ResumeMacroExecution(window);
}

/*
** Fork a subprocess to execute a command, return file descriptors for pipes
** connected to the subprocess' stdin, stdout, and stderr streams.  cmdDir
** sets the default directory for the subprocess.  If stderrFD is passed as
** NULL, the pipe represented by stdoutFD is connected to both stdin and
** stderr.  The function value returns the pid of the new subprocess, or -1
** if an error occured.
*/
static pid_t forkCommand(Widget parent, char *command, char *cmdDir,
	int *stdinFD, int *stdoutFD, int *stderrFD)
{
    int childStdoutFD, childStdinFD, childStderrFD, pipeFDs[2];
    int dupFD;
    pid_t childPid;
    
    /* Ignore SIGPIPE signals generated when user attempts to provide
       input for commands which don't take input */
    signal(SIGPIPE, SIG_IGN);
    
    /* Create pipes to communicate with the sub process.  One end of each is
       returned to the caller, the other half is spliced to stdin, stdout
       and stderr in the child process */
    if (pipe(pipeFDs) != 0) {
    	perror("Nedit: Internal error (opening stdout pipe)");
        return -1;
    }
    *stdoutFD = pipeFDs[0];
    childStdoutFD = pipeFDs[1];
    if (pipe(pipeFDs) != 0) {
    	perror("Nedit: Internal error (opening stdin pipe)");
        return -1;
    }
    *stdinFD = pipeFDs[1];
    childStdinFD = pipeFDs[0];
    if (stderrFD == NULL)
    	childStderrFD = childStdoutFD;
    else {
	if (pipe(pipeFDs) != 0) {
    	    perror("Nedit: Internal error (opening stdin pipe)");
            return -1;
        }
	*stderrFD = pipeFDs[0];
	childStderrFD = pipeFDs[1];
    }
    
    /* Fork the process */
    childPid = fork();
    
    /*
    ** Child process context (fork returned 0), clean up the
    ** child ends of the pipes and execute the command
    */
    if (0 == childPid) {

	/* close the parent end of the pipes in the child process   */
	close(*stdinFD);
	close(*stdoutFD);
	if (stderrFD != NULL)
	    close(*stderrFD);

	/* close current stdin, stdout, and stderr file descriptors before
	   substituting pipes */
	close(fileno(stdin));
	close(fileno(stdout));
	close(fileno(stderr));

	/* duplicate the child ends of the pipes to have the same numbers
	   as stdout & stderr, so it can substitute for stdout & stderr */
 	dupFD = dup2(childStdinFD, fileno(stdin));
	if (dupFD == -1)
	    perror("dup of stdin failed");
 	dupFD = dup2(childStdoutFD, fileno(stdout));
	if (dupFD == -1)
	    perror("dup of stdout failed");
 	dupFD = dup2(childStderrFD, fileno(stderr));
	if (dupFD == -1)
	    perror("dup of stderr failed");
	
	/* make this process the leader of a new process group, so the sub
	   processes can be killed, if necessary, with a killpg call */
#ifndef __EMX__  /* OS/2 doesn't have this */
	setsid();
#endif
      
       /* change the current working directory to the directory of the current
	  file. */ 
       if(cmdDir[0] != 0)
	   if(chdir(cmdDir) == -1)
	       perror("chdir to directory of current file failed");
     
	/* execute the command using the shell specified by preferences */
	execl(GetPrefShell(), GetPrefShell(), "-c", command, 0);

	/* if we reach here, execl failed */
	fprintf(stderr, "Error starting shell: %s\n", GetPrefShell());
	exit(1);
    }
    
    /* Parent process context, check if fork succeeded */
    if (childPid == -1)
    	DialogF(DF_ERR, parent, 1,
		"Error starting shell command process\n(fork failed)",
		"Dismiss");

    /* close the child ends of the pipes */
    close(childStdinFD);
    close(childStdoutFD);
    if (stderrFD != NULL)
    	close(childStderrFD);

    return childPid;
}    

/*
** Add a buffer full of output to a buffer list
*/
static void addOutput(buffer **bufList, buffer *buf)
{
    buf->next = *bufList;
    *bufList = buf;
}

/*
** coalesce the contents of a list of buffers into a contiguous memory block,
** freeing the memory occupied by the buffer list.  Returns the memory block
** as the function result, and its length as parameter "length".
*/
static char *coalesceOutput(buffer **bufList, int *outLength)
{
    buffer *buf, *rBufList = NULL;
    char *outBuf, *outPtr, *p;
    int i, length = 0;
    
    /* find the total length of data read */
    for (buf=*bufList; buf!=NULL; buf=buf->next)
    	length += buf->length;
    
    /* allocate contiguous memory for returning data */
    outBuf = XtMalloc(length+1);
    
    /* reverse the buffer list */
    while (*bufList != NULL) {
    	buf = *bufList;
    	*bufList = buf->next;
    	buf->next = rBufList;
    	rBufList = buf;
    }
    
    /* copy the buffers into the output buffer */
    outPtr = outBuf;
    for (buf=rBufList; buf!=NULL; buf=buf->next) {
    	p = buf->contents;
    	for (i=0; i<buf->length; i++)
    	    *outPtr++ = *p++;
    }
    
    /* terminate with a null */
    *outPtr = '\0';

    /* free the buffer list */
    freeBufList(&rBufList);
    
    *outLength = outPtr - outBuf;
    return outBuf;
}

static void freeBufList(buffer **bufList)
{
    buffer *buf;
    
    while (*bufList != NULL) {
    	buf = *bufList;
    	*bufList = buf->next;
    	XtFree((char *)buf);
    }
}

/*
** Remove trailing newlines from a string by substituting nulls
*/
static void removeTrailingNewlines(char *string)
{
    char *endPtr = &string[strlen(string)-1];
    
    while (endPtr >= string && *endPtr == '\n')
    	*endPtr-- = '\0';
}

/*
** Create a dialog for the output of a shell command.  The dialog lives until
** the user presses the Dismiss button, and is then destroyed
*/
static void createOutputDialog(Widget parent, char *text)
{
    Arg al[50];
    int ac, rows, cols, hasScrollBar;
    Widget form, textW, button;
    XmString st1;

    /* measure the width and height of the text to determine size for dialog */
    measureText(text, MAX_OUT_DIALOG_COLS, &rows, &cols);
    if (rows > MAX_OUT_DIALOG_ROWS) {
    	rows = MAX_OUT_DIALOG_ROWS;
    	hasScrollBar = True;
    } else
    	hasScrollBar = False;
    if (cols > MAX_OUT_DIALOG_COLS)
    	cols = MAX_OUT_DIALOG_COLS;
    if (cols == 0)
    	cols = 1;
    
    ac = 0;
    form = XmCreateFormDialog(parent, "shellOutForm", al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("Dismiss")); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_NONE);  ac++;
    button = XmCreatePushButtonGadget(form, "dismiss", al, ac);
    XtManageChild(button);
    XtVaSetValues(form, XmNdefaultButton, button, 0);
    XmStringFree(st1);
    XtAddCallback(button, XmNactivateCallback, destroyOutDialogCB,
    	    XtParent(form));
    
    ac = 0;
    XtSetArg(al[ac], XmNrows, rows);  ac++;
    XtSetArg(al[ac], XmNcolumns, cols);  ac++;
    XtSetArg(al[ac], XmNresizeHeight, False);  ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNwordWrap, True);  ac++;
    XtSetArg(al[ac], XmNscrollHorizontal, False);  ac++;
    XtSetArg(al[ac], XmNscrollVertical, hasScrollBar);  ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNspacing, 0);  ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);  ac++;
    XtSetArg(al[ac], XmNeditable, False);  ac++;
    XtSetArg(al[ac], XmNvalue, text);  ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET);  ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomWidget, button);  ac++;
    textW = XmCreateScrolledText(form, "outText", al, ac);
    XtManageChild(textW);
    
    XtVaSetValues(XtParent(form), XmNtitle, "Output from Command", 0);
    ManageDialogCenteredOnPointer(form);
}

/*
** Dispose of the command output dialog when user presses Dismiss button
*/
static void destroyOutDialogCB(Widget w, XtPointer callback, XtPointer closure)
{
    XtDestroyWidget((Widget)callback);
}

/*
** Measure the width and height of a string of text.  Assumes 8 character
** tabs.  wrapWidth specifies a number of columns at which text wraps.
*/
static void measureText(char *text, int wrapWidth, int *rows, int *cols)
{
    int maxCols = 0, line = 1, col = 0;
    char *c;
    
    for (c=text; *c!='\0'; c++) {
    	if (*c=='\n' || col > wrapWidth) {
    	    line++;
    	    col = 0;
    	} else {
    	    if (*c == '\t')
    		col += 8 - (col % 8);
    	    else
    		col++;
    	    if (col > maxCols)
    	    	maxCols = col;
    	}
    }
    *rows = line;
    *cols = maxCols;
}

/*
** Truncate a string to a maximum of length characters.  If it shortens the
** string, it appends "..." to show that it has been shortened. It assumes
** that the string that it is passed is writeable.
*/
static void truncateString(char *string, int length)
{
    if (strlen(string) > length)
	memcpy(&string[length-3], "...", 4);
}

/*
** Substitute the string subsStr in inStr wherever % appears, storing the
** result in outStr.  Returns False if the resulting string would be
** longer than outLen
*/
static int substitutePercent(char *outStr, char *inStr, char *subsStr,
	int outLen)
{
    char *inChar, *outChar, *c;
    
    inChar = inStr;
    outChar = outStr;
    while (*inChar != '\0') {
    	if (*inChar == '%') {
    	    if (*(inChar+1) == '%') {
    	    	inChar += 2;
    	    	*outChar++ = '%';
    	    } else {
    		for (c=subsStr; *c!='\0'; c++)
    	    	    *outChar++ = *c;
    		inChar++;
    	    }
    	} else
    	    *outChar++ = *inChar++;
    	if (outChar - outStr >= outLen)
    	    return False;
    }
    *outChar = '\0';
    return True;
}

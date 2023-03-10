/*******************************************************************************
*									       *
* shell.c -- Nirvana Editor shell command execution			       *
*									       *
* Copyright (c) 1991 Universities Research Association, Inc.		       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retaFins a paid-up,     *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warranty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* December, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)shell.c	1.10     10/31/94";
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef IBM
#define NBBY 8
#include <sys/select.h>
#endif
#include <time.h>
#include <errno.h>
#include <signal.h>
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

#define FILTER_BUF_SIZE 256	/* size of buffers for collecting cmd output */
#define MAX_SHELL_CMD_LEN 255	/* max length of a shell command (should be
				   eliminated, but substitutePercent needs) */
#define MAX_OUT_DIALOG_ROWS 30	/* max height of dialog for command output */
#define MAX_OUT_DIALOG_COLS 80	/* max width of dialog for command output */

/* flags for issueCommand */
#define ACCUMULATE 1
#define ERROR_DIALOGS 2
#define REPLACE_SELECTION 4

/* element of a buffer list for collecting output from filter processes */
typedef struct bufElem {
    struct bufElem *next;
    int length;
    char contents[FILTER_BUF_SIZE];
} buffer;

static char *issueCommand(WindowInfo *window, char *command, char *input,
	int flags, Widget textW, int replaceLeft, int replaceRight,
	int *success);
static pid_t forkCommand(Widget parent, char *command, int *stdinFD,
	int *stdoutFD, int *stderrFD);
static void addOutput(buffer **bufList, buffer *buf);
static char *coalesceOutput(buffer **bufList);
static void freeBufList(buffer **bufList);
static void removeTrailingNewlines(char *string);
static void createOutputDialog(Widget parent, char *text);
static void measureText(char *text, int wrapWidth, int *rows, int *cols);
static void truncateString(char *string, int length);
static int substitutePercent(char *outStr, char *inStr, char *subsStr,
	int outLen);

/* Flag indicating shell command in progress */
static int ShellCmdInProgress = False;

/* Window where shell command is being executed */
static WindowInfo *ShellCmdWindow;

/* Flag for aborting a shell command in progress */
static int AbortFlag = False;

/*
** Filter the current selection through shell command "command".  The selection
** is removed, and replaced by the output from the command execution.  Failed
** command status and output to stderr are presented in dialog form.
*/
void FilterSelection(WindowInfo *window, char *command)
{
    int success, left, right;
    char *text;

    /* This code is not reentrant.  Can't do two shell commands at once */
    if (ShellCmdInProgress) {
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
    left = window->buffer->primary.start;
    right = window->buffer->primary.end;
    
    /* Issue the command and collect its output */
    issueCommand(window, command, text, ACCUMULATE | ERROR_DIALOGS |
    	    REPLACE_SELECTION, window->lastFocus, left, right, &success);

    XtFree(text);
}

/*
** Execute shell command "command", depositing the result at the current
** insert position or in the current selection if if the window has a
** selection.
*/
void ExecShellCommand(WindowInfo *window, char *command)
{
    int success, left, right, flags = 0;

    /* This code is not reentrant.  Can't do two shell commands at once */
    if (ShellCmdInProgress) {
    	XBell(TheDisplay, 0);
    	return;
    }
    
    /* get the selection or the insert position */
    if (GetSimpleSelection(window->buffer, &left, &right))
    	flags = ACCUMULATE | REPLACE_SELECTION;
    else
    	left = right = TextGetCursorPos(window->lastFocus);
    
    /* issue the command and collect its output */
    issueCommand(window, command, "", flags, window->lastFocus,
    	    left, right, &success);
}

/*
** Execute the line of text where the the insertion cursor is positioned
** as a shell command.
*/
void ExecCursorLine(WindowInfo *window)
{
    char *cmdText;
    int success, left, right, insertPos;

    /* This code is not reentrant.  Can't do two shell commands at once */
    if (ShellCmdInProgress) {
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
    
    /* insert a newline after the entire line */
    BufInsert(window->buffer, insertPos, "\n");

    /* issue the command */
    issueCommand(window, cmdText, "", 0, window->lastFocus,
    	   insertPos+1, insertPos+1, &success);

    XtFree(cmdText);
}

/*
** Do a shell command, with the options allowed to users (input source,
** output destination, save first and load after) in the shell commands
** menu.
*/
void DoShellMenuCmd(WindowInfo *window, char *command, int input, int output,
	int outputReplacesInput, int saveFirst, int loadAfter) 
{
    int success, flags = 0;
    char *text, *filteredText;
    char subsCommand[MAX_SHELL_CMD_LEN], fullName[MAXPATHLEN];
    int left, right;
    WindowInfo *inWindow = window;
    Widget outWidget;

    /* This code is not reentrant.  Can't do two shell commands at once */
    if (ShellCmdInProgress) {
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
    	text = "";
    
    /* Assign the output destination and create a new window if necessary */
    if (output == TO_DIALOG) {
    	outWidget = NULL;
    	left = right = 0;
    } else if (output == TO_NEW_WINDOW) {
    	EditNewFile();
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
    
    /* issue the command and collect its output */
    filteredText = issueCommand(inWindow, subsCommand, text, flags,
    	    outWidget, left, right, &success);

    /* If the command requires the file to be reloaded afterward, reload it */
    if (loadAfter && success)
    	RevertToSaved(window, True);
    	
    /* if the output is to a dialog, present the dialog with the output */
    if (output == TO_DIALOG && filteredText != NULL) {
    	removeTrailingNewlines(filteredText);
	if (*filteredText != '\0')
    	    createOutputDialog(window->shell, filteredText);
    	XtFree(filteredText);
    }
    
    if (input != FROM_NONE)
    	XtFree(text);
}

/*
** Cancel the shell command in progress
*/
void AbortShellCommand(void)
{
    AbortFlag = True;
}

/*
** Issue a a command, feed it the string input, and either return or store its
** output in a text widget (if textW is NULL, return the output from the
** command as a string to be freed by the caller, otherwise store the output
** between leftPos and rightPos in the text widget textW).  Flags may be set
** to ACCUMULATE, REPLACE_SELECTION, and/or ERROR_DIALOGS.  ACCUMULATE causes
** output from the command to be saved up until the command completes. 
** ERROR_DIALOGS presents stderr output separately in popup a dialog, and also
** reports failed exit status as a popup dialog including the command output.
** REPLACE_SELECTION causes output to replace the selection in textW. Both
** REPLACE_SELECTION and ERROR_DIALOGS can only be used along with ACCUMULATE.
** REPLACE_SELECTION can only be used with text (as opposed to XmText) widgets.
*/
static char *issueCommand(WindowInfo *window, char *command, char *input,
	int flags, Widget textW, int replaceLeft, int replaceRight,
	int *success)
{
    int status, stdinFD, stdoutFD, stderrFD, maxFD;
    int len, leftPos = replaceLeft, rightPos = replaceRight;
    pid_t childPid;
    int nWritten, nRead;
    buffer *buf, *outBufs = NULL, *errBufs = NULL;
    char *outText, *errText, *inPtr = input;
    int resp, inLength = strlen(input);
    fd_set readfds, writefds;
    struct timeval timeout;
    int failure, errorReport, cancel = False, bannerIsUp = False;
    int outEOF = False, errEOF = (flags & ERROR_DIALOGS) ? False : True;
    XtAppContext context = XtDisplayToApplicationContext(TheDisplay);
    time_t startTime = time(NULL);
    time_t lastIOTime = time(NULL);
    
    *success = False;
    
    /* verify consistency of input parameters */
    if ((flags & ERROR_DIALOGS || flags & REPLACE_SELECTION) &&
    	    !(flags & ACCUMULATE))
    	return NULL;
    
    /* put up a watch cursor over the waiting window */
    BeginWait(window->shell);
    
    /* enable the cancel menu item */
    XtSetSensitive(window->cancelShellItem, True);
    
    /* Indicate which window this is all happening in, block other attempts */
    ShellCmdWindow = window;
    ShellCmdInProgress = True;

    /* fork the subprocess and issue the command */
    childPid = forkCommand(window->shell, command, &stdinFD, &stdoutFD, 
    	    (flags & ERROR_DIALOGS) ? &stderrFD: NULL);
    
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
    if (inLength == 0)
    	close(stdinFD);
    	
    /*
    ** Loop writing input to process and reading input from it until
    ** end-of-file is reached on both stdout and stderr pipes.
    */
    AbortFlag = False;
    while (!(outEOF && errEOF)) {
    
	/* Process all pending X events, regardless of whether
	   select says there are any. */
	while (XtAppPending(context)) {
	    XtAppProcessEvent(context, XtIMAll);
	}
   
	/* If the process is taking too long, put up a message */
	if (!bannerIsUp &&  time(NULL) >= startTime + 6) {
	    bannerIsUp = True;
	    SetModeMessage(window,
    		    "Shell Command in Progress -- Press Ctrl+. to Cancel");
	}
	
	/* Check the abort flag set by the Cancel Shell Command menu item */
	if (AbortFlag) {
	    kill(-childPid, SIGTERM);
    	    close(stdoutFD);
    	    if (flags & ERROR_DIALOGS)
    	    	close(stderrFD);
    	    goto errorReturn;
	}
	
	/* Block and wait for something to happen, but wakeup every second
	   to check abort flag and waiting dialog and output timers */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(ConnectionNumber(TheDisplay), &readfds);
	maxFD = ConnectionNumber(TheDisplay);
	if (!outEOF) {
	    FD_SET(stdoutFD, &readfds);
	    maxFD = stdoutFD > maxFD ? stdoutFD : maxFD;
	}
	if (!errEOF) {
	    FD_SET(stderrFD, &readfds);
	    maxFD = stderrFD > maxFD ? stderrFD : maxFD;
	}
	if (inLength > 0) {
	    FD_SET(stdinFD, &writefds);
	    maxFD = stdinFD > maxFD ? stdinFD : maxFD;
	}
	if (select(maxFD+1, &readfds, &writefds, NULL, &timeout) == -1) {
	    if (EINTR != errno)
		perror("NEdit: select");
	}
	
	/* Dump intermediate output to window if the process is taking a long
	   time.  If there's read data pending, hold off so that the output
	   is in bigger chunks (each is an undo operation) */
	if (!(flags & ACCUMULATE) && textW!=NULL &&
		!FD_ISSET(stdoutFD, &readfds) && time(NULL) >= lastIOTime + 3) {
    	    outText = coalesceOutput(&outBufs);
	    len = strlen(outText);
	    if (len != 0) {
	    	if (XtClass(textW) == textWidgetClass)
	    	    BufReplace(TextGetBuffer(textW), leftPos, rightPos,outText);
	    	else
	    	    XmTextReplace(textW, leftPos, rightPos, outText);
		XtFree(outText);
		leftPos += len;
		rightPos = leftPos;
	    }
	}
	
	/* write input to the sub-process stdin, close stdin when finished */
	if (FD_ISSET(stdinFD, &writefds) && inLength > 0) {
	    nWritten = write(stdinFD, inPtr, inLength);
	    if (nWritten == -1) {
    		if (errno != EWOULDBLOCK && errno != EAGAIN) {
    		    perror("NEdit: Write to filter command failed");
    		    goto errorReturn;
    		}
	    } else {
		inPtr += nWritten;
		inLength -= nWritten;
		if (inLength <= 0)
    		    close(stdinFD);
    	    }
    	}

    	/* read the output from stdout and create a linked list of buffers */
    	if (FD_ISSET(stdoutFD, &readfds)) {
    	    buf = (buffer *)XtMalloc(sizeof(buffer));
    	    nRead = read(stdoutFD, buf->contents, FILTER_BUF_SIZE);
    	    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
		    perror("NEdit: Error reading filter output");
		    XtFree((char *)buf);
		    goto errorReturn;
		}
    	    } else if (nRead == 0) { /* eof */
    		outEOF = True;
    		XtFree((char *)buf);
    	    } else { /* characters read */
    		buf->length = nRead;
    		addOutput(&outBufs, buf);
   	    }
    	}

    	/* read the output from stderr and create a linked list of buffers */
    	if ((flags & ERROR_DIALOGS) && FD_ISSET(stderrFD, &readfds)) {
    	    buf = (buffer *)XtMalloc(sizeof(buffer));
    	    nRead = read(stderrFD, buf->contents, FILTER_BUF_SIZE);
    	    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
		    perror("NEdit: Error reading filter error stream");
		    XtFree((char *)buf);
		    goto errorReturn;
		}
    	    } else if (nRead == 0) { /* eof */
    		errEOF = True;
    		XtFree((char *)buf);
    	    } else { /* chars read */
    		buf->length = nRead;
    		addOutput(&errBufs, buf);
   	    }
    	}
    }
    close(stdoutFD);
    if (flags & ERROR_DIALOGS)
    	close(stderrFD);
    
    /* assemble the output from the process' stderr and stdout streams into
       null terminated strings, and free the buffer lists used to collect it */
    outText = coalesceOutput(&outBufs);
    if (flags & ERROR_DIALOGS)
    	errText = coalesceOutput(&errBufs);

    /* Wait for the child process to complete and get its return status */
    waitpid(childPid, &status, 0);
    
    /* Clean up waiting-for-shell-command-to-complete mode */
    EndWait(window->shell);
    XtSetSensitive(window->cancelShellItem, False);
    if (bannerIsUp)
    	ClearModeMessage(window);
    ShellCmdInProgress = False;
    
    /* Present error and stderr-information dialogs.  If a command returned
       error output, or if the process' exit status indicated failure,
       present the information to the user. */
    if (flags & ERROR_DIALOGS) {
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
    	    return NULL;
	}
    }
    
    *success = True;
    
    /* insert the remaining output, and move the insert point to the end */
    if (textW != NULL) {
	if (flags & REPLACE_SELECTION) {
	    BufReplaceSelected(TextGetBuffer(textW), outText);
	    TextSetCursorPos(textW, TextGetBuffer(textW)->cursorPosHint);
	} else {
	    if (XtClass(textW) == textWidgetClass) {
		BufReplace(TextGetBuffer(textW), leftPos, rightPos, outText);
		TextSetCursorPos(textW, leftPos + strlen(outText));
	    } else {
		XmTextReplace(textW, leftPos, rightPos, outText);
		XmTextSetInsertionPosition(textW, leftPos + strlen(outText));
	    }
	}
	XtFree(outText);
	return NULL;
    }
    return outText;

errorReturn: /* Clean up when things go bad */

    freeBufList(&outBufs);
    freeBufList(&errBufs);
    if (bannerIsUp)
    	ClearModeMessage(window);
    EndWait(window->shell);
    XtSetSensitive(window->cancelShellItem, False);
    ShellCmdInProgress = False;
    return NULL;
}

/*
** Fork a subprocess to execute a command, return file descriptors for pipes
** connected to the subprocess' stdin, stdout, and stderr streams.  If
** stderrFD is passed as NULL, the pipe represented by stdoutFD is connected
** to both stdin and stderr.  The function value returns the pid of the new
** subprocess, or -1 if an error occured.
*/
static pid_t forkCommand(Widget parent, char *command, int *stdinFD,
	int *stdoutFD, int *stderrFD)
{
    int childStdoutFD, childStdinFD, childStderrFD, pipeFDs[2];
    int dupFD;
    pid_t childPid;
    
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
    
    /* Fork the filter process to run the command */
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
	setsid();
	
	/* execute the command using the shell specified by preferences */
	execl(GetPrefShell(), GetPrefShell(), "-c", command, 0);

	/* if we reach here, execl failed */
	fprintf(stderr, "Error starting shell: %s\n", GetPrefShell());
	exit(1);
    }
    
    /* Parent process context, check if fork succeeded */
    if (childPid == -1)
    	DialogF(DF_ERR, parent, 1,
		"Error starting filter process\n(fork failed)",
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
** freeing the memory occupied by the buffer list.
*/
static char *coalesceOutput(buffer **bufList)
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
    *outPtr++ = '\0';

    /* free the buffer list */
    freeBufList(&rBufList);
    
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
    form = XmCreateFormDialog(parent, "helpForm", al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("Dismiss")); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
    XtSetArg (al[ac], XmNtopAttachment, XmATTACH_NONE);  ac++;
    button = XmCreatePushButtonGadget(form, "dismiss", al, ac);
    XtManageChild(button);
    XtVaSetValues(form, XmNdefaultButton, button, 0);
    XmStringFree(st1);
    
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
** truncates a string to a maximum of length characters.  If it
** shortens the string, it appends "..." to show that it has been shortened.
** It assumes that the string that it is passed is writeable.
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
    	    for (c=subsStr; *c!='\0'; c++)
    	    	*outChar++ = *c;
    	    inChar++;
    	} else
    	    *outChar++ = *inChar++;
    	if (outChar - outStr >= outLen)
    	    return False;
    }
    *outChar = '\0';
    return True;
}

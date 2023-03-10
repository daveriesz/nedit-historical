/*******************************************************************************
*									       *
* server.c -- Nirvana Editor edit-server component			       *
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
* November, 1995							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#ifdef VMS
#include <lib$routines.h>
#include ssdef
#include syidef
#include "../util/VMSparam.h"
#include "../util/VMSutils.h"
#else
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#endif
#include <Xm/Xm.h>
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "selection.h"
#include "macro.h"
#include "menu.h"
#include "server.h"

/* If anyone knows where to get this from system include files (in a machine
   independent way), please change this (L_cuserid is apparently not ANSI) */
#define MAXUSERNAMELEN 32

#if defined(VMS) || defined(linux)
#define MAXNODENAMELEN (MAXPATHLEN+2)
#elif defined(SUNOS)
#define MAXNODENAMELEN 9
#else
#define MAXNODENAMELEN SYS_NMLN
#endif

static void processServerCommand(void);
static void cleanUpServerCommunication(void);
static char *getUserName(void);
static void getHostName(char *hostname);
static void processServerCommandString(char *string);

static Atom ServerRequestAtom = 0;
static Atom ServerExistsAtom = 0;

/*
** Set up inter-client communication for NEdit server end, expected to be
** called only once at startup time
*/
void InitServerCommunication(void)
{
    Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
    char *userName, propName[23+MAXNODENAMELEN+MAXUSERNAMELEN];
    char hostName[MAXNODENAMELEN+1];

    /* Create server property atoms.  Atom names are generated by
       concatenating NEDIT_SERVER_REQUEST_, and NEDIT_SERVER_EXITS_
       with hostname and user name. */
    userName = getUserName();
    getHostName(hostName);
    sprintf(propName, "NEDIT_SERVER_EXISTS_%s_%s", hostName, userName);
    ServerExistsAtom = XInternAtom(TheDisplay, propName, False);
    sprintf(propName, "NEDIT_SERVER_REQUEST_%s_%s", hostName, userName);
    ServerRequestAtom = XInternAtom(TheDisplay, propName, False);
    
    /* Create the server-exists property on the root window to tell clients
       whether to try a request (otherwise clients would always have to
       try and wait for their timeouts to expire) */
    XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom, XA_STRING, 8,
    	    PropModeReplace, (unsigned char *)"True", 4);
    
    /* Set up exit handler for cleaning up server-exists property */
    atexit(cleanUpServerCommunication);
    
    /* Pay attention to PropertyChangeNotify events on the root window */
    XSelectInput(TheDisplay, rootWindow, PropertyChangeMask);
}

/*
** Exit handler.  Removes server-exists property on root window
*/
static void cleanUpServerCommunication(void)
{
    /* Delete the server-exists property from the root window (if it was
       assigned) and don't let the process exit until the X server has
       processed the delete request (otherwise it won't be done) */
    if (ServerExistsAtom != 0) {
	XDeleteProperty(TheDisplay, RootWindow(TheDisplay,
		DefaultScreen(TheDisplay)), ServerExistsAtom);
	XSync(TheDisplay, True);
    }
}

/*
** Special event loop for NEdit servers.  Processes PropertyNotify events on
** the root window (this would not be necessary if it were possible to
** register an Xt event-handler for a window, rather than only a widget).
** Invokes server routines when a server-request property appears,
** re-establishes server-exists property when another server exits and
** this server is still alive to take over.
*/
void ServerMainLoop(XtAppContext context)
{
    XEvent event;
    XPropertyEvent *e = (XPropertyEvent *)&event;
    Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));

    while (TRUE) {
        XtAppNextEvent(context, &event);
        if (e->window == rootWindow) {
            if (e->atom == ServerRequestAtom && e->state == PropertyNewValue)
            	processServerCommand();
            else if (e->atom == ServerExistsAtom && e->state == PropertyDelete)
            	 XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom,
            	     XA_STRING, 8, PropModeReplace, (unsigned char *)"True", 4);
        }
    	XtDispatchEvent(&event);
    }
}

static void processServerCommand(void)
{
    Atom dummyAtom;
    unsigned long nItems, dummyULong;
    unsigned char *propValue;
    int getFmt;

    /* Get the value of the property, and delete it from the root window */
    if (XGetWindowProperty(TheDisplay, RootWindow(TheDisplay,
    	    DefaultScreen(TheDisplay)), ServerRequestAtom, 0, INT_MAX, True,
    	    XA_STRING, &dummyAtom, &getFmt, &nItems, &dummyULong, &propValue)
    	    != Success || getFmt != 8)
    	return;
    
    /* Invoke the command line processor on the string to process the request */
    processServerCommandString((char *)propValue);
    XFree(propValue);
}

/*
** Return a pointer to the username of the current user in a statically
** allocated string.
*/
static char *getUserName(void)
{
#ifdef VMS
    return cuserid(NULL);
#else
    /* This should be simple, but cuserid has apparently been dropped from
       the ansi C standard, and if strict ansi compliance is turned on (on
       Sun anyhow, maybe others), calls to cuserid fail to compile.
       Unfortunately the alternative is this weird sequence of getlogin
       followed by getpwuid.  Getlogin only works if a terminal is attached &
       there can be more than one name associated with a uid (really?).  Both
       calls return a pointer to a static area. */
    char *name;
    struct passwd *passwdEntry;
    
    name = getlogin();
    if (name == NULL || name[0] == '\0') {
    	passwdEntry = getpwuid(getuid());
    	name = passwdEntry->pw_name;
    }
    return name;
#endif
}

/*
** Writes the hostname of the current system in string "hostname".
*/
static void getHostName(char *hostname)
{
#ifdef VMS
    /* This should be simple, but uname is not supported in the DEC C RTL and
       gethostname on VMS depends either on Multinet or UCX.  So use uname 
       on Unix, and use LIB$GETSYI on VMS. Note the VMS hostname will
       be in DECNET format with trailing double colons, e.g. "FNALV1::".    */
    int syi_status;
    struct dsc$descriptor_s *hostnameDesc;
    unsigned long int syiItemCode = SYI$_NODENAME;	/* get Nodename */
    unsigned long int unused = 0;
    unsigned short int hostnameLen = MAXNODENAMELEN+1;
    
    hostnameDesc = NulStrWrtDesc(hostname, MAXNODENAMELEN+1);
    syi_status = lib$getsyi(&syiItemCode, &unused, hostnameDesc, &hostnameLen,
    			    0, 0);
    if (syi_status != SS$_NORMAL) {
	fprintf(stderr, "Error return from lib$getsyi: %d", syi_status);
	strcpy(hostname, "VMS");
    } else
    	hostname[hostnameLen] = '\0';
    FreeStrDesc(hostnameDesc);
#else
    struct utsname nameStruct;

    uname(&nameStruct);
    strcpy(hostname, nameStruct.nodename);
#endif
}

static void processServerCommandString(char *string)
{
    char *fullname, filename[MAXPATHLEN], pathname[MAXPATHLEN];
    char *doCommand, *inPtr;
    int editFlags, stringLen = strlen(string);
    int lineNum, createFlag, readFlag, fileLen, doLen, charsRead, itemsRead;
    WindowInfo *window;

    /* If the command string is empty, put up an empty, Untitled window
       (or just pop one up if it already exists) */
    if (string[0] == '\0') {
    	for (window=WindowList; window!=NULL; window=window->next)
    	    if (!window->filenameSet && !window->fileChanged)
    	    	break;
    	if (window == NULL) {
    	    EditNewFile();
    	    CheckCloseDim();
    	} else
    	    XMapRaised(TheDisplay, XtWindow(window->shell));
    	return;
    }

    /*
    ** Loop over all of the files in the command list
    */
    inPtr = string;
    while (TRUE) {
	
	if (*inPtr == '\0')
	    break;
	    
	/* Read a server command from the input string.  Header contains:
	   linenum createFlag fileLen doLen\n, followed by a filename and -do
	   command both followed by newlines.  This bit of code reads the
	   header, and converts the newlines following the filename and do
	   command to nulls to terminate the filename and doCommand strings */
	itemsRead = sscanf(inPtr, "%d %d %d %d %d\n%n", &lineNum, &readFlag,
    		&createFlag, &fileLen, &doLen, &charsRead);
	if (itemsRead != 5)
    	    goto readError;
	inPtr += charsRead;
	if (inPtr - string + fileLen > stringLen)
	    goto readError;
	fullname = inPtr;
	inPtr += fileLen;
	*inPtr++ = '\0';
	if (inPtr - string + doLen > stringLen)
	    goto readError;
	doCommand = inPtr;
	inPtr += doLen;
	*inPtr++ = '\0';
	
	/* Process the filename by looking for the files in an
	   existing window, or opening if they don't exist */
	editFlags = (readFlag ? FORCE_READ_ONLY : 0) |
		(createFlag ? SUPPRESS_CREATE_WARN | CREATE : 0);
	ParseFilename(fullname, filename, pathname);
    	window = FindWindowWithFile(filename, pathname);
    	if (window == NULL) {
	    EditExistingFile(WindowList, filename, pathname, editFlags);
	    window = WindowList;
	}
	
	/* Do the actions requested (note DoMacro is last, since the do
	   command can do anything, including closing the window!) */
	XMapRaised(TheDisplay, XtWindow(window->shell));
	if (lineNum > 0)
	    SelectNumberedLine(window, lineNum);
	if (*doCommand != '\0')
	    DoMacro(window, doCommand);
    }
    CheckCloseDim();
    return;

readError:
    fprintf(stderr, "NEdit: error processing server request\n");
    return;
}

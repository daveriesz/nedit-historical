static const char CVSID[] = "$Id: server.c,v 1.9 2001/08/14 08:37:16 jlous Exp $";
/*******************************************************************************
*									       *
* server.c -- Nirvana Editor edit-server component			       *
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
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
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
#include <sys/types.h>
#include <sys/utsname.h>
#ifndef __MVS__
#include <sys/param.h>
#endif
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
#include "preferences.h"

/* If anyone knows where to get this from system include files (in a machine
   independent way), please change this (L_cuserid is apparently not ANSI) */
#define MAXUSERNAMELEN 32

/* Ditto for the maximum length for a node name.  SYS_NMLN is not available
   on most systems, and I don't know what the portable alternative is. */
#ifdef SYS_NMLN
#define MAXNODENAMELEN SYS_NMLN
#else
#define MAXNODENAMELEN (MAXPATHLEN+2)
#endif

static void processServerCommand(void);
static void cleanUpServerCommunication(void);
static const char *getUserName(void);
static const char *getHostName(void);
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
    const char *userName, *hostName;
    char propName[24+MAXNODENAMELEN+MAXUSERNAMELEN+MAXPATHLEN];
    char serverName[MAXPATHLEN+2];

    /* Create server property atoms.  Atom names are generated by
       concatenating NEDIT_SERVER_REQUEST_, and NEDIT_SERVER_EXITS_
       with hostname and user name. */
    userName = getUserName();
    hostName = getHostName();
    if (GetPrefServerName()[0] != '\0') {
    	serverName[0] = '_';
    	strcpy(&serverName[1], GetPrefServerName());
    } else
    	serverName[0] = '\0';
    sprintf(propName, "NEDIT_SERVER_EXISTS_%s_%s%s", hostName, userName,
    	    serverName);
    ServerExistsAtom = XInternAtom(TheDisplay, propName, False);
    sprintf(propName, "NEDIT_SERVER_REQUEST_%s_%s%s", hostName, userName,
    	    serverName);
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
static const char *getUserName(void)
{
#ifdef VMS
    return cuserid(NULL);
#else
    /* cuserid has apparently been dropped from the ansi C standard, and if
       strict ansi compliance is turned on (on Sun anyhow, maybe others), calls
       to cuserid fail to compile.  Older versions of nedit try to use the
       getlogin call first, then if that fails, use getpwuid and getuid.  This
       results in the user-name of the original terminal being used, which is
       not correct when the user uses the su command.  Now, getpwuid only: */
    struct passwd *passwdEntry;

    passwdEntry = getpwuid(getuid());
    if (!passwdEntry) {
       perror("NEdit: getpwuid() failed ");
       exit(EXIT_FAILURE);
    }
    return passwdEntry->pw_name;
#endif
}

/*
** Writes the hostname of the current system in string "hostname".
*/
static const char *getHostName(void)
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
    static char hostname[MAXNODENAMELEN+1];
    
    hostnameDesc = NulStrWrtDesc(hostname, MAXNODENAMELEN+1);
    syi_status = lib$getsyi(&syiItemCode, &unused, hostnameDesc, &hostnameLen,
    			    0, 0);
    if (syi_status != SS$_NORMAL) {
	fprintf(stderr, "Error return from lib$getsyi: %d", syi_status);
	strcpy(hostname, "VMS");
    } else
    	hostname[hostnameLen] = '\0';
    FreeStrDesc(hostnameDesc);
    return hostname;
#else
    struct utsname nameStruct;
    static char hostname[MAXNODENAMELEN+1];
    int rc;
    
    rc=uname(&nameStruct);
    if (rc<0) {
       perror("NEdit: uname() failed ");
       exit(EXIT_FAILURE);
    }   
    strcpy(hostname, nameStruct.nodename);
    return hostname;
#endif
}

static void processServerCommandString(char *string)
{
    char *fullname, filename[MAXPATHLEN], pathname[MAXPATHLEN];
    char *doCommand, *geometry, *langMode, *inPtr;
    int editFlags, stringLen = strlen(string);
    int lineNum, createFlag, readFlag, iconicFlag;
    int fileLen, doLen, lmLen, geomLen, charsRead, itemsRead;
    WindowInfo *window;

    /* If the command string is empty, put up an empty, Untitled window
       (or just pop one up if it already exists) */
    if (string[0] == '\0') {
    	for (window=WindowList; window!=NULL; window=window->next)
    	    if (!window->filenameSet && !window->fileChanged)
    	    	break;
    	if (window == NULL) {
    	    EditNewFile(NULL, False, NULL, NULL);
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
	itemsRead = sscanf(inPtr, "%d %d %d %d %d %d %d %d%n", &lineNum,
		&readFlag, &createFlag, &iconicFlag, &fileLen, &doLen,
		&lmLen, &geomLen, &charsRead);
	if (itemsRead != 8)
    	    goto readError;
	inPtr += charsRead + 1;
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
	if (inPtr - string + lmLen > stringLen)
	    goto readError;
	langMode = inPtr;
	inPtr += lmLen;
	*inPtr++ = '\0';
	if (inPtr - string + geomLen > stringLen)
	    goto readError;
	geometry = inPtr;
	inPtr += geomLen;
	*inPtr++ = '\0';
	
	/* An empty file name means: choose a random window for
	   executing the -do macro upon */
	if (fileLen <= 0) {
	    if (*doCommand != '\0')
	    	DoMacro(WindowList, doCommand, "-do macro");
	    CheckCloseDim();
	    return;
	}
	
	/* Process the filename by looking for the files in an
	   existing window, or opening if they don't exist */
	editFlags = (readFlag ? FORCE_READ_ONLY : 0) | CREATE |
		(createFlag ? SUPPRESS_CREATE_WARN : 0);
	ParseFilename(fullname, filename, pathname);
    	window = FindWindowWithFile(filename, pathname);
    	if (window == NULL)
	    window = EditExistingFile(WindowList, filename, pathname,
		    editFlags, geometry, iconicFlag, lmLen==0?NULL:langMode);
	
	/* Do the actions requested (note DoMacro is last, since the do
	   command can do anything, including closing the window!) */
	if (window != NULL) {
	    if (!iconicFlag)
	    	XMapRaised(TheDisplay, XtWindow(window->shell));
	    if (lineNum > 0)
		SelectNumberedLine(window, lineNum);
	    if (*doCommand != '\0')
		DoMacro(window, doCommand, "-do macro");
	}
    }
    CheckCloseDim();
    return;

readError:
    fprintf(stderr, "NEdit: error processing server request\n");
    return;
}
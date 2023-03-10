/*******************************************************************************
*									       *
* file.c -- Nirvana Editor file i/o					       *
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
static char SCCSID[] = "@(#)file.c	1.17     2/24/94";
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#include <fcntl.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "../util/printUtils.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "file.h"

static int doSave(WindowInfo *window);
static int doOpen(WindowInfo *window, char *name, char *path, int create);
static int removeBackupFile(WindowInfo *window);
static char *errorString(void);
#ifndef VMS
static int followSymLinks(char *filename);
#else
void removeVersionNumber(char *fromName, char *toName);
#endif /*VMS*/

void EditNewFile()
{
    WindowInfo *window, *w;
    int i;
    char name[20];

    /*... test for creatability? */
    
    /* Find a (relatively) unique name for the new file */
    for (i=0; i<INT_MAX; i++) {
    	if (i == 0)
    	    sprintf(name, "Untitled");
    	else
    	    sprintf(name, "Untitled_%d", i);
	for (w=WindowList; w!=NULL; w=w->next)
     	    if (!strcmp(w->filename, name))
    	    	break;
    	if (w == NULL)
    	    break;
    }

    /* create the window */
    window = CreateWindow(name, GetPrefRows(), GetPrefCols(),
    	    GetPrefAutoIndent(), GetPrefAutoSave(), GetPrefFontName(),
    	    GetPrefWrap(), GetPrefTabDist());
    strcpy(window->filename, name);
    strcpy(window->path, "");
    window->filenameSet = FALSE;
    SetWindowModified(window, FALSE);
    window->readOnly = FALSE;
}   

/*
** Open an existing file specified by name and path.  Use the window inWindow
** unless inWindow is NULL or points to a window which is already in use
** (displays a file other than Untitled, or is Untitled but modified).  If
** create is true, prompt the user to create the file if it doesn't exist.
*/
void EditExistingFile(WindowInfo *inWindow, char *name, char *path, int create)
{
    WindowInfo *window;
    
    /* first look to see if file is already displayed in a window */
    window = FindWindowWithFile(name, path);
    if (window != NULL) {
    	XMapRaised(TheDisplay, XtWindow(window->shell));
	return;
    }
    
    /* If an existing window isn't specified, or the window is already
       in use (not Untitled or Untitled and modified), create the window */
    if (inWindow == NULL || inWindow->filenameSet || inWindow->fileChanged) {
	window = CreateWindow(name, GetPrefRows(), GetPrefCols(),
    		GetPrefAutoIndent(), GetPrefAutoSave(), GetPrefFontName(),
    		GetPrefWrap(), GetPrefTabDist());
    } else {
    	window = inWindow;
	SetWindowTitle(window, name);
    }
    	
    /* Open the file */
    if (!doOpen(window, name, path, create))
    	CloseWindow(window);
}

void RevertToSaved(WindowInfo *window)
{
    char name[MAXPATHLEN], path[MAXPATHLEN];
    int b;
    
    /* Can't revert untitled windows */
    if (!window->filenameSet) {
    	DialogF(DF_WARN, window->shell, 1,
    		"Window was never saved, can't revert", "OK");
    	return;
    }
    
    /* re-reading file is irreversible, prompt the user first */
    if (window->fileChanged)
	b = DialogF(DF_QUES, window->shell, 2, "Discard changes to\n%s%s?",
    		    "OK", "Cancel", window->path, window->filename);
    else
	b = DialogF(DF_QUES, window->shell, 2, "No changes made to\n%s%s",
    		    "Re-read File", "Cancel", window->path, window->filename);
    if (b != 1)
    	return;

    /* re-read the file, update the window title if new file is different */
    strcpy(name, window->filename);
    strcpy(path, window->path);
    removeBackupFile(window);
    ClearUndoList(window);
    if (!doOpen(window, name, path, FALSE)) {
    	CloseWindow(window);
    	return;
    }
    UpdateWindowTitle(window);
}

static int doOpen(WindowInfo *window, char *name, char *path, int create)
{
    char fullname[MAXPATHLEN];
#ifdef VMS
    char tempname[MAXPATHLEN];
    int canWrite;
#endif
    struct stat statbuf;
    int fileLen, readLen;
    char *fileString;
    FILE *fp = NULL;
    int fd;
    int readOnly = FALSE;
    int resp;

    strcpy(window->filename, name);
    strcpy(window->path, path);
    window->filenameSet = TRUE;

    /* Get the full name of the file */
    strcpy(fullname, path);
    strcat(fullname, name);
    
#ifdef VMS
    /* Test if file can be written by removing version number from the file
       and opening it for write.  Close and remove the file if open worked. */
    removeVersionNumber(fullname, tempname);
    fp = fopen(tempname, "w");
    canWrite = (fp != NULL);
    if (canWrite) {
    	fgetname(fp, tempname);
    	fclose(fp);
    	remove(tempname);
    }
#endif /*VMS*/

    /* Open the file */
#ifdef VMS
    fp = fopen(fullname, "r");
    if (fp == NULL || !canWrite) {
    	/* Error opening file or file is not writeable */
    	if (fp != NULL) {
#else
    if ((fp = fopen(fullname, "r+")) == NULL) {
    	/* Error opening file or file is not writeable */
	if ((fp = fopen(fullname, "r")) != NULL) {
#endif /*VMS*/
	    /* File is read only */
	    readOnly = TRUE;
	} else if (create && errno == ENOENT) {
	    /* Give option to create (or to exit if this is the only window) */
	    if (WindowList == window && window->next == NULL)
	    	resp = DialogF(DF_WARN, window->shell, 3, "Can't open %s:\n%s",
	    	   "Create", "Cancel", "Exit NEdit", fullname, errorString());
	    else
	    	resp = DialogF(DF_WARN, window->shell, 2, "Can't open %s:\n%s",
	    		"Create", "Cancel", fullname, errorString());
	    if (resp == 2)
	    	return FALSE;
	    else if (resp == 3)
	    	exit(0);
	    /* Test if new file can be created */
	    if ((fd = creat(fullname, 0666)) == -1) {
    		DialogF(DF_ERR, window->shell, 1, "Can't create %s:\n%s",
    			"OK", fullname, errorString());
        	return FALSE;
	    } else {
#ifdef VMS
		/* get correct version number and make sure file is closed */
		getname(fd, fullname);
		close(fd);
#endif
	        remove(fullname);
	    }
	    SetWindowModified(window, FALSE);
	    window->readOnly = FALSE;
	    return TRUE;
	} else {
	    /* A true error */
	    DialogF(DF_ERR, window->shell, 1, "Could not open %s%s:\n%s",
	    	    "OK", path, name, errorString());
	    return FALSE;
	}
    }
    
    /* Get the length of the file and the protection mode */
    if (fstat(fileno(fp), &statbuf) != 0) {
	DialogF(DF_ERR, window->shell, 1, "Error opening %s", "OK", name);
	return FALSE;
    }
    fileLen = statbuf.st_size;
    window->fileMode = statbuf.st_mode;
 
    /* allocate space for the whole contents of the file (unfortunately) */
    fileString = (char *)malloc(fileLen+1);  /* +1 = space for null */
    if (fileString == NULL) {
	DialogF(DF_ERR, window->shell, 1, "File is too large to edit", "OK");
	return FALSE;
    }

    /* read the file into fileString and terminate with a null */
    readLen = fread(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "Error reading %s:\n%s", "OK",
    		name, errorString());
	free(fileString);
	return FALSE;
    }
    fileString[readLen] = 0;
 
    /* close the file */
    if (fclose(fp) != 0) {
    	/* unlikely error */
	DialogF(DF_WARN, window->shell, 1, "Unable to close file", "OK");
	/* we read it successfully, so continue */
    }
    
    /* display the file contents in the text widget */
    window->ignoreModify = True;
    XmTextSetString(window->textArea, fileString);
    window->ignoreModify = False;

    /* release the memory that holds fileString */
    free(fileString);

    /* Set window title and file changed flag */
    /* This goes after adding text to widget because modifiedCB is activated */
    if (readOnly) {
	window->readOnly = TRUE;
	window->fileChanged = FALSE;
	UpdateWindowTitle(window);
    } else {
	window->readOnly = FALSE;
	SetWindowModified(window, FALSE);
    }

    return TRUE;
}   

int IncludeFile(WindowInfo *window, char *name)
{
    struct stat statbuf;
    int fileLen, readLen;
    char *fileString;
    FILE *fp = NULL;
    XmTextPosition left, right;

    /* Open the file */
    fp = fopen(name, "r");
    if (fp == NULL) {
	DialogF(DF_ERR, window->shell, 1, "Could not open %s:\n%s",
	    	"OK", name, errorString());
	return FALSE;
    }
    
    /* Get the length of the file */
    if (fstat(fileno(fp), &statbuf) != 0) {
	DialogF(DF_ERR, window->shell, 1, "Error openinig %s", "OK", name);
	return FALSE;
    }
    fileLen = statbuf.st_size;
 
    /* allocate space for the whole contents of the file */
    fileString = (char *)malloc(fileLen+1);  /* +1 = space for null */
    if (fileString == NULL) {
	DialogF(DF_ERR, window->shell, 1, "File is too large to include", "OK");
	return FALSE;
    }

    /* read the file into fileString and terminate with a null */
    readLen = fread(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "Error reading %s:\n%s", "OK",
    		name, errorString());
	fclose(fp);
	free(fileString);
	return FALSE;
    }
    fileString[readLen] = 0;
 
    /* close the file */
    if (fclose(fp) != 0) {
    	/* unlikely error */
	DialogF(DF_WARN, window->shell, 1, "Unable to close file", "OK");
	/* we read it successfully, so continue */
    }
    
    /* insert the contents of the file in the selection or at the insert
       position in the window if no selection exists */
    if (!XmTextGetSelectionPosition(window->textArea, &left, &right))
    	left = right = XmTextGetInsertionPosition(window->lastFocus);
    XmTextReplace(window->textArea, left, right, fileString);

    /* release the memory that holds fileString */
    free(fileString);

    return TRUE;
}

/*
** Close all files and windows, leaving one untitled window
*/
int CloseAllFilesAndWindows()
{
    while (WindowList->next != NULL || 
    		WindowList->filenameSet || WindowList->fileChanged) {
    	if (!CloseFileAndWindow(WindowList))
    	    return FALSE;
    }
    return TRUE;
}

int CloseFileAndWindow(WindowInfo *window)
{ 
    int response, stat;
    
    /* Make sure that the window is not in iconified state */
    XMapRaised(TheDisplay, XtWindow(window->shell));

    /* if window is modified, ask about saving, otherwise just close */
    if (!window->fileChanged) {
       CloseWindow(window);
       /* up-to-date windows don't have outstanding backup files to close */
    } else {
	response = DialogF(DF_WARN, window->shell, 3,
		"Save %s before closing?", "Yes", "No", "Cancel",
		window->filename);
	if (response == 1) {
	    /* Save */
	    stat = SaveWindow(window);
	    if (stat)
	    	CloseWindow(window);
	} else if (response == 2) {
	    /* Don't Save */
	    removeBackupFile(window);
	    CloseWindow(window);
	} else /* 3 == Cancel */
	    return FALSE;
    }
    return TRUE;
}

int SaveWindow(WindowInfo *window)
{
    int stat;
    
    if (!window->fileChanged)
    	return TRUE;
    if (!window->filenameSet)
    	return SaveWindowAs(window);
#ifdef VMS
    removeBackupFile(window);
    stat = doSave(window);
#else
    stat = doSave(window);
    removeBackupFile(window);
#endif /*VMS*/
    return stat;
}
    
int SaveWindowAs(WindowInfo *window)
{
    int response;
    char fullname[MAXPATHLEN], filename[MAXPATHLEN], pathname[MAXPATHLEN];
    WindowInfo *otherWindow;
    
    response = GetNewFilename(window->shell, "Save File As:", fullname);
    if (response != GFN_OK)
    	return FALSE;
    
    ParseFilename(fullname, filename, pathname);
    otherWindow = FindWindowWithFile(filename, pathname);
    if (otherWindow != NULL) {
	response = DialogF(DF_WARN, window->shell, 2,
		"%s is open in another NEdit window", "Cancel",
		"Close Other Window", filename);
	if (response == 1)
	    return FALSE;
	if (!CloseFileAndWindow(otherWindow))
	    return FALSE;
    }
    removeBackupFile(window);
    strcpy(window->filename, filename);
    strcpy(window->path, pathname);
    window->filenameSet = TRUE;
    SetWindowTitle(window, filename);
    /* XmTextSetEditable(window->textArea, TRUE); */
    window->readOnly = FALSE;
    return doSave(window);
}

static int doSave(WindowInfo *window)
{
    char *fileString = NULL;
    char fullname[MAXPATHLEN];
    char tempname[MAXPATHLEN];
#ifndef VMS
    char file[MAXPATHLEN];
    char path[MAXPATHLEN];
#endif
    FILE *tfp;
    int fileLen;

    /*
    ** Notes on doSave under Unix:  If possible, files are written first to a
    ** temporary file in the same directory, then renamed to the name of the
    ** file to be replaced.  We can't use the tmpnam or tempnam subroutines to
    ** generate temporary file names, because they prefer to write in the
    ** system temporary directory, which may be on another volume.  Even
    ** tempnam, which allows you to specify a directory, can be overridden by
    ** the  TEMPDIR environment variable.  Putting the temp file in the same
    ** directory, on the same filesystem, and renaming, rather
    ** than copying the temp file into the real file, draws out more
    ** of the potential problems, such as file system full, or protection
    ** problems than putting it in some random temp directory.
    **
    ** The temporary file name is generated by adding a tilde to the beginning
    ** of the filename.  If the user happens to have a file by this name, it
    ** is wiped out!  NEdit users should be mostly prepared for this, since
    ** the automatic backup feature saves to a similarly named file, but
    ** I would still like to know a better way to do this.
    **
    ** Under VMS: No temporary files are used.  The version number is
    ** stripped from the name of the file being edited before the new file
    ** is created.  VMS chooses a new version number for the file, and NEdit
    ** re-loads the name and changes it in the window title.
    */
    
    /* Get the full name of the file */
    strcpy(fullname, window->path);
    strcat(fullname, window->filename);

#ifdef VMS
    /* strip the version number from the file so VMS will begin a new one */
    removeVersionNumber(fullname, tempname);

    /* open the file */
    if ((tfp = fopen(tempname, "w")) == NULL) {
    	DialogF(DF_WARN, window->shell, 1, "Unable to save %s:\n%s", "OK",
		window->filename, errorString());
        return FALSE;
    }

    /* get the complete name of the file including the new version number */
    fgetname(tfp, tempname);
        
    /* set the protection for the file */
    if (window->fileMode)
    	chmod(tempname, window->fileMode);

#else /* Unix */
    /* If the file is actually a symbolic link, get the real name */
    if (!followSymLinks(fullname)) {
    	DialogF(DF_ERR, window->shell, 1, "Error saving %s:\n%s", "OK",
		window->filename, errorString());
	return FALSE;
    }
    ParseFilename(fullname, file, path);

    /* Generate a temporary file name by prefixing with ~ (see note above) */
    sprintf(tempname, "%s~%s", path, window->filename);

    /* open the file */
    tfp = fopen(tempname, "w");
    
    /* if we don't have write access to temporary file, risk using real one */
    if (tfp == NULL && errno == EACCES) {
    	strcpy(tempname, fullname);
    	tfp = fopen(tempname, "w");
    }
    
    /* check that open has succeeded and present error if it hasn't */
    if (tfp == NULL) {    
    	DialogF(DF_WARN, window->shell, 1, "Unable to save %s:\n%s", "OK",
		window->filename, errorString());
        return FALSE;
    }
    
    /* set the protection for the file */
    if (window->fileMode)
    	fchmod(fileno(tfp), window->fileMode);

#endif /*VMS/Unix*/

    /* get the text buffer and its length from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1, "%s is too large to save", "OK", 
		window->filename);
	fclose(tfp);
	remove(tempname);
	return FALSE;
    }
    fileLen = strlen(fileString);
    
    /* add a terminating newline if the file doesn't already have one */
    if (fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* write to the file */
    fwrite(fileString, sizeof(char), fileLen, tfp);
    if (ferror(tfp)) {
    	DialogF(DF_ERR, window->shell, 1, "%s not saved:\n%s", "OK", 
		window->filename, errorString());
	fclose(tfp);
	remove(tempname);
        XtFree(fileString);
	return FALSE;
    }
    
    /* close the file */
    if (fclose(tfp) != 0) {
    	DialogF(DF_ERR, window->shell,1,"Error closing file:\n%s", "OK",
		errorString());
        XtFree(fileString);
	return FALSE;
    }

    /* free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
    
#ifndef VMS
    /* move the temporary output file to replace the real file, unless we
       were forced to use the real file in the first place */
    if (strcmp(tempname, fullname)) {
	if (rename(tempname, fullname) != 0) {
    	    DialogF(DF_ERR, window->shell, 1,
    		    "Error writing %s,\ncontents saved in %s",
		    "OK", window->filename, tempname);
	    fclose(tfp);
            return FALSE;
	}
    }
#endif /*VMS*/
    
#ifdef VMS
    /* reflect the fact that NEdit is now editing a new version of the file */
    ParseFilename(tempname, window->filename, window->path);
#endif /*VMS*/

    /* success, file was written */
    SetWindowModified(window, FALSE);
    return TRUE;
}

/*
** Create a backup file for the current window.  The name for the backup file
** is generated using the name and path stored in the window and adding a
** tilde (~) on UNIX and underscore (_) on VMS to the beginning of the name.  
** Note that in the case of linked (UNIX)
** files, this is not necessarily the same as the temporary filename used
** by the doSave routine when it saves the real file.
*/
int WriteBackupFile(WindowInfo *window)
{
    char *fileString = NULL;
    char name[MAXPATHLEN];
    FILE *fp;
    int fileLen;
    
    /* Generate a name for the autoSave file */
#ifdef VMS
    sprintf(name, "%s\_%s", window->path, window->filename);
#else
    sprintf(name, "%s~%s", window->path, window->filename);
#endif /*VMS*/

#ifdef VMS
    /* remove the old backup file because we reuse the same version number */
    remove(name);
#endif /*VMS*/
    
    /* open the file */
    if ((fp = fopen(name, "w")) == NULL) {
    	DialogF(DF_WARN, window->shell, 1,
    	       "Unable to save backup for %s:\n%s\nAutomatic backup is now off",
    	       "OK", window->filename, errorString());
        window->autoSave = FALSE;
        return FALSE;
    }

    /* get the text buffer and its length from the text area widget */
    fileString = XmTextGetString(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1, "%s is too large to save!", "OK", 
		window->filename);
	fclose(fp);
	remove(name);
	return FALSE;
    }
    fileLen = strlen(fileString);
    
    /* add a terminating newline if the file doesn't already have one */
    if (fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* write out the file */
    fwrite(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1,
    	   "Error while saving backup for %s:\n%s\nAutomatic backup is now off",
    	   "OK", window->filename, errorString());
	fclose(fp);
	remove(name);
        XtFree(fileString);
        window->autoSave = FALSE;
	return FALSE;
    }
    
    /* close the backup file */
    if (fclose(fp) != 0) {
	XtFree(fileString);
	return FALSE;
    }

    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);

    return TRUE;
}

static int removeBackupFile(WindowInfo *window)
{
    char name[MAXPATHLEN];
    
    /* Generate the name from filename and path in window and remove the file */
#ifdef VMS
    sprintf(name, "%s\_%s", window->path, window->filename);
#else
    sprintf(name, "%s~%s", window->path, window->filename);
#endif /*VMS*/
    remove(name);
}

int PrintWindow(WindowInfo *window, int selectedOnly)
{
    char *fileString = NULL;
    char tmpFileName[L_tmpnam];    /* L_tmpnam defined in stdio.h */
    FILE *fp;
    int fileLen;

    /* Generate a temporary file name */
    tmpnam(tmpFileName);

    /* open the temporary file */
    if ((fp = fopen(tmpFileName, "w")) == NULL) {
    	DialogF(DF_WARN, window->shell, 1,
	    "Unable to write file for printing:\n%s", "OK", errorString());
        return FALSE;
    }
    
    /* get the text buffer from the text area widget */
    if (selectedOnly) {
    	fileString = XmTextGetSelection(window->textArea);
    	if (fileString == NULL || *fileString == '\0') {
    	    XBell(TheDisplay, 100);
    	    fclose(fp);
    	    remove(tmpFileName);
	    return FALSE;
	}
    } else {
    	fileString = XmTextGetString(window->textArea);
	if (fileString == NULL) {
            DialogF(DF_ERR, window->shell, 1,
		  "Out of memory, you may be\nunable to save your file!", "OK");
	    fclose(fp);
    	    remove(tmpFileName);
	    return FALSE;
	}
    }
    fileLen = strlen(fileString);
    
    /* add a terminating newline if the file doesn't already have one */
    if (fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* write to the file */
    fwrite(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "%s not printed:\n%s", "OK", 
		window->filename, errorString());
	fclose(fp);
    	remove(tmpFileName);
        XtFree(fileString);
	return FALSE;
    }
    
    /* close the temporary file */
    if (fclose(fp) != 0) {
    	DialogF(DF_ERR,window->shell,1,"Error closing temp. print file:\n%s",
		"OK", errorString());
        XtFree(fileString);
    	remove(tmpFileName);
	return FALSE;
    }

    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);

    /* Print the temporary file, then delete it and return success */
#ifdef VMS
    strcat(tmpFileName, ".");
    PrintFile(window->shell, tmpFileName, window->filename, True);
#else
    PrintFile(window->shell, tmpFileName, window->filename);
    remove(tmpFileName);
#endif /*VMS*/
    return TRUE;
}

#ifndef VMS
/*
** Follow symbolic links (if any) to translate filename into the name of the
** real file that it represents.  Returns TRUE if the call was successful,
** meaning the links were translated successfully, or the file was not 
** linked to begin with (or there was no file).  Returns false if some
** error prevented the call from determining if there were symbolic links
** to process, or there was an error in processing them.  The error
** can be read from the unix global variable errno.
*/
static int followSymLinks(char *filename)
{
    int cc;
    char buf[MAXPATHLEN];
    
    cc = readlink(filename, buf, MAXPATHLEN);
    if (cc == -1) {
#ifdef SGI
    	if (errno == EINVAL || errno == ENOENT || errno == ENXIO)
#else
    	if (errno == EINVAL || errno == ENOENT)
#endif
    	    return TRUE;   /* no error, just not a symbolic link, or no file */
    	else
    	    return FALSE;
    } else {
    	buf[cc] = '\0';
    	strcpy(filename, buf);
    	return TRUE;
    }
}   
#endif /*VMS*/

/*
** Wrapper for strerror so all the calls don't have to be ifdef'd for VMS.
*/
static char *errorString(void)
{
#ifdef VMS
    return strerror(errno, vaxc$errno);
#else
    return strerror(errno);
#endif
}

#ifdef VMS
/*
** Copies a string containing a file name (fromName) to another string
** (toName), removing the VMS version number if the name has one.  toName
** must point to a buffer large enough to store the new name.
*/
void removeVersionNumber(char *fromName, char *toName)
{
    char *versionStart;
    
    strcpy(toName, fromName);
    versionStart = strrchr(toName, ';');
    if (versionStart != NULL)
    	*versionStart = '\0';
}
#endif /*VMS*/

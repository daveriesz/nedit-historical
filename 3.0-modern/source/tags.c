static char SCCSID[] = "@(#)tags.c	1.4     3/14/94";
#include <stdio.h>
#include <stdlib.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "search.h"
#include "clipboard.h"
#include "tags.h"

#define MAXLINE 512
#define MAX_TAG_LEN 80

enum searchDirection {FORWARD, BACKWARD};

typedef struct {
    char *name;
    char *file;
    char *searchString;
} tag;

static void setTag(tag *t, char *name, char *file, char *searchString);
static void freeTagList(tag *tags);
static int fakeRegExSearch(WindowInfo *window, char *searchString, 
	int *start, int *end);

/* Parsed list of tags read by LoadTagsFile.  List is terminated by a tag
   structure with the name field == NULL */
static tag *Tags = NULL;
static char TagPath[MAXPATHLEN];

int LoadTagsFile(char *filename)
{
    FILE *fp = NULL;
    char line[MAXLINE], name[MAXLINE], file[MAXLINE], searchString[MAXLINE];
    char unused[MAXPATHLEN];
    char *charErr;
    tag *tags;
    int i, nTags, nRead;
    WindowInfo *w;

    /* Open the file */
    if ((fp = fopen(filename, "r")) == NULL)
    	return FALSE;
	
    /* Read it once to see how many lines there are */
    for (nTags=0; TRUE; nTags++) {
    	charErr = fgets(line, MAXLINE, fp);
    	if (charErr == NULL) {
    	    if (feof(fp))
    	    	break;
    	    else
    	    	return FALSE;
    	}
    }
    
    /* Allocate zeroed memory for list so that it is automatically terminated
       and can be freed by freeTagList at any stage in its construction*/
    tags = (tag *)calloc(nTags + 1, sizeof(tag));
    
    /* Read the file and store its contents */
    rewind(fp);
    for (i=0; i<nTags; i++) {
    	charErr = fgets(line, MAXLINE, fp);
    	if (charErr == NULL) {
    	    if (feof(fp))
    	    	break;
    	    else {
    	    	freeTagList(tags);
    	    	return FALSE;
    	    }
    	}
    	nRead = sscanf(line, "%s\t%s\t%[^\n]", name, file, searchString);
    	if (nRead != 3) {
    	    freeTagList(tags);
    	    return FALSE;
    	}
	setTag(&tags[i], name, file, searchString);
    }
    
    /* Make sure everything was read */
    if (i != nTags) {
    	freeTagList(tags);
    	return FALSE;
    }
    
    /* Replace current tags data and path for retrieving files */
    if (Tags != NULL)
    	freeTagList(Tags);
    Tags = tags;
    ParseFilename(filename, unused, TagPath);
    
    /* Undim the "Find Definition" menu item in the existing windows */
    for (w=WindowList; w!=NULL; w=w->next)
    	XtSetSensitive(w->findDefItem, TRUE);
    return TRUE;
}

int TagsFileLoaded()
{
    return Tags != NULL;
}

/*
** Given a name, lookup a file, search string.  Returned strings are pointers
** to internal storage which are valid until the next LoadTagsFile call.
*/
int LookupTag(char *name, char **file, char **searchString)
{
    int i;
    tag *t;
    
    for (i=0, t=Tags; t->name!=NULL; i++, t++) {
 	if (!strcmp(t->name, name)) {
 	    *file = t->file;
 	    *searchString = t->searchString;
 	    return TRUE;
	}
    }
    return FALSE;
}

int FindDefinition(WindowInfo *window, Time time)
{
    int selStart, selEnd, startPos, endPos, found, lineNum;
    char *string, *fileToSearch, *searchString, *eptr;
    WindowInfo *windowToSearch;
    char filename[MAXPATHLEN], pathname[MAXPATHLEN], temp[MAXPATHLEN];

    /* get the name to match from the text widget selection */
    if (!GetSelection(window->textArea, &selStart, &selEnd)) {
	XBell(TheDisplay, 100);
	return FALSE;
    }
    if ((selEnd - selStart) > MAX_TAG_LEN || (selEnd - selStart) == 0) {
    	XBell(TheDisplay, 100);
	return FALSE;
    }
    string = GetTextRange(window->textArea, selStart, selEnd);
    if (string == NULL) {
    	XBell(TheDisplay, 100);
	return FALSE;
    }
    
    /* lookup the name in the tags file */
    found = LookupTag(string, &fileToSearch, &searchString);
    if (!found) {
    	DialogF(DF_WARN, window->shell, 1, "%s not found in tags file", "OK",
    		string);
    	XtFree(string);
    	return FALSE;
    }
    
    /* if the path is not absolute, qualify file path with directory
       from which tags file was loaded */
    if (fileToSearch[0] == '/')
    	strcpy(temp, fileToSearch);
    else {
    	strcpy(temp, TagPath);
    	strcat(temp, fileToSearch);
    }
    ParseFilename(temp, filename, pathname);
    
    /* open the file containing the definition */
    EditExistingFile(WindowList, filename, pathname, FALSE);
    windowToSearch = FindWindowWithFile(filename, pathname);
    if (windowToSearch == NULL) {
    	DialogF(DF_WARN, window->shell, 1, "File %s not found", 
    		"OK", fileToSearch);
    	XtFree(string);
    	return FALSE;
    }
    
    /* if the search string is a number, select the numbered line */
    lineNum = strtol(searchString, &eptr, 10);
    if (eptr != searchString) {
    	SelectNumberedLine(windowToSearch, lineNum, time);
    	XtFree(string);
    	return TRUE;
    }
    
    /* search for the tags file search string in the newly opened file */
    found = fakeRegExSearch(windowToSearch, searchString ,&startPos, &endPos);
    if (!found) {
    	DialogF(DF_WARN, windowToSearch->shell, 1,"Definition for %s\nnot found in %s", 
    		"OK", string, fileToSearch);
    	XtFree(string);
    	return FALSE;
    }
    XtFree(string);

    /* select the matched string */
    XmTextSetSelection(windowToSearch->lastFocus, startPos, endPos, time);
    return TRUE;
}

static void setTag(tag *t, char *name, char *file, char *searchString)
{
    t->name = (char *)malloc(sizeof(char) * strlen(name) + 1);
    strcpy(t->name, name);
    t->file = (char *)malloc(sizeof(char) * strlen(file) + 1);
    strcpy(t->file, file);
    t->searchString = (char *)malloc(sizeof(char) * strlen(searchString) + 1);
    strcpy(t->searchString, searchString);
}

static void freeTagList(tag *tags)
{
    int i;
    tag *t;
    
    for (i=0, t=tags; t->name!=NULL; i++, t++) {
    	free(t->name);
    	free(t->file);
    	free(t->searchString);
    }
    free(tags);
}

/*
** regex searching is not available on all platforms.  To use built in
** case sensitive searching, this routine fakes enough to handle the
** search characters presented in ctags files
*/
static int fakeRegExSearch(WindowInfo *window, char *searchString, 
	int *start, int *end)
{
    int startPos, endPos, found=FALSE, hasBOL, hasEOL, fileLen, searchLen, dir;
    char *fileString, searchSubs[MAXLINE];
    
    /* get the entire (sigh) text buffer from the text area widget */
    fileString = XmTextGetString(window->textArea);
    fileLen = XmTextGetLastPosition(window->textArea);
    if (fileString == NULL) {
        DialogF(DF_ERR, window->shell, 1,
		"Out of memory!\nTry closing some windows.\nSave your files!",
		"OK");
	return FALSE;
    }

    /* remove / .. / or ? .. ? and substitute ^ and $ with \n */
    searchLen = strlen(searchString);
    if (searchString[0] == '/')
    	dir = FORWARD;
    else if (searchString[0] == '?')
    	dir = BACKWARD;
    else {
    	fprintf(stderr, "NEdit: Error parsing tag file search string");
    	return FALSE;
    }
    searchLen -= 2;
    strncpy(searchSubs, &searchString[1], searchLen);
    searchSubs[searchLen] = '\0';
    hasBOL = searchSubs[0] == '^';
    hasEOL = searchSubs[searchLen-1] == '$';
    if (hasBOL) searchSubs[0] = '\n';
    if (hasEOL) searchSubs[searchLen-1] = '\n';

    /* search for newline-substituted string in the file */
    if (dir==FORWARD)
    	found = SearchString(fileString, searchSubs, SEARCH_FORWARD,
    		SEARCH_CASE_SENSE, False, 0, &startPos, &endPos);
    else
    	found = SearchString(fileString, searchSubs, SEARCH_BACKWARD,
    		SEARCH_CASE_SENSE, False, fileLen, &startPos, &endPos);
    if (found) {
    	if (hasBOL) startPos++;
    	if (hasEOL) endPos--;
    }
    
    /* if not found: ^ may match beginning of file, $ may match end */
    if (!found && hasBOL) {
    	found = strncmp(&searchSubs[1], fileString, searchLen-1);
    	if (found) {
    	    startPos = 0;
    	    endPos = searchLen - 2;
    	}
    }
    if (!found && hasEOL) {	    
    	found = strncmp(searchSubs, fileString+fileLen-searchLen+1,
    		 searchLen-1);
    	if (found) {
    	    startPos = fileLen-searchLen+2;
    	    endPos = fileLen;
    	}
    }

    /* free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
    
    /* return the result */
    if (!found) {
    	XBell(TheDisplay, 100);
	return FALSE;
    }
    *start = startPos;
    *end = endPos;
    return TRUE;
}

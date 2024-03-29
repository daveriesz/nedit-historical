/* $Id: prefFile.h,v 1.6 2002/07/11 21:18:09 slobasso Exp $ */

#ifndef NEDIT_PREFFILE_H_INCLUDED
#define NEDIT_PREFFILE_H_INCLUDED

#include <X11/Intrinsic.h>

enum PrefDataTypes {PREF_INT, PREF_BOOLEAN, PREF_ENUM, PREF_STRING,
	PREF_ALLOC_STRING};

typedef struct _PrefDescripRec {
    char *name;
    char *class;
    int dataType;
    char *defaultString;
    void *valueAddr;
    void *arg;
    int save;
} PrefDescripRec;

XrmDatabase CreatePreferencesDatabase(const char *fileName,
         const char *appName, XrmOptionDescList opTable,
         int nOptions, unsigned int *argcInOut, char **argvInOut);
void RestorePreferences(XrmDatabase prefDB, XrmDatabase appDB,
	const char *appName, const char *appClass, PrefDescripRec *rsrcDescrip, int nRsrc);
void OverlayPreferences(XrmDatabase prefDB, const char *appName,
        const char *appClass, PrefDescripRec *rsrcDescrip, int nRsrc);
void RestoreDefaultPreferences(PrefDescripRec *rsrcDescrip, int nRsrc);
int SavePreferences(Display *display, const char *fileName,
        const  char *fileHeader, PrefDescripRec *rsrcDescrip, int nRsrc);

#endif /* NEDIT_PREFFILE_H_INCLUDED */

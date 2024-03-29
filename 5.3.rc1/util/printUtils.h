/* $Id: printUtils.h,v 1.5 2001/11/22 21:01:05 amai Exp $ */
/* Maximum length of an error returned by IssuePrintCommand() */
#define MAX_PRINT_ERROR_LENGTH 1024

#define DESTINATION_REMOTE 1
#define DESTINATION_LOCAL  2

void LoadPrintPreferences(XrmDatabase prefDB, const char *appName,
     const char *appClass, int lookForFlpr);

#ifdef VMS
void PrintFile(Widget parent, const char *PrintFileName, const char *jobName, int delete);
#else
void PrintFile(Widget parent, const char *PrintFileName, const char *jobName);
#endif /*VMS*/

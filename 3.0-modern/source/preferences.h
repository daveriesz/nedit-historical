/*******************************************************************************
*									       *
* preferences.h -- Nirvana Editor preferences processing		       *
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
* April 20, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
/* SCCS ID: preferences.h 1.6 2/4/94 */
XrmDatabase CreateNEditPrefDB(int *argcInOut, char **argvInOut);
void RestoreNEditPrefs(XrmDatabase prefDB, XrmDatabase appDB);
void SaveNEditPrefs(Widget parent);
void SetPrefWrap(int state);
int GetPrefWrap(void);
void SetPrefSearchDlogs(int state);
int GetPrefSearchDlogs(void);
void SetPrefStatsLine(int state);
int GetPrefStatsLine();
void SetPrefSearch(int searchType);
int GetPrefSearch(void);
void SetPrefAutoIndent(int state);
int GetPrefAutoIndent(void);
void SetPrefAutoSave(int state);
int GetPrefAutoSave(void);
void SetPrefRows(int nRows);
int GetPrefRows(void);
void SetPrefCols(int nCols);
int GetPrefCols(void);
void SetPrefTabDist(int tabDist);
int GetPrefTabDist();
void SetPrefTagFile(char *tagFileName);
char *GetPrefTagFile();
void SetPrefFont(char *font);
char *GetPrefFontName();
void SetPrefShell(char *shell);
char *GetPrefShell();
void RowColumnPrefDialog(Widget parent);
void SetPrefMapDelete(int state);
int GetPrefMapDelete(void);

/*******************************************************************************
*									       *
* window.h -- Nirvana Editor window creation/deletion			       *
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
/* SCCS ID: window.h 1.10 8/25/94 */
WindowInfo *CreateWindow(char *title, int rows, int cols, int autoIndent, 
	int autoSave, int saveOldVersion, char *fontName, XmFontList fontList,
	int wrap, int overstrike, int tabDist, int showMatching);
void CloseWindow(WindowInfo *window);
int NWindows();
void SetWindowTitle(WindowInfo *window, char *title);
void UpdateWindowTitle(WindowInfo *window);
void UpdateWindowReadOnly(WindowInfo *window);
void SetWindowModified(WindowInfo *window, int modified);
void MakeSelectionVisible(WindowInfo *window, Widget textPane);
int GetSelection(Widget widget, int *left, int *right);
int GetSimpleSelection(textBuffer *buf, int *left, int *right);
char *GetTextRange(Widget widget, int left, int right);
WindowInfo *FindWindowWithFile(char *name, char *path);
void SetAutoIndent(WindowInfo *window, int state);
void SetFontByName(WindowInfo *window, char *fontName);
void SetOverstrike(WindowInfo *window, int overstrike);
void SetAutoWrap(WindowInfo *window, int state);
void SetWrapMargin(WindowInfo *window, int margin);
void SplitWindow(WindowInfo *window);
void ClosePane(WindowInfo *window);
void ShowStatsLine(WindowInfo *window, int state);
void SetModeMessage(WindowInfo *window, char *message);
void ClearModeMessage(WindowInfo *window);
WindowInfo *WidgetToWindow(Widget w);

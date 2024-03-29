/*******************************************************************************
*									       *
* userCmds.h -- Nirvana Editor shell and macro command dialogs 		       *
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
* November, 1995							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
/* SCCS ID: shell.h 1.1 2/4/94 */
void EditShellMenu(WindowInfo *window);
void EditMacroMenu(WindowInfo *window);
void UpdateShellMenu(WindowInfo *window);
void UpdateMacroMenu(WindowInfo *window);
char *WriteShellCmdsString(void);
char *WriteMacroCmdsString(void);
int LoadShellCmdsString(char *inString);
int LoadMacroCmdsString(char *inString);
int DoNamedShellMenuCmd(WindowInfo *window, char *itemName);
int DoNamedMacroMenuCmd(WindowInfo *window, char *itemName);

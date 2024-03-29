/*******************************************************************************
*									       *
* clipboard.h -- Nirvana Editor clipboard interface			       *
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
/* SCCS ID: clipboard.h 1.3 2/4/94 */
void CutToClipboard(WindowInfo *window, Time time);
void CopyToClipboard(WindowInfo *window, Time time);
void PasteFromClipboard(WindowInfo *window, Time time);
void GotoSelectedLineNumber(WindowInfo *window, Time time);
void GotoLineNumber(WindowInfo *window, Time time);
void SelectNumberedLine(WindowInfo *window, int lineNum, Time time);
void OpenSelectedFile(WindowInfo *window, Time time);


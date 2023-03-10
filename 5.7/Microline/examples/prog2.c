/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Microline Widget Library, originally made available under the NPL by Neuron Data <http://www.neurondata.com>.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * In addition, as a special exception to the GNU GPL, the copyright holders
 * give permission to link the code of this program with the Motif and Open
 * Motif libraries (or with modified versions of these that use the same
 * license), and distribute linked combinations including the two. You
 * must obey the GNU General Public License in all respects for all of
 * the code used other than linking with Motif/Open Motif. If you modify
 * this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version.
 *
 * ***** END LICENSE BLOCK ***** */


#include <Xm/Xm.h>
#include <XmL/Progress.h>

Boolean compute();

Widget progress;

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell;

	shell =  XtAppInitialize(&app, "Prog2", NULL, 0,
		&argc, argv, NULL, NULL, 0);

	progress = XtVaCreateManagedWidget("progress",
		xmlProgressWidgetClass, shell,
		XmNshowTime, True,
		XtVaTypedArg, XmNbackground, XmRString, "white", 6,
		XtVaTypedArg, XmNforeground, XmRString, "#800000", 8,
		XmNwidth, 300,
		XmNheight, 25,
		NULL);

	XtAppAddWorkProc(app, compute, NULL);
	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

Boolean compute(clientData)
XtPointer clientData;
{
	int i;

	XtVaSetValues(progress,
		XmNvalue, 0,
		XmNcompleteValue, 7,
		NULL);
	for (i = 0; i < 7; i++)
	{
		XtVaSetValues(progress,
			XmNvalue, i,
			NULL);

		/* perform processing */
		sleep(1);
	}
	XtVaSetValues(progress,
		XmNvalue, i,
		NULL);
	return(FALSE);
}

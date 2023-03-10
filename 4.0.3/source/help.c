/*******************************************************************************
*									       *
* help.c -- Nirvana Editor help display					       *
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
* their employees, makes any warranty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* September 10, 1991							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
static char SCCSID[] = "@(#)help.c	1.28     10/31/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PushBG.h>
#include "../util/misc.h"
#include "textBuf.h"
#include "nedit.h"
#include "help.h"

static char *HelpTitles[NUM_TOPICS] = {
"Version",
"Getting Started",
"Finding and Replacing Text",
"Selecting Text",
"Cut and Paste",
"Features for Programming",
"Using the Mouse",
"Keyboard Shortcuts",
"Shifting and Filling",
"Crash Recovery",
"Preferences",
"Shell Commands/Filters",
"Regular Expressions",
"NEdit Command Line",
"Server Mode and nc",
"Customizing NEdit",
"X Resources",
"Macros, Learn/Replay",
"Actions",
"Problems/Bugs",
"Mailing Lists",
"Distribution Policy",
"Tabs Dialog"};

static char *HelpText[NUM_TOPICS] = {
"NEdit Version 4.0.3\n\
January 16, 1997\n\
\n\
Copyright (c) 1992, 1993, 1994, 1996\n\
Universities Research Association, Inc.\n\
All rights reserved.\n\
\n\
NEdit was written by Mark Edel, Joy Kyriakopulos, Arnulfo Zepeda-Navratil, \
Suresh Ravoor, Donna Reid, and Jeff Kallenbach, \
at Fermi National Accelerator Laboratory*.\n\
\n\
The regular expression matching routines used in NEdit are adapted (with \
permission) from original code written by Henry Spencer at the \
University of Toronto.\n\
\n\
NEdit sources, executables, additional documentation, and contributed \
software are available from ftp.fnal.gov in the /pub/nedit directory.\n\
\n\
Send questions or comments to: nedit_support@fnal.gov.\n\
\n\
Mark Edel\n\
edel@fnal.gov\n\
Fermi National Accelerator Laboratory\n\
P.O. Box 500\n\
Batavia, IL 60148\n\
\n\
* Fermi National Accelerator Laboratory is \
operated by Universities Research Association, Inc., under \
contract DE-AC02-76CHO3000 with the U.S. Department of Energy.",

"Welcome to NEdit!\n\
\n\
NEdit is a standard GUI (Graphical User Interface) style text editor for \
programs and plain-text files.  Users of Macintosh and MS Windows based \
text editors should find NEdit a familiar and comfortable environment.  \
NEdit provides all of the standard menu, dialog, editing, and mouse \
support, as well as all of the standard shortcuts to which the users \
of modern GUI based environments are accustomed.  For users of older \
style Unix editors, welcome to the world of mouse-based editing!\n\
\n\
Help sections of interest to new users:\n\
\n\
	Selecting Text\n\
	Finding and Replacing Text\n\
	Cut and Paste\n\
	Using the Mouse\n\
	Keyboard Shortcuts\n\
	Shifting and Filling\n\
\n\
Programmers should also read:\n\
\n\
	Features for Programming\n\
\n\
If you get into trouble, all of the dialogs have Cancel buttons, and the \
Undo command in the Edit menu can reverse any modifications that you \
make.  NEdit does not change the file you are editing until you tell it \
to Save.\n\
\n\
Editing an Existing File\n\
\n\
To open an existing file, choose Open... from the file menu. Select the \
file that you want to open in the pop-up dialog that appears and click on \
OK.  You may open any number of files at the same time.  Each file will \
appear in its own editor window.  Using Open... rather than re-typing the \
NEdit command and running additional copies of NEdit, will give you quick \
access to all of the files you have open via the Windows menu, and ensure \
that you don't accidentally open the same file twice.  NEdit has no \
\"main\" window.  It remains running as long as at least one editor window \
is open.\n\
\n\
Creating a New File\n\
\n\
If you already have an empty (Untitled) window displayed, just begin \
typing in the window.  To create a new Untitled window, choose New from \
the File menu.  To give the file a name and save its contents to the \
disk, choose Save or Save As... from the File menu.\n\
\n\
Backup Files\n\
\n\
NEdit maintains periodic backups of the file you are editing so that you \
can recover the file in the event of a problem such as a system crash, \
network failure, or X server crash.  These files are saved under the name \
~filename (on Unix) or _filename (on VMS), where filename is the name of \
the file you were editing.  If an NEdit process is killed, some of these \
backup files may remain in your directory.  (To remove one of these files \
on Unix, you may have to prefix the ~ (tilde) character with a \
(backslash) to prevent the shell from interpreting it as a special \
character.)\n\
\n\
Shortcuts\n\
\n\
As you become more familiar with NEdit, substitute the control and \
function keys shown on the right side of the menus for pulling down menus \
with the mouse.\n\
\n\
Dialogs are also streamlined so you can enter information quickly and \
without using the mouse*.  To move the keyboard focus around a dialog, \
use the tab and arrow keys.  One of the buttons in a dialog is usually \
drawn with a thick, indented, outline.  This button can be activated by \
pressing return or enter.  The Cancel or Dismiss button can be activated \
by pressing escape.  For example, to replace the string \"thing\" with \
\"things\" type:\n\
\n\
    <ctrl-r>thing<tab>things<return>\n\
\n\
To open a file named \"whole_earth.c\", type:\n\
\n\
    <ctrl-o>who<return>\n\
\n\
(how much of the filename you need to type depends on the other files in \
the directory).  See the section called Keyboard Shortcuts for more \
details.\n\
\n\
* Users who have set their keyboard focus mode to \"pointer\" should set \
\"Popups Under Pointer\" in the Default Settings menu to avoid the \
additional step of moving the mouse into the dialog.",

"The Search menu contains a number of commands for finding and replacing text.\n\
\n\
The Find... and Replace... commands present dialogs for entering text for \
searching and replacing.  These dialogs also allow you to choose whether you \
want the search to be sensitive to upper and lower case, or whether to use the \
standard Unix pattern matching characters (regular expressions).  Searches \
begin at the current text insertion position.\n\
\n\
Find Same and Replace Same repeat the last find or replace command without \
prompting for search strings.  To selectively replace text, use the two \
commands in combination: Find Same, then Replace Same if \
the highlighted string should be replaced, or Find Same again to go to the \
next string.\n\
\n\
Find Selection searches for the text contained in the current primary \
selection (see Selecting Text).  The selected text does not have to \
be in the current editor window, it may even be in another program.  For \
example, if the word dog appears somewhere in a window on your screen, and \
you want to find it in the file you are editing, select the \
word dog by dragging the mouse across it, switch to your NEdit window and \
choose Find Selection from the Search menu.\n\
\n\
Searching Backwards\n\
\n\
Holding down the shift key while choosing any of the search or replace \
commands from the menu (or using the keyboard shortcut), will search in \
the reverse direction.  Users who have set the search direction using \
the buttons in the search dialog, may find it a bit confusing that Find \
Same and Replace Same don't continue in the same direction as the \
original search (for experienced users, consistency \
of the direction implied by the shift key is more important). \n\
\n\
Selective Replacement\n\
\n\
To replace only some occurrences of a string within a file, choose Replace... \
from the Search menu, enter the string to search for and the string to \
substitute, and finish by pressing the Find button.  When the first \
occurrence is highlighted, use either Replace Same (^T) to replace it, or \
Find Same (^G) to move to the next occurrence without replacing it, and \
continue in such a manner through all occurrences of interest.\n\
\n\
To replace all occurrences of a string within some range of text, \
select the range (see Selecting Text), choose Replace... from the \
Search menu, type the string to search for and the string to substitute, and \
press the \"R. in Selection\" button in the dialog.  Note that selecting text \
in the Replace... dialog will unselect the text in the window.",

"NEdit \
has two general types of selections, primary (highlighted text), and \
secondary (underlined text). Selections can cover either \
a simple range of text between two points in the file, or they can \
cover a rectangular area of the file.  Rectangular selections \
are only useful with non-proportional (fixed spacing) fonts.\n\
\n\
To select text for copying, deleting, or replacing, press the left \
mouse button with the pointer at one end of the text you want to select, \
and drag it to the other end.  The text will become highlighted.  To \
select a whole word, double click (click twice quickly in succession).  \
Double clicking and then dragging the mouse will select a number of words.  \
Similarly, you can select \
a whole line or a number of lines by triple clicking or triple clicking and \
dragging.  Quadruple clicking selects the whole file.  \
After releasing the mouse button, you can still adjust a selection \
by holding down the shift key and dragging on either end of the selection.  \
To delete the selected text, press delete or backspace.  To replace it, \
begin typing.\n\
\n\
To select a rectangle or column of text, hold the Ctrl key while dragging \
the mouse.  Rectangular selections can be used in any context that \
normal selections can be used, including cutting and pasting, filling, \
shifting, dragging, and searching.  Operations on rectangular selections \
automatically fill in tabs and spaces to maintain alignment of text \
within and to the right of the selection.  Note that the interpretation \
of rectangular selections by Fill Paragraph is slightly different from that \
of other commands, the section titled \"Shifting and Filling\" has details.\n\
\n\
The middle mouse button can be used to make an \
additional selection (called the secondary selection).  As soon as the \
button is released, the contents of this selection will be \
copied to the insert position of the window where the mouse was last \
clicked (the destination window).  This position is marked by a caret \
shaped cursor when the \
mouse is outside of the destination window.  \
If there is a (primary) selection, adjacent to the cursor \
in the window, \
the new text will replace the selected text.  Holding the shift key \
while making the secondary selection will move the text, deleting it \
at the site of the secondary selection, rather than \
copying it.\n\
\n\
Selected text can also be dragged to a new location in the file using the \
middle mouse button.  Holding the shift key while \
dragging the text will copy the selected text, leaving the original \
text in place.  Holding the control key will drag the text in overlay \
mode.\n\
\n\
Normally, dragging moves text by removing it from the \
selected position at the start of the drag, and inserting it at a \
new position relative to to the mouse.  Dragging a block of text \
over existing characters, displaces the characters to \
the end of the selection.  In overlay mode, characters which are \
occluded by blocks of text being dragged are simply removed.  When \
dragging non-rectangular selections, overlay mode also converts the \
selection to rectangular form, allowing it to be dragged outside of \
the bounds of the existing text.\n\
\n\
The section \"Using the Mouse\" sumarizes the mouse commands for making \
primary and secondary selections.  Primary selections can also be made \
via keyboard commands, see \"Keyboard Shortcuts\".",

"The easiest way to copy and move text around in your file or between \
windows, is to use the clipboard, an imaginary area that temporarily stores \
text and data.  The Cut command removes the \
selected text (see Selecting Text) from your file and places it in the \
clipboard.  Once text is in \
the clipboard, the Paste command will copy it to the insert position in the \
current window.  For example, to move some text from one place to another, \
select it by \
dragging the mouse over it, choose Cut to remove it, \
click the pointer to move the insert point where you want the text inserted, \
then choose Paste to insert it.  Copy copies text to the clipboard without \
deleting it from your file.  You can also use the clipboard to transfer text \
to and from other Motif programs and X programs which make proper use of \
the clipboard.\n\
\n\
There are a number of other methods for copying and moving text within \
NEdit windows and between NEdit and other programs.  The most common \
such method is clicking the middle mouse button to copy the primary \
selection (to the clicked position).  Copying the selection by clicking \
the middle mouse button in many cases is the only way to transfer data \
to and from many X programs.  Holding the Shift key while clicking the \
middle mouse button moves the text, deleting it from its original \
position, rather than copying it.  Other methods for transferring text \
include secondary selections, primary selection dragging, keyboard-based \
selection copying, and drag and drop.  These are described in detail in \
the sections: Selecting Text, Using the Mouse, and Keyboard Shortcuts.",


"Indentation\n\
\n\
With Auto Indent turned on (the default), NEdit keeps a running indent.  \
When you press the return key, \
space and tabs are inserted to line up the insert point under the start of \
the previous line.  Ctrl+Return in auto-indent mode acts like a normal \
return, With auto-indent turned off, Ctrl+Return does indentation.\n\
\n\
The Shift Left and Shift Right commands adjust the \
indentation for several lines at once.  To shift a block of text one \
character \
to the right, select the text, then choose Shift Right from the Edit menu.  \
Note that the accelerator keys for these menu items are Ctrl+9 and Ctrl+0, \
which correspond to  the right and left parenthesis on most keyboards.  \
Remember them as adjusting the text in the direction pointed to by the \
parenthesis character.  Holding the Shift key while selecting either \
Shift Left or Shift Right will shift the text by one tab stop.\n\
\n\
Tabs\n\
\n\
Most Unix programs for text display and processing, assume an eight \
character tab.  Unfortunately, an 8 character tab is not convenient \
for programming in C, C++, and other structured languages.  Inside \
of NEdit and other editors that allow you to change the interpretation \
of the tab character, it is much easier to work with C code when \
the tab distance is set to match the program indentation distance, \
usually 3 or 4 characters.  However, there are serious tradeoffs \
to using non-standard tabs.  In addition to many Unix utilities not \
displaying or interpreting the files correctly, other programmers \
may not interpret the files correctly, or may use editors which \
can't display or edit the files properly. \n\
\n\
An alternative to changing the interpretation of the tab character \
is tab emulation.  In the Tabs... dialog, turning on Emulated \
Tabs causes the Tab key to insert the correct number of spaces and/or \
tabs to bring the cursor the next emulated tab stop, as if tabs were \
set at the emulated tab distance rather than the hardware tab distance.  \
Backspacing immediately after entering an emulated tab \
will delete it as a unit, but as soon as you move the \
cursor away from the spot, NEdit will forget that the \
collection of spaces and tabs is a tab, and will treat \
it as separate characters.  To enter a real tab \
character with \"Emulate Tabs\" turned on, use Ctrl+Tab.\n\
\n\
It is also possible to tell NEdit not to insert ANY tab characters \
at all in the course of processing emulated tabs, and in shifting \
and rectangular insertion/deletion operations, for programmers \
who worry about the misinterpretation of tab characters on other \
systems. \n\
\n\
Line Numbers\n\
\n\
To find a particular line in a source file by line number, choose Goto \
Line #... from the Search menu.  You can also directly select the line \
number text in the compiler message in the terminal emulator window \
(xterm, decterm, winterm, etc.) where you ran the compiler, and choose \
Goto Selected from the Search \
menu.  Note that under AIXWindows (IBM), selections in aixterms appear to \
go away when you release the mouse, but they are actually retained and \
can be used by NEdit and other programs that use selections.\n\
\n\
To find out the line number of a particular line in your file, turn on \
Statistics Line in the Preferences menu and position the insertion point \
anywhere on the line.  The statistics line continuously updates the line \
number of the line containing the cursor.\n\
\n\
Matching Parentheses\n\
\n\
To help you inspect nested parentheses, brackets, braces, quotes, and other \
characters, NEdit has both an automatic parenthesis matching mode, and a \
Find Matching command.  Automatic parenthesis matching is activated when \
you type, or move the insertion cursor after a parenthesis, bracket, or \
brace.  It momentarily highlights the matching character if that character \
is visible in the window.  To find a matching character anywhere in \
the file, select it or position the cursor after it, and choose Find \
Matching from the Search menu.  If \
the character matches itself, such as a quote or slash, select the first \
character of the pair.  NEdit will match {, (, [, <, \", \', `, /, and \\.\n\
\n\
Opening Included Files\n\
\n\
The Open Selected command in the File menu understands the C \
preprocessor's #include \
syntax, so selecting an #include line and invoking Open Selected will \
generally find the file referred to, unless doing so depends on the \
settings of compiler switches or other information not available to NEdit.\n\
\n\
Finding Subroutine and Data Declarations\n\
\n\
NEdit can process tags files generated using the Unix ctags command.  \
Ctags creates index files correlating names of functions and declarations \
with their locations in C, Fortran, or Pascal source code files. (See the \
ctags manual page for more information).  Ctags produces a file called \
\"tags\" which can be loaded by NEdit.  Once loaded, the information in \
the tags file enables NEdit to go directly to the declaration of a \
highlighted function or data structure name with a single command.  To \
load a tags file, select \"Load Tags File\" from the File menu and choose \
a tags file to load, or specify the name of the tags file on the NEdit \
command line:\n\
\n\
    nedit -tags tags\n\
\n\
NEdit can also be set to load a tags file automatically when it starts up. \
Setting the X resource nedit.tagFile to the name of a tag file tells NEdit \
to look for that file at startup time (see Customizing NEdit).  The file name \
can be either a complete path name, in which case NEdit will always load \
the same tags file, or a file name without a path or with a relative path, \
in which case NEdit will load it starting from the current directory.  The \
second option allows you to have different tags files for different \
projects, each automatically loaded depending on the directory you're in \
when you start NEdit.  Setting the name to \"tags\" is an obvious choice \
since this is the name that ctags uses.\n\
\n\
To find the definition of a function or data structure once a tags file is \
loaded, select the name anywhere it appears in your program (see Selecting \
Text) and choose \"Find Definition\" from the Search menu.",

"Mouse-based \
editing is what NEdit is all about, and learning to use the more advanced \
features like secondary selections and primary selection \
dragging will be well worth your while.\n\
\n\
If you don't have time to learn everything, you can get by adequately with \
just the left mouse button:  Clicking the left button \
moves the cursor.  \
Dragging with the left button makes a selection.  Holding the shift \
key while clicking extends the existing selection, or begins a selection \
between the cursor and the mouse.  Double or triple clicking selects a \
whole word or a whole line.\n\
\n\
This section will make more sense if you also read the section called, \
\"Selecting Text\", which explains the terminology of selections, i.e. \
what is meant by primary, secondary, rectangular, etc.\n\
\n\
\n\
GENERAL\n\
\n\
General meaning of mouse buttons and modifier keys:\n\
\n\
Buttons\n\
\n\
  Button 1 (left)    Cursor position and primary selection\n\
\n\
  Button 2 (middle)  Secondary selections, and dragging and\n\
                     copying the primary selection\n\
\n\
Modifier keys\n\
\n\
  Shift   On primary selections, (left mouse button):\n\
             Extends selection to the mouse pointer\n\
          On secondary and copy operations, (middle):\n\
             Toggles between move and copy\n\
\n\
  Ctrl    Makes selection rectangular or insertion\n\
          columnar\n\
\n\
  Alt*    (on release) Exchange primary and secondary\n\
          selections.\n\
\n\
\n\
BUTTON 1\n\
\n\
The left mouse button is used to position the cursor and to make primary \
selections\n\
\n\
  Click 	Moves the cursor\n\
 \n\
  Double Click	Selects a whole word\n\
\n\
  Triple Click	Selects a whole line\n\
\n\
  Quad Click	Selects the whole file\n\
\n\
  Shift Click	Adjusts (extends or shrinks) the\n\
 		selection, or if there is no existing\n\
 		selection, begins a new selection\n\
 		between the cursor and the mouse.\n\
\n\
  Ctrl+Shift+	Adjusts (extends or shrinks) the\n\
  Click 	selection rectangularly.\n\
\n\
  Drag		Selects text between where the mouse\n\
 		was pressed and where it was released.\n\
\n\
  Ctrl+Drag	Selects rectangle between where the\n\
 		mouse was pressed and where it was\n\
 		released.\n\
\n\
\n\
BUTTON 2\n\
\n\
The middle mouse button is for making secondary selections, and copying and \
dragging the primary selection\n\
\n\
  Click        Copies the primary selection to the\n\
 	       clicked position.\n\
\n\
  Shift+Click  Moves the primary selection to the\n\
 	       clicked position, deleting it from its\n\
 	       original position.\n\
\n\
  Drag	       1) Outside of the primary selection:\n\
 	           Begins a secondary selection.\n\
 	       2) Inside of the primary selection:\n\
 	           Moves the selection by dragging.\n\
\n\
  Ctrl+Drag    1) Outside of the primary selection:\n\
 	           Begins a rectangular secondary\n\
 	           selection.\n\
 	       2) Inside of the primary selection:\n\
 	           Drags the selection in overlay\n\
 	           mode (see below).\n\
\n\
When the mouse button is released after creating a secondary selection:\n\
\n\
  No Modifiers	If there is a primary selection,\n\
 		replaces it with the secondary\n\
 		selection.  Otherwise, inserts the\n\
 		secondary selection at the cursor\n\
 		position.\n\
\n\
  Shift 	Move the secondary selection, deleting\n\
 		it from its original position.  If\n\
 		there is a primary selection, the move\n\
 		will replace the primary selection\n\
 		with the secondary selection.\n\
 		Otherwise, moves the secondary\n\
 		selection to to the cursor position.\n\
\n\
  Alt*		Exchange the primary and secondary\n\
 		selections.\n\
\n\
\n\
While moving the primary selection by dragging with the middle mouse button:\n\
\n\
  Shift   Leaves a copy of the original\n\
 	  selection in place rather than\n\
 	  removing it or blanking the area.\n\
 \n\
  Ctrl	  Changes from insert mode to overlay\n\
 	  mode (see below).\n\
\n\
  Escape  Cancels drag in progress.\n\
\n\
Overlay Mode: Normally, dragging moves text by removing it from the \
selected position at the start of the drag, and inserting it at a \
new position relative to to the mouse. When you drag a block of text \
over existing characters, the existing characters are displaced to \
the end of the selection.  In overlay mode, characters which are \
occluded by blocks of text being dragged are simply removed.  When \
dragging non-rectangular selections, overlay mode also converts the \
selection to rectangular form, allowing it to be dragged outside of \
the bounds of the existing text. \n\
\n\
\n\
* The Alt key may be labeled Meta or Compose-Character on some \
keyboards.  Some window managers, including default configurations \
of mwm, bind combinations of the Alt key and mouse buttons to window \
manager operations.  In NEdit, Alt is only used on button release, \
so regardless of the window manager bindings for Alt-modified mouse \
buttons, you can still do the corresponding NEdit operation by using \
the Alt key AFTER the initial mouse press, so that Alt is held while \
you release the mouse button.  If you find this difficult or \
annoying, you can re-configure most window managers to skip this \
binding, or you can re-configure NEdit to use a different key \
combination.",

"Most of the keyboard shortcuts in NEdit are shown on the right hand sides \
of the pull-down menus.  However, there are more which are not as obvious.  \
These include; dialog button shortcuts; menu and \
dialog mnemonics; labeled keyboard keys, such as the arrows, page-up, \
page-down, and home; and optional Shift modifiers on accelerator keys, like \
[Shift]Ctrl+F.\n\
\n\
\n\
Menu Accelerators\n\
\n\
Pressing the key combinations shown on the right of the menu items is a \
shortcut for selecting the menu item with the mouse.  Some items have the \
shift key enclosed in brackets, such as [Shift]Ctrl+F.  This indicates \
that the shift key is optional.  In search commands, including the shift \
key reverses the direction of the search.  In Shift commands, it makes the \
command shift the selected text by a whole tab stop rather than by \
single characters.\n\
\n\
\n\
Menu Mnemonics\n\
\n\
Pressing the Alt key in combination with one of the underlined characters \
in the menu bar pulls down that menu.  Once the menu is pulled down, \
typing the underlined characters in a menu item (without the Alt key) \
activates that item.  With a menu pulled down, you can also use the arrow \
keys to select menu items, and the space or enter keys to activate them.\n\
\n\
\n\
Keyboard Shortcuts within Dialogs\n\
\n\
One button in a dialog is usually marked with a thick indented outline.  \
Pressing the Return or Enter key activates this button.\n\
\n\
All dialogs have either a Cancel or Dismiss button.  This button can \
be activated by pressing the Escape (or Esc) key.\n\
\n\
Pressing the tab key moves the keyboard focus to the next item in a \
dialog.  Within an associated group of buttons, the arrow keys move \
the focus among the buttons.  Shift+Tab moves backward through the items.\n\
\n\
Most items in dialogs have an underline under one character in their name.  \
Pressing the Alt key along with this character, activates a button as if you \
had pressed it with the mouse, or moves the keyboard focus to the associated \
text field or list.\n\
\n\
You can select items from a list by using the arrow keys to move the \
selection and space to select.\n\
\n\
In file selection dialogs, you can type the beginning characters of the \
file name or directory in the list to select files\n\
\n\
\n\
Labeled Function Keys \n\
\n\
The labeled function keys on standard workstation and PC keyboards, like \
the arrows, and page-up and page-down, are active in NEdit, though not \
shown in the pull-down menus. \n\
\n\
Holding down the control key while pressing a named key extends the \
scope of the action that it performs.  For example, Home normally moves \
the insert cursor the beginning of a line.  Ctrl+Home moves it to the \
beginning of the file. Backspace deletes one character, Ctrl+Backspace \
deletes one word. \n\
\n\
Holding down the shift key while pressing a named key begins or extends \
a selection.  Combining the shift and control keys combines their \
actions.  For example, to select a word without using the mouse, \
position the cursor at the beginning of the word and press \
Ctrl+Shift+RightArrow.  The Alt key modifies selection commands to make \
the selection rectangular. \n\
\n\
Under X and Motif, there are several levels of translation between \
keyboard keys and the actions they perform in a program.  The \
\"Customizing NEdit\", and \"X Resources\" sections of the Help \
menu have more information on this subject.  Because of all of this \
configurability, and since keyboards and standards for the meaning of \
some keys vary from machine to machine, the mappings may be changed from \
the defaults listed below. \n\
\n\
Modifier Keys (in general)\n\
\n\
  Ctrl	 Extends the scope of the action that the key\n\
 	 would otherwise perform.  For example, Home\n\
 	 normally moves the insert cursor the beginning of\n\
 	 a line. Ctrl+Home moves it to the beginning of\n\
 	 the file.  Backspace deletes one character, Ctrl+\n\
 	 Backspace deletes one word.\n\
\n\
  Shift  Extends the selection to the cursor position. If\n\
 	 there's no selection, begins one between the old\n\
 	 and new cursor positions.\n\
\n\
  Alt	 When modifying a selection, makes the selection\n\
 	 rectangular.\n\
\n\
(For the effects of modifier keys on mouse button presses, see \
the section titled \"Using the Mouse\")\n\
\n\
All Keyboards\n\
\n\
  Escape	Cancels operation in progress: menu\n\
  		selection, drag, selection, etc.  Also\n\
  		equivalent to cancel button in dialogs.\n\
\n\
  Backspace	Delete the character before the cursor\n\
\n\
  Ctrl+BS	Delete the word before the cursor\n\
\n\
  Arrows\n\
\n\
    Left	Move the cursor to the left one character\n\
\n\
    Ctrl+Left   Move the cursor backward one word\n\
    		(Word delimiters are settable, see\n\
    		Customizing NEdit, and X Resources)\n\
\n\
    Right	Move the cursor to the right one character\n\
\n\
    Ctrl+Right  Move the cursor forward one word\n\
\n\
    Up  	Move the cursor up one line\n\
\n\
    Ctrl+Up	Move the cursor up one paragraph.\n\
    		(Paragraphs are delimited by blank lines)\n\
\n\
    Down	Move the cursor down one line.\n\
\n\
    Ctrl+Down	Move the cursor down one paragraph.\n\
\n\
  Ctrl+Return	Return with automatic indent, regardless\n\
  		of the setting of Auto Indent.\n\
\n\
  Shift+Return	Return without automatic indent,\n\
  		regardless of the setting of Auto Indent.\n\
\n\
  Ctrl+Tab	Insert an ascii tab character, without\n\
  		processing emulated tabs.\n\
\n\
  Alt+Ctrl+<c>	Insert the control-code equivalent of\n\
  		a key <c>\n\
\n\
  Ctrl+/	Select everything (same as Select\n\
     		All menu item or ^A)\n\
\n\
  Ctrl+\\	Unselect\n\
\n\
  Ctrl+U	Delete to start of line\n\
\n\
PC Standard Keyboard\n\
\n\
  Ctrl+Insert	Copy the primary selection to the\n\
 		clipboard (same as Copy menu item or ^C)\n\
 		for compatibility with Motif standard key\n\
 		binding\n\
  Shift+Ctrl+\n\
  Insert	Copy the primary selection to the cursor\n\
 		location.\n\
\n\
  Delete	Delete the character before the cursor.\n\
 		(Can be configured to delete the character\n\
 		after the cursor, see Customizing NEdit,\n\
 		and X Resources)\n\
\n\
  Ctrl+Delete	Delete to end of line.\n\
\n\
  Shift+Delete	Cut, remove the currently selected text\n\
 		and place it in the clipboard. (same as\n\
 		Cut menu item or ^X) for compatibility\n\
 		with Motif standard key binding\n\
  Shift+Ctrl+\n\
  Delete	Cut the primary selection to the cursor\n\
 		location.\n\
\n\
  Home		Move the cursor to the beginning of the\n\
                line\n\
\n\
  Ctrl+Home	Move the cursor to the beginning of the\n\
                file\n\
\n\
  End		Move the cursor to the end of the line\n\
\n\
  Ctrl+End	Move the cursor to the end of the file\n\
\n\
  PageUp	Scroll and move the cursor up by one page.\n\
\n\
  Ctrl+PageUp   Scroll and move the cursor left by one\n\
  		page.\n\
  PageDown	Scroll and move the cursor down by one\n\
  		page.\n\
\n\
  Ctrl+PageDown Scroll and move the cursor right by one\n\
  		page.\n\
\n\
  F10		Make the menu bar active for keyboard\n\
  		input (Arrow Keys, Return, Escape,\n\
  		and the Space Bar)\n\
\n\
Specialty Keyboards \n\
\n\
On machines with different styles of keyboards, generally, text \
editing actions are properly matched to the labeled keys, such as \
Remove, Next-screen, etc..  If you prefer different key bindings, see \
the heading titled \"Binding Keys to Actions\" in the X Resources \
section of the Help menu.",

"Built in Editing Operations: Shifting, Filling, Case\n\
\n\
\n\
Shift Left, Shift Right \n\
\n\
While shifting blocks of text is most important for programmers (See \
Features for Programming), it is also useful for other tasks, such as \
creating indented paragraphs. \n\
\n\
To shift a block of text one tab stop to the right, select the text, \
then choose Shift Right from the Edit menu.  Note that the accelerator \
keys for these menu items are Ctrl+9 and Ctrl+0, which correspond to \
the right and left parenthesis on most keyboards.  Remember them as \
adjusting the text in the direction pointed to by the parenthesis \
character.  Holding the Shift key while selecting either Shift Left or \
Shift Right will shift the text by one character.\n\
\n\
It is also possible to shift blocks of text by selecting the text \
rectangularly, and dragging it left or right (and up or down as well).  \
Using a rectangular selection also causes tabs within the selection to \
be recalculated and substituted, such that the non-whitespace characters \
remain stationary with respect to the selection.\n\
\n\
\n\
Filling \n\
\n\
The Fill Paragraph command in NEdit is important for anyone who types \
plain blocks of text.  In a plain text file, there is no way to store \
any additional format information, like a word processor might do.  This \
makes it impossible for the editor to tell parts of the text belong \
together as a paragraph, from carefully arranged individual lines.  So, \
unlike a word processor, when you begin editing a paragraph in NEdit, \
nicely arranged by Auto Wrap when you entered it, the lines become messy \
and uneven.\n\
\n\
Since NEdit can't act automatically to keep your text lined up, you need \
to tell it explicitly where to operate, and that is what Fill Paragraph \
is for.  It arranges lines to fill the space between two margins, \
wrapping the lines neatly at word boundaries.  Normally, the left margin \
for filling is inferred from the text being filled, the left edge of the \
text, or the furthest left non-whitespace character.  The right margin \
is either the Wrap Margin, set in the preferences menu (by default, the \
right edge of the window), or can also be chosen on the fly by using a \
rectangular selection (see below).\n\
\n\
There are three ways to use Fill Paragraph.  The simplest is, while you \
are typing text, and there is no selection, simply select Fill Paragraph \
(or type Ctrl+J), and NEdit will arrange the text in the paragraph \
adjacent to the cursor.  A paragraph, in this case, means an area of \
text delimited by blank lines.\n\
\n\
The second way to use Fill Paragraph is with a selection.  If you select \
a range of text and then chose Fill Paragraph, all of the text in the \
selection will be filled.  Again, continuous text between blank lines is \
interpreted as paragraphs and filled individually. \n\
\n\
The third way to use Fill Paragraph is with a rectangular selection.  \
Fill Paragraph treats rectangular selections differently from other \
commands.  Instead of simply filling the text inside the rectangular \
selection, NEdit interprets the right edge of the selection as the \
requested wrap margin.  Text to the left of the selection is not \
disturbed (the usual interpretation of a rectangular selection), but \
text to the right of the selection is included in the operation and \
is pulled in to the selected region.\n\
\n\
Changing Case\n\
\n\
The Capitalize and Lowercase commands operate on the current (primary) \
selection, or if there is no text selected in the window, on the single \
character before the cursor.",

"If a system crash, network failure, X server crash, or program error should \
happen while you are editing a file, you can still recover most of your work.  \
NEdit maintains a backup file which it updates periodically (every 8 editing \
operations or 80 characters typed).  This file is has the same name as the \
file that you are editing, but with the character \"~\" (tilde) on Unix or \
\"_\" (underscore) on VMS prefixed \
to the name.  To recover a file after a crash, simply rename the file to \
remove the tilde or underscore character, replacing the older version of the \
file.  \
(Because several of the Unix shells consider the tilde to be a special \
character, you may have to prefix the character with a \"\\\" (backslash) \
when you move or delete an NEdit backup file.)\n\
\n\
Example, to recover the file called \"help.c\" on Unix type the command:\n\
\n\
    mv \\~help.c help.c\n\
\n\
On VMS, type:\n\
\n\
    RENAME _HELP.C HELP.C",

"The Preferences menu allows you to set options for both the current \
editing window, and default values for newly created windows and future \
NEdit sessions.  The first group of options in the Preferences menu \
take effect immediately and refer to the current window only.  Options \
in the Default Settings sub-menu have no effect on the current window, \
but instead provide initial settings for future windows created using \
the New or Open commands.  Preferences set in the Default Settings \
sub-menu can also be saved in a file \
that is automatically read by NEdit at startup time, by selecting \
Save Defaults.\n\
\n\
    Auto Indent -- Maintain a running indent.  Pressing\n\
        the return key will line up the cursor with the\n\
        indent level of the previous line.\n\
\n\
    Auto Wrap -- Wrap text at word boundaries when the\n\
        cursor reaches the right margin.\n\
\n\
    Wrap Margin... -- Set margin for Auto Wrap and Fill\n\
    	Paragraph.  By default, lines wrap at the right\n\
    	margin of the window, but a wrap margin can also\n\
    	be set at a specific column.\n\
\n\
    Preserve Last Version -- On Save, write a backup copy\n\
    	of the file as it existed before the Save command\n\
    	with the extension .bck (Unix only).\n\
\n\
    Incremental Backup -- Periodically make a backup copy\n\
        of the file being edited under the name ~filename\n\
        on Unix or _filename on VMS (see Crash Recovery).\n\
\n\
    Show Matching (..) -- Momentarily highlight matching\n\
        parenthesis, brackets, and braces when one of\n\
        these characters is typed, or when the insertion\n\
        cursor is positioned after it.\n\
\n\
    Text Font... -- Set the font for the text in this\n\
        NEdit window.  To set the font for all windows\n\
        use the equivalent item in the Default Settings\n\
        sub-menu.  Note that since the font selection\n\
        dialog narrows its lists of font characteristics\n\
        depending on those already selected, it is\n\
        important to know that you can unselect them\n\
        by clicking on the selected items a second time.\n\
\n\
    Tabs -- Set the tab distance (number of characters\n\
        between tab stops) for tab characters, and\n\
        control tab emulation and use of tab characters\n\
        in padding and emulated tabs.\n\
\n\
    Overstrike -- In overstrike mode, new characters\n\
        entered replace the characters in front of the\n\
        insertion cursor, rather than being inserted\n\
        before them.\n\
\n\
    Statistics Line -- Show the full file name, line\n\
        number, and length of the file being edited.\n\
\n\
    Default Settings -- Sub-menu of initial settings\n\
        for future windows.  These are the same as the\n\
        options in the main part of the menu, but apply\n\
        as defaults for future windows created during\n\
        this NEdit session.  These settings can be saved\n\
        using the Save Defaults command below, to be\n\
        loaded automatically each time NEdit is started.\n\
\n\
    Save Defaults -- Save the default options as set\n\
        under Default Settings for future NEdit sessions.",

"The Shell menu (Unix versions only) allows you to execute Unix shell commands \
from within NEdit.  You can add items to the menu to extend NEdit's command \
set or to incorporate custom automatic editing features using shell commands \
or editing languages like awk and \
sed.  To add items to the menu, use the Shell Commands... dialog in the \
Preferences menu under Default Settings.  NEdit comes pre-configured with \
a few useful Unix commands like spell and sort, but we encourage you to \
add your own custom extensions.\n\
\n\
Filter Selection... prompts you for a Unix command to \
use to process the currently selected text.  The output from this \
command replaces the contents of the selection.\n\
\n\
Execute Command... prompts you for a Unix command and replaces the \
current selection with the output of the command.  If there is no \
selection, it deposits the output at the current insertion point.\n\
\n\
Execute Command Line uses the position of the cursor in the window \
to indicate a line to execute as a shell command line.  The cursor may \
be positioned anywhere on the line.  This command allows you to use \
an NEdit window as an editable command window for saving output \
and saving commands for re-execution.\n\
\n\
The X resource called nedit.shell (See Customizing NEdit) determines \
which Unix shell is used to execute commands.  The default value for \
this resource is /bin/csh.",

"Regular expressions are available in the Find... and Replace... \
dialogs as a way to match inexact sequences of characters.  \
Regular expression substitution can also be used to program \
automatic editing operations.  For example, the following are \
search and replace strings to find occurrences of the \
subroutine get_x, reverse the first and second parameters, add a \
third parameter of NULL, and change the name to new_get_x\":\n\
\n\
	Search string:  get_x\\(([^ ,]*), ([^\\)]*)\\)\n\
	Replace string: new_get_x(\\2, \\1, NULL)\n\
\n\
To use regular expressions, click on the Regular Expression button \
in the Find... or Replace... dialogs before doing a search or \
replacement.\n\
\n\
Regular Expression Syntax\n\
\n\
The components of a regular expression are: branches, \
pieces, atoms, and ranges. A regular expression consists of zero or \
more branches, separated by `|'.  It matches anything that matches one \
of the branches.\n\
\n\
A branch is zero or more pieces, concatenated.  It matches a match for \
the first, followed by a match for the second, etc.\n\
\n\
A piece is an atom possibly followed by `*', `+', or `?'.  An atom \
followed by `*' matches a sequence of 0 or more matches of the atom.  An \
atom followed by `+' matches a sequence of 1 or more matches of the \
atom.  An atom followed by `?' matches a match of the atom, or the null \
string.\n\
\n\
An atom is a regular expression in parentheses (matching a match for the \
regular expression), a range (see below), `.' (matching any single \
character), `^' (matching the null string at the beginning of a line \
string), `$' (matching the null string at the end of a line), \
a `\\' followed by a single character (matching that character), or a \
single character with no other significance (matching that character). \
\\t, \\n, \\b, \\r, and \\f represent the characters tab newline, backspace, \
carriage return, and form feed.\n\
\n\
A range is a sequence of characters enclosed in `[]'.  It normally \
matches any single character from the sequence.  If the sequence begins \
with `^', it matches any single character not from the rest of the \
sequence.  If two characters in the sequence are separated by `-', this \
is shorthand for the full list of ASCII characters between them (e.g. \
`[0-9]' matches any decimal digit).  To include a literal `]' in the \
sequence, make it the first character (following a possible `^').  To \
include a literal `-', make it the first or last character.  A backslash \
`\\' followed by a single character includes that character, however \
backslashes are not necessary for most special characters, since inside \
a range, only the `]', `-', and '\\' characters are treated specially.\n\
\n\
Substitution\n\
\n\
Wherever the substitution string contains the character `&', NEdit will \
substitute the the entire string that was matched in the Find operation.  \
Up to nine sub-expressions of the match string can also be inserted into \
the replacement string, using `\\' followed by a digit. \\1 through \\9 \
represent the strings that matched parenthesized expressions within the \
regular expression, numbered left-to-right in order of their opening \
parentheses.  Preceding & or \\1-9 with \\U, \\u, \\L, or \\l adjusts the \
case \
of the inserted text.  \\u and \\l change only the first character, while \
\\U and \\L change the entire string to upper or lower case.  \\t, \\n, \
\\b, \\r, and \\f represent the characters tab newline, backspace, \
carriage return, and form feed in a substitution string represent the tab and \
newline characters as they do in match strings.\n\
\n\
Ambiguity\n\
\n\
If a regular expression could match two different parts of the text, \
it will match the one which begins earliest.  If both begin in \
the same place but match different lengths, or match the same length \
in different ways, life gets messier, as follows.\n\
\n\
In general, the possibilities in a list of branches are considered in \
left-to-right order, the possibilities for `*', `+', and `?' are \
considered longest-first, nested constructs are considered from the \
outermost in, and concatenated constructs are considered leftmost-first.  The \
match that will be chosen is the one that uses the earliest possibility \
in the first choice that has to be made.  If there is more than one \
choice, the next will be made in the same manner (earliest possibility) \
subject to the decision on the first choice.  And so forth.\n\
\n\
For example, `(ab|a)b*c' could match `abc' in one of two ways.  The \
first choice is between `ab' and `a'; since `ab' is earlier, and does \
lead to a successful overall match, it is chosen.  Since the `b' is \
already spoken for, the `b*' must match its last possibility-the empty \
string-since it must respect the earlier choice.\n\
\n\
In the particular case where no `|'s are present and there is only one \
`*', `+', or `?', the net effect is that the longest possible match will \
be chosen.  So `ab*', presented with `xabbbby', will match `abbbb'. \
Note that if `ab*' is tried against `xabyabbbz', it will match `ab' just \
after `x', due to the begins-earliest rule.  (In effect, the decision on \
where to start the match is the first choice to be made, hence subsequent \
choices must respect it even if this leads them to less-preferred \
alternatives.)",

#ifndef VMS
"nedit [-read] [-create] [-line n | +n] [-server]\n\
    [-do command] [-tags file] [-tabs n] [-wrap]\n\
    [-nowrap] [-autoindent] [-noautoindent] [-autosave]\n\
    [-noautosave] [-rows n] [-columns n] [-font font]\n\
    [-geometry geometry] [-display [host]:server[.screen]\n\
    [-xrm resourcestring] [-svrname name] [file...]\n\
\n\
    -read -- Open the file Read Only regardless of\n\
    	the actual file protection.\n\
\n\
    -create -- Don't warn about file creation when\n\
    	a file doesn't exist.\n\
\n\
    -line n (or +n) -- Go to line number n\n\
\n\
    -server -- Designate this session as an NEdit\n\
        server, for processing commands from the nc\n\
        program.  nc can be used to interface NEdit to\n\
        code development environments, mailers, etc.,\n\
        or just as a quick way to open files from the\n\
        shell command line without starting a new NEdit\n\
        session.\n\
\n\
    -do command -- Execute an NEdit action routine.\n\
        on each file following the -do argument on the\n\
        command line.  -do is particularly useful from\n\
        the nc program, where nc -do can remotely\n\
        execute commands in an nedit -server session.\n\
\n\
    -tags file -- Load a file of directions for finding\n\
        definitions of program subroutines and data\n\
        objects.  The file must be of the format gen-\n\
        erated by the Unix ctags command.\n\
\n\
    -tabs n -- Set tab stops every n characters.\n\
\n\
    -wrap, -nowrap -- Wrap lines at the right edge of\n\
        the window rather than continuing them past it.\n\
\n\
    -autoindent, noautoindent -- Maintain a running\n\
        indent.\n\
\n\
    -autosave, -noautosave -- Maintain a backup copy of\n\
        the file being edited under the name ~filename \n\
        (on Unix) or _filename (on VMS).\n\
\n\
    -rows n -- Default height in characters for an editing\n\
        window.\n\
\n\
    -columns n -- Default width in characters for an\n\
        editing window.\n\
\n\
    -font font (or -fn font) -- Font for text being\n\
        edited (Font for menus and dialogs can be set\n\
        with -xrm \"*fontList:font\").\n\
\n\
    -display [host]:server[.screen] -- The name of the\n\
        X server to use.  host specifies the machine,\n\
        server specifies the display server number, and\n\
        screen specifies the screen number.  host or\n\
        screen can be omitted and default to the local\n\
        machine, and screen 0.\n\
\n\
    -geometry geometry (or -g geometry) -- The initial\n\
        size and/or location of editor windows.  The\n\
        argument geometry has the form:\n\
\n\
        [<width>x<height>][+|-][<xoffset>[+|-]<yoffset>]\n\
\n\
        where <width> and <height> are the desired width\n\
        and height of the window, and <xoffset> and\n\
        <yoffset> are the distance from the edge of the\n\
        screen to the window, + for top or left, - for\n\
        bottom or right.\n\
\n\
    -background color (or -bg color) -- Background color.\n\
        (background color for text can be set separately\n\
        with -xrm \"nedit*text:background color\").\n\
\n\
    -foreground color (or -fg color) -- Foreground color.\n\
        (foreground color for text can be set separately\n\
        with -xrm \"nedit*text:foreground color\").\n\
\n\
    -xrm resourcestring -- Set the value of an X resource\n\
        to override a default value (see Customizing NEdit).\n\
\n\
    -svrname name -- When starting nedit in server mode,\n\
        name the server, such that it responds to requests\n\
        only when nc is given a correspoding -svrname\n\
        argument.  By naming servers, you can run several\n\
        simultaneously, and direct files and commands\n\
        specifically to any one.",
#else
"Command Format:\n\
\n\
    NEDIT [filespec[,...]]\n\
\n\
The following qualifiers are accepted:\n\
\n\
    /read -- Open the file Read Only regardless of\n\
    	the actual file protection.\n\
\n\
    /create -- Don't warn about file creation when\n\
    	a file doesn't exist.\n\
\n\
    /line=n -- Go to line #n\n\
\n\
    /server -- Designate this session as an NEdit\n\
        server for processing commands from the nc\n\
        program.  nc can be used to interface NEdit to\n\
        code development environments, mailers, etc.,\n\
        or just as a quick way to open files from the\n\
        shell command line without starting a new NEdit\n\
        session.\n\
\n\
    /do=command -- Execute an NEdit action routine.\n\
        on each file following the /do argument on the\n\
        command line.  /do is particularly useful from\n\
        the nc program, where nc /do can remotely\n\
        execute commands in an nedit /server session.\n\
    /tags=file -- Load a file of directions for finding\n\
        definitions of program subroutines and data\n\
        objects.  The file must be of the format gen-\n\
        erated by the Unix ctags command.\n\
\n\
    /wrap, /nowrap -- Wrap lines at the right edge of\n\
        the window rather than continuing them past it.\n\
\n\
    /autoindent, noautoindent -- Maintain a running\n\
        indent.\n\
\n\
    /autosave, /noautosave -- Maintain a backup copy of\n\
        the file being edited under the name ~filename \n\
        (on Unix) or _filename (on VMS).\n\
\n\
    /rows=n -- Default width in characters for an editing\n\
        window.\n\
\n\
    /columns=n -- Default height in characters for an\n\
        editing window.\n\
\n\
    /font=font (or /fn=font) -- Font for text being\n\
        edited (Font for menus and dialogs can be set\n\
        with -xrm \"*fontList:font\").\n\
\n\
    /display [host]:server[.screen] -- The name of the\n\
        X server to use.  host specifies the machine,\n\
        server specifies the display server number, and\n\
        screen specifies the screen number.  host or\n\
        screen can be omitted and default to the local\n\
        machine, and screen 0.\n\
\n\
    /geometry=geometry (or /g=geometry) -- The initial\n\
        size and/or location of editor windows.  The\n\
        argument geometry has the form:\n\
\n\
        [<width>x<height>][+|-][<xoffset>[+|-]<yoffset>]\n\
\n\
        where <width> and <height> are the desired width\n\
        and height of the window, and <xoffset> and\n\
        <yoffset> are the distance from the edge of the\n\
        screen to the window, + for top or left, - for\n\
        bottom or right.\n\
\n\
    /background=color (or /bg=color) -- Background color.\n\
        (background color for text can be set separately\n\
        with /xrm \"nedit*text:background color\").\n\
\n\
    /foreground=color (or /fg=color) -- Foreground color.\n\
        (foreground color for text can be set separately\n\
        with -xrm \"nedit*text:foreground color\").\n\
\n\
    /xrm=resourcestring -- Set the value of an X resource\n\
        to override a default value (see Customizing NEdit).\n\
\n\
    /svrname=name -- When starting nedit in server mode,\n\
        name the server, such that it responds to requests\n\
        only when nc is given a correspoding -svrname\n\
        argument.  By naming servers, you can run several\n\
        simultaneously, and direct files and commands\n\
        specifically to any one.\n\
\n\
Unix-style command lines (but not file names) are also acceptable:\n\
\n\
    nedit -rows 20 -wrap file1.c file2.c\n\
\n\
is equivalent to:\n\
\n\
    nedit /rows=20/wrap file1.c, file2.c",
#endif /*VMS*/

"NEdit can be operated on its own, or as a two-part client/server \
application.  Client/server mode is useful for \
integrating NEdit with software \
development environments, mailers, and other programs; or just as a \
quick way to open files from the shell command line without starting \
a new NEdit session.\n\
\n\
To run NEdit in server mode, type:\n\
\n\
    nedit -server\n\
\n\
NEdit can also be started in server mode via the nc program when \
no servers are available.\n\
\n\
The nc (for NEdit Client) program, which is distributed along with nedit, sends \
commands to an nedit server to open files, select lines, or execute editor \
actions.  It accepts a limited set of the nedit command line options: -read, \
-create, -line (or +n), -do, and a list of file names.  Listing a file on the \
nc command line means, open it if it is not already open and bring the window \
to the front.  -read and -create affect only newly opened files, but -line and \
-do can also be used on files which are already open (See  \
\"NEdit Command Line\" for more information).\n\
\n\
In typical Unix style, arguments affect the files which follow them on the \
command line, for example:\n\
\n\
    incorrect:   nc file.c -line 25\n\
    correct:     nc -line 25 file.c\n\
\n\
nc also accepts one command line option of its own, -noask (or -ask), \
which instructs it whether to automatically start a server if one is not \
available.  This is also settable via the X resource, nc.autoStart \
(See X Resources).\n\
\n\
Sometimes it is useful to have more than one NEdit server running, for \
example to keep mail and programming work separate, or more importantly \
for working with tools like ClearCase which provide different views of \
the file system from different shells.  The option, -svrname, to both nedit \
and nc, allow you to start, and communicate with, separate named servers.  \
A named server responds only to requests with the corresponding -svrname \
argument.\n\
\n\
Communication between nc and nedit is through the X display.  So as long as X \
windows is set up and working properly, nc will will work \
properly as well.  nc uses the DISPLAY environment variable, the machine name \
and your user name to find the appropriate server, meaning, if you have several \
machines sharing a common file system, nc will not be able to find a server \
that is running on a machine with a different host name, even though it may be \
perfectly appropriate for editing a given file.\n\
\n\
The command which nc uses to start an nedit server is settable \
via the X resource nc.serverCommand, by default, \
\"nedit -server\".",

"NEdit can be customized in quite a number of ways.  The most important \
user-settable options are presented in the Preferences menu, including \
all options that users might need to change during an editing session.  \
Options set in the Default Settings sub-menu of the Preferences menu \
can be preserved between sessions by selecting Save Defaults, \
which writes a file called .nedit in the user's \
home directory.  See the section titled \"Preferences\" for more details.\n\
\n\
User defined commands can be added to both NEdit's Shell and Macro \
menus.  Dialogs for creating items in these menus can be found in the \
Default Settings sub menu of the Preferences menu (labeled Shell Commands and \
Macro Commands).  Since NEdit's macro facility is not yet well developed, \
the best way to add complex automatic editing features is through shell \
commands that call programs like awk and sed, rather than through NEdit \
macros.\n\
\n\
For hard-core users who depend on NEdit every day and want to tune \
every excruciating detail, there are also X resources for a vast number \
of such details, down to the color of each individual button.  However, \
some options of importance to average users are also set via X \
resources, most importantly, key binding.  While limited key binding can \
be done through the Macro Commands dialog in the Preferences menu, \
significant changes should be made via the Translations resource and menu \
accelerator resources.  The section titled \"X Resources\" has more \
information on setting X resources, as well as a list of selected resources \
of interest to the average user.",

"NEdit has additional options to those provided in the Preferences menu \
which are set using X resources.  \
Like most other X programs, NEdit can be customized to vastly unnecessary \
proportions, from initial window positions down to the font and shadow \
colors of each individual button (A complete discussion of how to do \
this is left to books on the X Windows System).  Key binding (see \
\"Binding Keys to Actions\" below) \
is one of the most useful of these resource settable options.\n\
\n\
X resources are usually specified in a file called .Xdefaults or \
.Xresources in your \
home directory (on VMS this is sys$login:decw$xdefaults.dat).  On some \
systems, this file is read and its information \
attached to the X server (your screen) when you start X.  On other \
systems, the .Xdefaults file is read each time you run an X program.  \
When X resource values are attached to the X server, changes to the \
resource file are not available to application programs until you either run \
the xrdb program with the appropriate file as input, or re-start \
the X server.\n\
\n\
The .nedit File\n\
\n\
The .nedit (saved preferences) file is in the same format as an X resource \
file, and its contents can be moved into your X resource file.  One reason \
for doing so would be to attach server specific preferences, such as \
a default font to a particular X server.  Another reason for moving \
preferences into the X resource file would be to keep preferences menu \
options and resource settable options together in one place. Though \
the files are the same format, additional resources should not be added \
to the .nedit file, they will not be read, and NEdit modifies this file \
by overwriting it completely.  Note also that the contents of the .nedit \
file take precedence over the values of X resources.  Using \
Save Defaults after moving the contents of your .nedit file to your \
.Xdefaults file will re-create the .nedit file, interfering with \
the options that you have moved.\n\
\n\
Selected X Resource Names\n\
\n\
The following are selected NEdit resource names and default values \
for NEdit options not settable via the Preferences menu (for preference \
resource names, see your .nedit file):\n\
\n\
    nedit.tagFile: (not defined) -- The name of a file\n\
        of the type produced by the Unix ctags command\n\
        which NEdit will load at startup time (see\n\
        Features for Programmers).  The tag file provides\n\
        a database from which NEdit can automatically\n\
        open files containing the definition of a\n\
        particular subroutine or data type.\n\
\n\
    nedit.shell: /bin/csh -- (Unix systems only) The Unix\n\
        shell (command interpreter) to use for executing\n\
        commands from the Shell menu\n\
\n\
    nedit.wordDelimiters: .,/\\\\`'!@#%^&*()-=+{}[]\":;<>?\n\
        -- The characters, in addition to blanks and tabs,\n\
        which mark the boundaries between words for the\n\
        move-by-word (Ctrl+Arrow) and select-word (double\n\
        click) commands.\n\
\n\
    nedit.remapDeleteKey: True -- Setting this resource\n\
        to False restores the original Motif binding of\n\
        the delete key to forward-delete.  This binding\n\
        causes problems when X servers with one delete/\n\
        backspace configuration are connected with X\n\
        clients of the other.  Users with a backspace\n\
        key in the backspace/delete position and who use\n\
        only machines with that style of keyboard can\n\
        set this resource to False to get back the\n\
        forward-delete function of the delete key.\n\
\n\
    nedit.stdOpenDialog: False -- Setting this resource\n\
        to True restores the standard Motif style of\n\
        Open dialog.  NEdit file open dialogs are missing\n\
        a text field at the bottom of the dialog, where\n\
        the file name can be entered as a string.  The\n\
        field is removed in NEdit to encourage users to\n\
        type file names in the list, a non-standard, but\n\
        much faster method for finding files.\n\
\n\
    nedit.maxPrevOpenFiles: 30 -- Number of files listed\n\
        in the Open Previous sub-menu of the File menu.\n\
        Setting this to zero disables the Open Previous\n\
        menu item and maintenance of the .neditdb file.\n\
\n\
    nedit.printCommand: (system specific) -- Command used\n\
        by the print dialog to print a file, i.e. lp,\n\
        lpr, etc..  The command must be capable of\n\
        accepting input via stdin (standard input).\n\
\n\
    nedit.printCopiesOption: (system specific) -- Option\n\
        name used to specify multiple copies to the print\n\
        command.  If the option should be separated from\n\
        its argument by a space, leave a trailing space.\n\
        If blank, no \"Number of Copies\" item will\n\
        appear in the print dialog.\n\
\n\
    nedit.printQueueOption: (system specific) -- Option\n\
        name used to specify a print queue to the print\n\
        command.  If the option should be separated from\n\
        its argument by a space, leave a trailing space.\n\
        If blank, no \"Queue\" item will appear in the\n\
        print dialog.\n\
\n\
    nedit.printNameOption: (system specific) -- Option\n\
        name used to specify a job name to the print\n\
        command.  If the option should be separated from\n\
        its argument by a space, leave a trailing space.\n\
        If blank, no job or file name will be attached\n\
        to the print job or banner page.\n\
\n\
    nedit.printHostOption: (system specific) -- Option\n\
        name used to specify a host name to the print\n\
        command.  If the option should be separated from\n\
        its argument by a space, leave a trailing space.\n\
        If blank, no \"Host\" item will appear in the\n\
        print dialog.\n\
\n\
    nedit.printDefaultQueue: (system specific) -- The\n\
        name of the default print queue.  Used only to\n\
        display in the print dialog, and has no effect on\n\
        printing.\n\
\n\
    nedit.printDefaultHost: (system specific) -- The\n\
        node name of the default print host.  Used only\n\
        to display in the print dialog, and has no effect\n\
        on printing.\n\
\n\
    nedit.multiClickTime: (system specific) -- Maximum\n\
        time in milliseconds allowed between mouse clicks\n\
        within double and triple click actions.\n\
\n\
    nedit*scrollBarPlacement: BOTTOM_LEFT -- How scroll\n\
        bars are placed in NEdit windows, as well as\n\
        various lists and text fields in the program.\n\
        Other choices are: BOTTOM_RIGHT, TOP_LEFT, or\n\
        TOP_RIGHT.\n\
\n\
    nedit*text.heavyCursor: False -- For monitors with\n\
        poor resolution or users who have difficulty\n\
        seeing the cursor, makes the cursor in the text\n\
        editing area of the window heavier and darker.\n\
\n\
    nedit*text.foreground: black -- Foreground color of\n\
        the text editing area of the NEdit window.\n\
\n\
    nedit*text.background: white -- Background color of\n\
        the text editing area of the NEdit window.\n\
\n\
    nedit*text.selectForeground: black -- Foreground\n\
    	(text) color for selections in the text editing\n\
    	area of the NEdit window.\n\
\n\
    nedit*text.selectBackground: gray80 -- Color for\n\
    	selections in the text editing area of the NEdit\n\
    	window.\n\
\n\
    nedit*text.highlightForeground: white -- Foreground\n\
    	(text) color for highlights (parenthesis\n\
    	flashing) in the text editing area of the NEdit\n\
    	window.\n\
\n\
    nedit*text.highlightBackground: red -- Color for\n\
    	highlights (parenthesis flashing) in the text\n\
    	editing area of the NEdit window.\n\
\n\
    nedit*text.cursorForeground: black -- Color for\n\
    	text cursor in the text editing area of the\n\
    	NEdit window.\n\
\n\
    nedit*text.blinkRate: 600 -- Blink rate of the text\n\
        insertion cursor in milliseconds.  Set to zero\n\
        to stop blinking.\n\
\n\
    nedit*text.Translations: -- Modifies key bindings\n\
        (see below).\n\
\n\
    nedit*statsLine.foreground: black -- Foreground\n\
        color of the statistics line area of the NEdit\n\
        window.\n\
\n\
    nedit*statsLine.background: gray70 -- Background\n\
        color of the statistics line area of the NEdit\n\
        window.\n\
\n\
    nc.autoStart: False -- Whether the nc program should\n\
        automatically start an NEdit server (without\n\
        prompting the user) if an appropriate server is\n\
        not found.\n\
\n\
    nc.serverCommand: nedit -server -- Command used by\n\
        the nc program to start an NEdit server.\n\
\n\
\n\
Binding Keys to Actions\n\
\n\
There are several ways to change key bindings in NEdit.  The easiest \
way to add a new key binding in NEdit is to define a macro in the Macro \
Commands dialog in the Default Settings sub-menu of the Preferences menu.  \
However, if you want to change existing bindings or add a significant \
number of new key bindings you will need to do so via X resources.  The \
methods for changing menu accelerator keys is different from that for \
general key binding via translation tables.  The Help topic \"Action \
Routines\" lists the actions available to be bound.\n\
\n\
Key Binding Via Translations \n\
\n\
The most general way to bind actions to keys in NEdit is to use the \
translation table associated with the text widget.  To add a binding to \
Alt+Y to insert the string \"Hi!\", for example, add lines similar to the \
following to your X resource file:\n\
\n\
  NEdit*text.Translations: #override \\n\\\n\
    Alt<Key>y: insert-string(\"Hi!\") \\n\n\
\n\
Unfortunately, the syntax for translation tables is not simple and is \
not covered completely here.  \
You will need to refer to a book on the X window system for the exact \
syntax for translation tables.  Translation tables map key and mouse presses, \
window operations, and other kinds of events to actions.  The syntax \
(somewhat oversimplified) is a keyword; #override, #augment, or #replace; \
followed by lines (separated by newline characters) pairing events with \
actions.  Events begin with modifiers, like Ctrl, Shift, or Alt, \
followed by the event type in <>.  BtnDown, Btn1Down, Btn2Down, Btn1Up, \
Key, KeyUp are valid event types.  For key presses, the \
event type is followed by the name of the key.  You can specify a \
combination of events, such as a sequence of key presses, by separating \
them with commas.  The other half of the event/action pair is a set \
of actions.  These are separated from the event specification by a colon \
and from each other by spaces.  Actions \
are names followed by parentheses, optionally \
containing one or more parameters separated by comas.\n\
\n\
Changing Menu Accelerator Keys\n\
\n\
The menu shortcut keys shown at the right of NEdit menu items can also \
be changed via X resources.  Each menu item has two \
resources associated with it, accelerator, the \
event to trigger the menu item; and acceleratorText, the string \
shown in the menu.  \
The form of the accelerator resource is the same as events for translation \
table entries discussed above, though multiple keys and other subtleties \
are not allowed.  \
The resource name for a menu is the title in lower case, followed by \
\"Menu\", the resource name of menu item is the name in lower case, run \
together, with words separated by caps, and all punctuation removed.  \
For example, to change Cut to Ctrl+X, \
you would add the following to your .Xdefaults file:\n\
\n\
    nedit*editMenu.cut.accelerator: Ctrl<Key>x\n\
    nedit*editMenu.cut.acceleratorText: Ctrl+X\n\
\n\
Accelerator keys with optional shift key modifiers, like Find..., have an \
additional accelerator resource with Shift appended to the name.  For \
example:\n\
\n\
    nedit*searchMenu.find.acceleratorText: [Shift]Alt+F\n\
    nedit*searchMenu.find.accelerator: Alt<Key>f\n\
    nedit*searchMenu.findShift.accelerator: Shift Alt<Key>f",

"Learn/Replay\n\
\n\
Selecting Learn Keystrokes from the Macro menu puts NEdit in learn mode.  \
In learn mode, keystrokes and menu commands are recorded, to be played \
back later, using the Replay Keystrokes command, or pasted into a \
macro in the Macro Commands dialog of the Default Settings menu in \
Preferences. \n\
\n\
Note that only keyboard and menu commands are recorded, not mouse clicks \
or mouse movements since these have no absolute point of reference, such \
as cursor or selection position.  When you do a mouse-based operation in \
learn mode, NEdit will beep (repeatedly) to remind you that the operation \
was not recorded. \n\
\n\
\n\
Macros\n\
\n\
NEdit 4.0 has a very limited ability to process macros.  Macro \
commands can use all of NEdit's editing functionality, available from \
either the keyboard or menus, however, there is not yet a macro \
\"language\".  That is, you can't use branching or looping, and can't \
set or test variables.\n\
\n\
The way to accomplish more complex editing tasks, is still to use the \
Shell Commands dialog to attach programs written in awk, sed, Perl, \
etc.\n\
\n\
As they stand, macros are useful for saving Learn/Replay sequences, \
for limited key-binding, and, surprisingly, many very complex tasks \
can be coded in terms of regular expression substitutions.\n\
\n\
Key binding via macros is simpler than any other method available in \
NEdit, because you don't have to deal \
with X resources or translation table syntax.  However, the only way \
to change menu accelerator keys is through X resources, and if you \
try to do a lot key binding via the Macro Commands dialog, you will \
quickly end up with a very large Macro menu. \n\
\n\
A macro in this version of NEdit is simply a list of newline-separated \
actions.  NEdit action routines are listed in the Action Routines section \
of the Help menu.  The easiest way to write a macro, is to record \
keystrokes and menu \
choices in learn mode, then paste the macro in the Macro Commands \
dialog, using the Paste Learn/Replay Macro button.",

"All of the editing capabilities of NEdit are represented as a special type of \
subroutine, called an action routine, which can be invoked from both \
macros and translation table entries (see \"Binding Keys to Actions\" \
in the X Resources section of the Help menu).\n\
\n\
\n\
Actions Representing Menu Commands:\n\
\n\
    File Menu		      Search Menu\n\
    ---------------------     -----------------------\n\
    new()		      find()\n\
    open()		      find-dialog()\n\
    open-dialog()	      find-same()\n\
    open-selected()	      find-selection()\n\
    close()		      replace()\n\
    save()		      replace-dialog()\n\
    save-as()		      replace-all()\n\
    save-as-dialog()	      replace-in-selection()\n\
    revert-to-saved()	      replace-same()\n\
    include-file()	      goto-line-number()\n\
    include-file-dialog ()    goto-line-number-dialog()\n\
    load-tags-file()	      goto-selected()\n\
    load-tags-file-dialog()   mark()\n\
    print()		      mark-dialog()\n\
    print-selection()	      goto-mark()\n\
    exit()		      goto-mark-dialog()\n\
    			      match()\n\
    			      find-definition()\n\
    			      split-window()\n\
    Edit Menu		      close-pane()\n\
    ---------------------\n\
    undo()		      Shell Menu\n\
    redo()		      -----------------------\n\
    clear()		      filter-selection-dialog()\n\
    select-all()	      filter-selection()\n\
    shift-left()	      execute-command()\n\
    shift-left-by-tab()	      execute-command-dialog()\n\
    shift-right()	      execute-command-line()\n\
    shift-right-by-tab()      shell-menu-command()\n\
    capitalize()\n\
    lowercase()		      Macro Menu\n\
    fill-paragraph()	      -----------------------\n\
    control-code-dialog()     macro-menu-command()\n\
\n\
The actions representing menu commands are named the same as the menu item \
with \
punctuation removed, all lower case, and dashes replacing spaces.  Without the \
-dialog suffix, commands which normally prompt the user for information, \
instead take the information from the routine's arguments (see below).  To \
present a dialog and ask the user for input, rather than supplying it in via \
arguments, use the actions with the -dialog suffix.\n\
\n\
Menu Action Routine Arguments:\n\
\n\
Arguments are text strings enclosed in quotes.  Below are the menu action \
routines which take arguments.  Optional arguments are inclosed in [].\n\
\n\
  open(filename)\n\
\n\
  save-as(filename)\n\
\n\
  include(filename)\n\
\n\
  load-tags-file(filename)\n\
\n\
  find-dialog([search-direction])\n\
\n\
  find(search-string [, search-direction], [search-type])\n\
\n\
  find-same([search-direction])\n\
\n\
  find-selection([search-direction])\n\
\n\
  replace-dialog([search-direction])\n\
\n\
  replace(search-string, replace-string,\n\
	  [, search-direction] [, search-type])\n\
\n\
  replace-in-selection(search-string, replace-string\n\
	  [, search-type])\n\
\n\
  replace-same([search-direction])\n\
\n\
  goto-line-number([line-number])\n\
\n\
  mark(mark-letter)\n\
\n\
  goto-mark(mark-letter)\n\
\n\
  filter-selection(shell-command)\n\
\n\
  execute-command(shell-command)\n\
\n\
  shell-menu-command(shell-menu-item-name)\n\
\n\
  macro-menu-command(macro-menu-item-name)\n\
\n\
Some notes on argument types above:\n\
\n\
  filename	    Path names are interpreted relative\n\
 		    to the directory from which NEdit was\n\
 		    started, wildcards and ~ are not\n\
 		    expanded.\n\
 		    \n\
  search-direction  Either \"forward\" or \"backward\"\n\
\n\
  search-type	    Either \"literal\", \"case\", or \"regex\"\n\
\n\
  mark-letter	    The mark command limits users to\n\
 		    single letters.  Inside of macros,\n\
 		    numeric marks are allowed, which won't\n\
 		    interfere with marks set by the user.\n\
\n\
  (macro or shell)  Name of the command exactly as\n\
  -menu-item-name   specified in the Macro Commands or\n\
 		    Shell Commands dialogs\n\
\n\
\n\
Keyboard-Only Actions\n\
\n\
backward-character()\n\
Moves the cursor one character to the left.\n\
\n\
backward-paragraph()\n\
Moves the cursor to the beginning of the paragraph, or if the \
cursor is already at the beginning of a paragraph, moves the cursor \
to the beginning of the previous paragraph.  Paragraphs are \
defined as regions of text delimited by one or more blank lines.\n\
\n\
backward-word()\n\
Moves the cursor to the beginning of a word, or, if the \
cursor is already at the beginning of a word, moves the \
cursor to the beginning of the previous word.  Word delimiters \
are user-settable, and defined by the X resource wordDelimiters.\n\
\n\
beginning-of-file()\n\
Moves the cursor to the beginning of the file.\n\
\n\
beginning-of-line()\n\
Moves the cursor to the beginning of the line.\n\
\n\
beginning-of-selection()\n\
Moves the cursor to the beginning of the selection \
without disturbing the selection.\n\
\n\
copy-clipboard()\n\
Copies the current selection to the clipboard.\n\
\n\
copy-primary()\n\
Copies the primary selection to the cursor.\n\
\n\
copy-to()\n\
If a secondary selection exists, copies the secondary selection to the \
cursor.  If no secondary selection exists, copies the primary \
selection to the \
pointer location.\n\
\n\
copy-to-or-end-drag()\n\
Completes either a secondary selection operation, or a primary \
drag.  If the user is dragging the mouse to adjust a secondary selection, \
the selection is copied and either inserted at the cursor location, \
or, if pending-delete is on and a primary selection exists in the window, \
replaces the primary selection.  If the user is dragging a block of \
text (primary selection), completes the drag operation and leaves the \
text at it's current location.\n\
\n\
cut-clipboard()\n\
Deletes the text in the primary selection and places it in the clipboard.\n\
\n\
cut-primary()\n\
Copies the primary selection to the cursor and deletes it \
at its original location.\n\
\n\
delete-selection()\n\
Deletes the contents of the primary selection.\n\
\n\
delete-next-character()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the character following the cursor.\n\
\n\
delete-previous-character()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the character before the cursor.\n\
\n\
delete-next-word()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the word following the cursor.\n\
\n\
delete-previous-word()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the word before the cursor.\n\
\n\
delete-to-start-of-line()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the characters between the cursor \
and the start of the line.\n\
\n\
delete-to-end-of-line()\n\
If a primary selection exists, deletes its contents.  Otherwise, \
deletes the characters between the cursor \
and the end of the line.\n\
\n\
deselect-all()\n\
De-selects the primary selection.\n\
\n\
end-of-file()\n\
Moves the cursor to the end of the file.\n\
\n\
end-of-line()\n\
Moves the cursor to the end of the line.\n\
\n\
end-of-selection()\n\
Moves the cursor to the end of the selection \
without disturbing the selection.\n\
\n\
exchange()\n\
Exchange the primary and secondary selections.\n\
\n\
extend-adjust()\n\
Attached mouse-movement events to begin a selection between the \
cursor and the mouse, or extend the primary selection to the \
mouse position.\n\
\n\
extend-end()\n\
Completes a primary drag-selection operation.\n\
\n\
extend-start()\n\
Begins a selection between the \
cursor and the mouse.  A drag-selection operation can be started with \
either extend-start or grab-focus.\n\
\n\
forward-character()\n\
Moves the cursor one character to the right.\n\
\n\
forward-paragraph()\n\
Moves the cursor to the beginning of the next paragraph.  Paragraphs are \
defined as regions of text delimited by one or more blank lines.\n\
\n\
forward-word()\n\
Moves the cursor to the beginning of the next word.  Word delimiters \
are user-settable, and defined by the X resource wordDelimiters.\n\
\n\
grab-focus()\n\
Moves the cursor to the mouse pointer location, and prepares for \
a possible drag-selection operation (bound to extend-adjust), or \
multi-click operation (a further grab-focus action).  If a second \
invocation of grab focus follows immediately, it selects a whole word, \
or a third, a whole line.\n\
\n\
insert-string(\"string\") \n\
If pending delete is on and the cursor is inside the selection, replaces \
the selection with \"string\".  Otherwise, inserts \"string\" at the \
cursor location.\n\
\n\
key-select(\"direction\")\n\
Moves the cursor one character in \
\"direction\" (\"left\", \"right\", \"up\", or \"down\") and extends the \
selection.  Same as forward/backward-character(\"extend\"), or \
process-up/down(\"extend\"), for compatibility with previous versions.\n\
\n\
move-destination()\n\
Moves the cursor to the pointer location without disturbing the selection.  \
(This is an unusual way of working.  We left it in for compatibility with \
previous versions, but if you actually use this capability, please send us some \
mail, otherwise it is likely to disappear in the future.\n\
\n\
move-to()\n\
If a secondary selection exists, deletes the contents of the secondary \
selection and inserts it at the cursor, or if pending-delete is on and there is \
a primary selection, replaces the primary selection.  If no secondary selection \
exists, moves the primary selection to the pointer location, deleting it \
from its original position.\n\
\n\
move-to-or-end-drag()\n\
Completes either a secondary selection operation, or a primary \
drag.  If the user is dragging the mouse to adjust a secondary selection, \
the selection is deleted and either inserted at the cursor location, \
or, if pending-delete is on and a primary selection exists in the window, \
replaces the primary selection.  If the user is dragging a block of \
text (primary selection), completes the drag operation and deletes the \
text from it's current location.\n\
\n\
newline()\n\
Inserts a newline character.  If Auto Indent is on, lines up the indentation \
of the cursor with the current line.\n\
\n\
newline-and-indent()\n\
Inserts a newline character and lines up the indentation \
of the cursor with the current line, regardless of the setting of Auto Indent.\n\
\n\
newline-no-indent()\n\
Inserts a newline character, without automatic indentation, regardless of \
the setting of Auto Indent.\n\
\n\
next-page()\n\
Moves the cursor and scroll forward one page.\n\
\n\
page-left()\n\
Move the cursor and scroll left one page.\n\
\n\
page-right()\n\
Move the cursor and scroll right one page.\n\
\n\
paste-clipboard()\n\
Insert the contents of the clipboard at the cursor, or if pending delete \
is on, replace the primary selection with the contents of the clipboard.\n\
\n\
previous-page()\n\
Moves the cursor and scroll backward one page.\n\
\n\
process-bdrag()\n\
Same as secondary-or-drag-start for compatibility with previous versions.\n\
\n\
process-cancel()\n\
Cancels the current extend-adjust, secondary-adjust, or \
secondary-or-drag-adjust in progress.\n\
\n\
process-down()\n\
Moves the cursor down one line.\n\
\n\
process-return()\n\
Same as newline for compatibility with previous versions.\n\
\n\
process-shift-down()\n\
Same as process-down(\"extend\") for compatibility with previous versions.\n\
\n\
process-shift-up()\n\
Same as process-up(\"extend\") for compatibility with previous versions.\n\
\n\
process-tab()\n\
If tab emulation is turned on, inserts an emulated tab, otherwise inserts \
a tab character.\n\
\n\
process-up()\n\
Moves the cursor up one line.\n\
\n\
scroll-down(nLines)\n\
Scroll the display down (towards the end of the file) by nLines.\n\
\n\
scroll-up(nLines)\n\
Scroll the display up (towards the beginning of the file) by nLines.\n\
\n\
scroll-to-line(lineNum)\n\
Scroll to position line number lineNum at the top of the \
pane.  The first line of a file is line 1.\n\
\n\
secondary-adjust()\n\
Attached mouse-movement events to extend the secondary selection to the \
mouse position.\n\
\n\
secondary-or-drag-adjust()\n\
Attached mouse-movement events to extend the secondary selection, or \
reposition the primary text being dragged.  Takes two optional arguments, \
\"copy\", and \"overlay\".  \"copy\" leaves a copy of the dragged text \
at the site at which the drag began.  \"overlay\" does the drag in overlay \
mode, meaning the dragged text is laid on top of the existing text, \
obscuring and ultimately deleteing it when the drag is complete.\n\
\n\
secondary-or-drag-start()\n\
To be attached to a mouse down event.  Begins drag selecting a secondary \
selection, or dragging the contents of the primary selection, depending on \
whether the mouse is pressed inside of an existing primary selection.\n\
\n\
secondary-start()\n\
To be attached to a mouse down event.  Begin drag selecting a secondary \
selection.\n\
\n\
select-all()\n\
Select the entire file.\n\
\n\
self-insert()\n\
To be attached to a key-press event, inserts the character equivalent \
of the key pressed.\n\
\n\
Arguments to Keyboard Action Routines \n\
\n\
In addition to the arguments listed in the call descriptions, any routine \
involving cursor movement can take the argument \"extend\", meaning, adjust \
the primary selection to the new cursor position.  Routines which take \
the \"extend\" argument as well as mouse dragging operations for both \
primary and secondary selections can take the optional keyword \"rect\", \
meaning, make the selection rectangular.",

"SOLUTIONS TO COMMON PROBLEMS\n\
\n\
P: No files are shown in the \"Files\" list in the Open... dialog.\n\
S: When you use the \"Filter\" field, include the file specification or a \
complete directory specification, including the trailing \"/\" on Unix.  \
(See Help in the Open... dialog).\n\
\n\
P: Keyboard shortcuts for menu items don't work.\n\
S: Be sure the Caps Lock and Num Lock keys are both unlocked.  In Motif \
programs, these keys prevent the menu accelerators from working.\n\
\n\
P: Find Same and Replace Same don't continue in the same direction \
as the original Find or Replace.\n\
S: Find Same and Replace Same don't use the direction of the original \
search.  The Shift key controls the direction: Ctrl+G means \
forward, Shift+Ctrl+G means backward.\n\
\n\
P: Preferences specified in the Preferences menu don't seem to get saved \
when I select Save Defaults.\n\
S: NEdit has two kinds of preferences: 1) per-window preferences, in the \
Preferences menu, and 2) default settings for preferences in newly created \
windows, in the Default \
Settings sub-menu of the Preferences menu.  Per-window preferences are not \
saved by Save Defaults, only Default Settings.\n\
\n\
P: Columns and indentation don't line up.\n\
S: NEdit is using a proportional width font.  Set the font to a fixed style \
(see Preferences).\n\
\n\
P: NEdit performs poorly on very large files.\n\
S: Turn off Incremental Backup.  With Incremental Backup on, NEdit \
periodically writes a full copy of the file to disk.\n\
\n\
P: Commands added to the Shell Commands menu (Unix only) don't output \
anything until they are finished executing.\n\
S: If the command output is \
directed to a dialog, or the input is from a selection, output is collected \
together and held until the command completes.  De-select both of the options \
and the output will be shown incrementally as the command executes.\n\
\n\
P: Dialogs don't automatically get keyboard focus when they pop up.\n\
S: Most X Window managers allow you to choose between two categories of \
keyboard focus models: pointer focus, and explicit focus.  \
Pointer focus means that as you move the mouse around the screen, the window \
under the mouse automatically gets the keyboard focus.  NEdit users who use \
this focus model should set \"Popups Under Pointer\" in the Default Settings \
sub menu of the preferences menu in NEdit.  Users with the explicit \
focus model, in some cases, may have problems with certain dialogs, such as \
Find and Replace.  In MWM this is caused by the mwm resource startupKeyFocus \
being set to False (generally a bad choice for explicit focus users).  \
NCDwm users should use the focus model \"click\" \
instead of \"explicit\", again, unless you have set it that way to correct \
specific problems, this is the appropriate setting for most \
explicit focus users.\n\
\n\
P: The Delete key doesn't forward-delete.\n\
S: See the X Resources section on nedit.remapDeleteKey.\n\
\n\
\n\
KNOWN BUGS\n\
\n\
Below is the list of known bugs which affect NEdit.  The bugs your copy \
of NEdit will exhibit depend on which system you are running and with which \
Motif libraries it was built. Note that there are now Motif 1.2 / X11R5 \
libraries available on ALL supported platforms, and as you can see below \
there are far fewer bugs in Motif 1.2, so it is in your best interest to \
upgrade your system.\n\
\n\
All Versions\n\
\n\
BUG: Operations between rectangular selections on overlapping lines \
do nothing.\n\
WORKAROUND: None.  These operations are \
very complicated and rarely used, maybe next version...\n\
\n\
BUG: Cut and Paste menu items fail, or possibly crash, for very large \
(multi-megabyte) selections.\n\
WORKAROUND: Use selection copy (middle mouse button click) for transferring \
larger quantities of data.  Cut and Paste save the copied text in server \
memory, which is usually limited.\n\
\n\
Motif 1.1 Versions\n\
\n\
BUG: The shortcut method for entering control characters (Alt+Ctrl+char) is \
not available.\n\
WORKAROUND: Use the Ins. Control Char command.\n\
\n\
BUG: Pop-up dialogs \"jump\" (appear briefly in a different location) when \
they are first invoked.\n\
WORKAROUND: Turn off \"Popups Under Pointer\" if this gives you a headache.\n\
\n\
VMS Versions\n\
\n\
BUGS: 1. Abbreviation of command line qualifiers is not allowed.  2. \
Error messages for mistakes on the command line don't make sense.\n\
WORKAROUND: NEdit does not parse its command line with the standard DCL \
command parser, instead, it superficially converts the command line to \
a Unix-style command line before processing it.  Because information \
is lost, NEdit may not always be able to \
distinguish between items that are supposed to be qualifiers and those \
which are supposed to be files and arguments to the qualifiers.  \
However, correct VMS command lines are always processed correctly, and only \
certain types of errors will appear to give strange results.\n\
\n\
BUG: Protection settings for new versions of files produced by NEdit over \
DECNET connections may revert to defaults.\n\
WORKAROUND: Check and reset protections when accessing files via DECNET.\n\
\n\
IBM Versions built with Motif 1.1\n\
\n\
BUG: The Command to set the font for the current window is not \
available.\n\
WORKAROUND: Use the Default Settings sub-menu to set the default \
font and then create a new window.\n\
\n\
BUG: NEdit can occasionally crash on window closing.\n\
WORKAROUND: Save files frequently, see Crash Recovery.\n\
\n\
\n\
Send questions and comments to: nedit_support@fnal.gov.",

"There are now two separate mailing lists for NEdit users. \
nedit_discuss, as the name implies, is for open discussion \
among NEdit users.  nedit_announces intended to be \
a very low volume mailing list for announcement of new versions, new \
executables, and significant contributed software.\n\
\n\
To subscribe to nedit_discuss, send a message containing the following \
line in the body of the message (not the subject) to mailserv@fnal.gov:\n\
\n\
    subscribe nedit_discuss\n\
\n\
To subscribe to nedit_announce, send a separate message to \
mailserv@fnal.gov containing the line:\n\
\n\
    subscribe nedit_announce\n\
\n\
To unsubscribe, send:\n\
\n\
    unsubscribe nedit_discuss (or nedit_announce)\n\
\n\
After subscribing, you will receive copies of all of the email submitted \
to the list.  You may submit mail to the discussion list by sending it to:\n\
\n\
    nedit_discuss@fnal.gov\n\
\n\
Users are allowed to post to nedit_announce as well (just make sure that \
the content is appropriate).",

"INFORMATION/LICENSE AGREEMENT FOR NEDIT.\n\
\n\
FermiTools Software Legal Information - November 1, 1996\n\
\n\
COPYRIGHT STATUS:  Fermi National Accelerator Laboratory \
(FNAL) documents are sponsored by the U.S. Department of \
Energy under Contract No. DE-AC02-76CH03000.  Therefore, the \
U.S. Government retains a non-exclusive, royalty-free license \
to publish or reproduce these documents or software for U.S. \
Government purposes.  All documents and software available \
from FNAL are protected under the U.S. and Foreign Copyright \
Laws, and FNAL reserves all rights.\n\
\n\
Terms and Conditions\n\
\n\
When a User distributes or otherwise obtains a software \
package included in the Fermilab Software Tools Program, the \
user agrees to abide by the Terms and Conditions of the \
Program below:\n\
\n\
�       Any redistribution of the software shall be accompanied \
by this INFORMATION/LICENSE AGREEMENT and the product's \
ORIGIN STATEMENT (below).\n\
\n\
�       The User shall acknowledge the origin of the software as \
set forth below:\n\
\n\
        \"This work was performed at Fermi National Accelerator \
Laboratory, operated by Universities Research Association, \
Inc., under contract DE-AC02-76CH03000 with the U.S. \
Department of Energy.\"\n\
\n\
�       The user is asked to feed back problems, benefits, \
and/or suggestions about the software to the Fermilab \
Software Providers and/or FermiTools management.\n\
\n\
�       Any distribution of this software shall be at no charge. \
To obtain a license to commercialize any of the software \
programs available from Fermilab including this software, \
contact FNAL's Office of Research and Technology \
Applications, P.O. Box 500, MS-200, Batavia, IL 60510-0500.\n\
\n\
\n\
INDEMNIFICATION BY USER OF THIRD PARTY CLAIMS AND \
DISCLOSURE OF LIABILITY\n\
\n\
The User, his/her directors, officers, employees, and \
agents hereby release and waive all claims against \
Universities Research Association, Inc. (URA) operator \
of Fermi National Accelerator Laboratory, its \
trustees, overseers, directors, officers, employees, \
agents, subcontractors, successors and assigns, for \
any and all liability and damages arising from the \
reproduction, use or other disposition of the \
software.  The User shall indemnify URA and the U.S. \
Government for all damages, costs or expenses, \
including attorney's fees, arising from the \
utilization of the software, including, but not \
limited to, the making, using, selling or exporting of \
products, processes or services derived from the \
Software.  The User agrees to indemnify, hold harmless \
and defend URA, its trustees, overseers, directors, \
officers, employees, agents, subcontractors, \
successors and assigns, against any and all liability, \
damage, loss, cost, charge, claim, demand, fee or \
expense of every nature and kind which may at any time \
hereafter, be sustained by URA by reason of claims of \
third parties arising out of alleged acts or omissions \
of the User in the reproduction, use or other \
disposition of the Software.\n\
\n\
The User agrees that URA, its trustees, overseers, \
directors, officers, employees, agents, \
subcontractors, successors and assigns shall not be \
liable under any claim, charge, or demand, whether in \
contract, tort, criminal law, or otherwise, for any \
and all loss, cost, charge, claim, demand, fee, \
expense, or damage of every nature and kind arising \
out of, connected with, resulting from or sustained as \
a result of the use of this software program.  In no \
event shall URA be liable for special, direct, \
indirect or consequential damages, losses, costs, \
charges, claims, demands, fees or expenses of any \
nature or kind.\n\
\n\
DISCLAIMER OF WARRANTIES\n\
\n\
The software is provided on an \"as is\" basis only. \
URA makes no representations, express or implied.  URA \
MAKES NO REPRESENTATIONS OR WARRANTIES OF \
MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE, \
or assumes any legal liability or responsibility for \
the accuracy, completeness, or usefulness of any \
information, apparatus, product or process disclosed, \
or represents that the Software will not infringe any \
privately held patent, copyright, or trademark.  The \
entire risk as to the results and the performance of \
this software is assumed by the User.\n\
\n\
DISCLAIMER OF ENDORSEMENT\n\
\n\
Reference herein to any specific commercial products, \
process, or service by tradename, trademark, \
manufacturer or otherwise, does not constitute or \
imply its endorsement, recommendation, or favoring by \
the United States Government, U.S. Department of \
Energy or URA.  The views and opinions of individuals \
expressed herein do not necessarily state or reflect \
those of the United States Government, U.S. Department \
of Energy or URA and shall not be used for advertising \
or product endorsement purposes.\n\
\n\
LIABILITIES OF THE GOVERNMENT\n\
\n\
This software is provided by URA, independent from its \
Prime Contract with the U.S. Department of Energy. \
URA is acting independently from the Government and in \
its own private capacity and is not acting on behalf \
of the U.S. Government, nor as its contractor nor its \
agent.  Correspondingly, it is understood and agreed \
that the U.S. Government has no connection to this \
software and in no manner whatsoever shall be liable \
for nor assume any responsibility or obligation for \
any claim, cost, or damages arising out of or \
resulting from the use of this software.\n\
\n\
\n\
ORIGIN STATEMENT\n\
\n\
Authors\n\
\n\
Mark Edel, Joy Kyriakopulos, Arnulfo Zepeda-Navratil, \
Suresh Ravoor, Donna Reid, Jeff Kallenbach\n\
\n\
Fermi National Accelerator Laboratory\n\
MS 234\n\
P.O.Box 500\n\
Batavia, IL 60510\n\
\n\
EMAIL: edel@fnal.gov\n\
\n\
Acknowledgement:\n\
\n\
Regular expression code by Henry Spencer\n\
\n\
Nedit incorporates an altered version of Henry \
Spencer's regcomp and regexec code adapted for NEdit. \
Original copyright notice:\n\
\n\
Copyright (c) 1986 by University of Toronto. \
Written by Henry Spencer.  Not derived from licensed \
software.\n\
\n\
Permission is granted to anyone to use this software \
for any purpose on any computer system, and to \
redistribute it freely, subject to the following \
restrictions:\n\
\n\
1. The author is not responsible for the consequences \
of use of this software, no matter how awful, even if \
they arise from defects in it.\n\
\n\
2. The origin of this software must not be \
misrepresented, either by explicit claim or by \
omission.\n\
\n\
3. Altered versions must be plainly marked as such, \
and must not be misrepresented as being the original \
software.\n\
\n\
End of INFORMATION/LICENSE AGREEMENT FOR NEDIT.",

"The Tabs dialog controls both the operation of the Tab \
key, and the interpretation of tab characters \
within a file.\n\
\n\
The first field, Tab Spacing, controls how  NEdit \
responds to tab characters in a file.  On most Unix and \
VMS systems the conventional interpretation of a tab \
character is to advance the text position to the nearest \
multiple of eight characters (a tab spacing of 8).  \
However, many programmers of C and other structured \
languages, when given the choice, prefer a tab spacing \
of 3 or 4 characters.  Setting a three or four character \
hardware tab spacing is useful and convenient as long as \
your other software tools support it.  Unfortunately, on \
Unix and VMS systems, system utilities, such as more, \
and printing software can't always properly display \
files with other than eight character tabs.\n\
\n\
Selecting \"Emulate Tabs\" will cause the Tab key to \
insert the correct number of spaces or tabs to reach the \
next tab stop, as if the tab spacing were set at the \
value in the \"Emulated tab spacing\" field.  \
Backspacing immediately after entering an emulated tab \
will delete it as a unit, but as soon as you move the \
cursor away from the spot, NEdit will forget that the \
collection of spaces and tabs is a tab, and will treat \
it as separate characters.  To enter a real tab \
character with \"Emulate Tabs\" turned on, use Ctrl+Tab.\n\
\n\
In generating emulated tabs, and in Shift Left, Paste \
Column, and some rectangular selection operations, NEdit \
inserts blank characters (spaces or tabs) to preserve \
the alignment of non-blank characters.  The bottom \
toggle button in the Tabs dialog instructs NEdit whether \
to insert tab characters as padding in such situations.  \
Turning this off, will keep NEdit from automatically \
inserting tabs.  Some software developers prefer to keep \
their source code free of tabs to avoid its \
misinterpretation on systems with different tab \
character conventions."
};

static Widget HelpWindows[NUM_TOPICS] = {NULL}; 

static Widget createHelpPanel(Widget parent, int topic);
static void dismissCB(Widget w, int topic, caddr_t call_data);

void Help(Widget parent, enum HelpTopic topic)
{
    if (HelpWindows[topic] != NULL)
    	XtDestroyWidget(HelpWindows[topic]);
    
    HelpWindows[topic] = createHelpPanel(parent, topic);
}

static Widget createHelpPanel(Widget parent, int topic)
{
    Arg al[50];
    int ac;
    Widget form, text, button;
    XmString st1;
    char *contents;
    
    contents = HelpText[topic];
    
    ac = 0;
    form = XmCreateFormDialog(parent, "helpForm", al, ac);

#ifndef MOTIF10
    ac = 0;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("Dismiss")); ac++;
    XtSetArg (al[ac], XmNtopAttachment, XmATTACH_NONE);  ac++;
    XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
    button = XmCreatePushButtonGadget(form, "dismiss", al, ac);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)dismissCB,
    	    (void *)topic);
    XmStringFree(st1);
    XtManageChild(button);
    SET_ONE_RSRC(form, XmNdefaultButton, button);
#endif
    
    ac = 0;
    XtSetArg(al[ac], XmNrows, 15);  ac++;
    XtSetArg(al[ac], XmNcolumns, 60);  ac++;
    XtSetArg(al[ac], XmNresizeHeight, False);  ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNwordWrap, True);  ac++;
    XtSetArg(al[ac], XmNscrollHorizontal, False);  ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);  ac++;
    XtSetArg(al[ac], XmNeditable, False);  ac++;
    XtSetArg(al[ac], XmNvalue, contents);  ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNspacing, 0);  ac++;
#ifndef MOTIF10
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET);  ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNbottomWidget, button);  ac++;
#endif
    text = XmCreateScrolledText(form, "helpText", al, ac);
    XtManageChild(text);
    
    SET_ONE_RSRC(XtParent(form), XmNtitle, HelpTitles[topic]);
    ManageDialogCenteredOnPointer(form);
    
    return XtParent(form);
}

static void dismissCB(Widget w, int topic, caddr_t call_data)
{
    XtDestroyWidget(XtParent(w));
    HelpWindows[topic] = NULL;
}

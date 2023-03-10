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
static char SCCSID[] = "@(#)help.c	1.16     3/15/94";
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PushBG.h>
#include "nedit.h"
#include "help.h"

static char *HelpTitles[NUM_TOPICS] = {
"Version",
"Getting Started",
"Finding & Replacing Text",
"Selecting Text",
"Cut & Paste",
"Features for Programming",
"Navigation from the Keyboard",
"Crash Recovery",
"Preferences",
"Shell Commands/Filters",
"Regular Expressions",
"NEdit Command Line",
"Customizing NEdit",
"Problems/Bugs",
"Mailing List"};

static char *HelpText[NUM_TOPICS] = {
"NEdit Version 3.0\n\
March 15, 1994\n\
\n\
Copyright (c) 1992, 1993, 1994\n\
Universities Research Association, Inc.\n\
All rights reserved.\n\
\n\
NEdit was written by Mark Edel, Joy Kyriakopulos, Arnulfo Zepeda-Navratil, \
Suresh Ravoor, Donna Reid, and Jeff Kallenbach, \
at Fermi National Accelerator Laboratory.\n\
\n\
The regular expression matching routines used in NEdit are adapted (with \
permission) from original code written by Henry Spencer at the \
University of Toronto.\n\
\n\
Send questions or comments to:\n\
\n\
    Mark Edel\n\
    edel@fnal.gov\n\
    Fermi National Accelerator Laboratory\n\
    P.O. Box 500\n\
    Batavia, IL 60148",

"Welcome to NEdit!\n\
\n\
If you have used other Motif programs before, you can probably use \
NEdit without much instruction.  Editor commands are \
available from the pulldown menus (File, Edit, \
Search, etc.).  If you make a mistake, select Undo from the Edit \
menu.\n\
\n\
Editing an Existing File\n\
\n\
To open an existing file, choose Open... from the file \
menu. Select the file that you want to open in the pop-up dialog that \
appears and click on OK.  You may open any number of files at the same time.  \
Each file will appear in its own editor window.  Using Open... \
rather than re-typing the NEdit command and running additional copies \
of NEdit, will give you quick access to all of the files you have open \
via the Windows menu, and ensure that you don't accidentally open the same \
file twice.  NEdit has no \"main\" window.  It remains running as \
long as at least one editor window is open.\n\
\n\
Creating a New File\n\
\n\
If you already have an empty (Untitled) window displayed, just begin typing \
in the window.  To create a new Untitled window, choose New from the File \
menu.  To give the file a name and save its contents to the disk, choose \
Save or Save As... from the File menu.\n\
\n\
Backup Files\n\
\n\
NEdit maintains periodic backups of the file you are editing so that you \
can recover the file in the event of a problem such as a system crash, \
network failure, or X server crash.  These files are saved under the name \
~filename (on Unix) or _filename (on VMS), where filename is the name of \
the file you were editing.  If \
an NEdit process is killed, some of these backup files may remain in \
your directory.  (To remove one of these files on Unix, you may have to prefix \
the ~ (tilde) character with a \\ (backslash) to prevent the shell from \
interpreting it as a special character.)\n\
\n\
Shortcuts\n\
\n\
As you become more familiar with NEdit, you can substitute the control \
and function keys shown on the right side of the menus for pulling down \
menus with the mouse.\n\
\n\
Dialogs are also streamlined so you can enter \
information quickly and without using the mouse*.  To move the keyboard \
focus around a dialog, use the tab and arrow keys.  One of the buttons \
in a dialog \
is usually drawn with a thick, indented, outline.  This button can be \
activated by pressing return or enter.  For example, to \
replace the string \"thing\" with \"things\" type:\n\
\n\
    <ctrl-r>thing<tab>things<return>\n\
\n\
To open a file named \"whole_earth.c\", type:\n\
\n\
    <ctrl-o>who<return>\n\
\n\
(how much of the filename you need to type depends \
on the other files in the directory).\n\
\n\
* Users who have set their keyboard focus mode to \"pointer\" must still \
move the mouse into the dialog before they begin typing.",

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
the reverse direction.\n\
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
search menu, type the string to search for and the string to substitute, and \
press the R. in Selection button in the dialog.  Note that selecting text \
in the Replace... dialog will unselect the text in the window.",

#ifdef MOTIF12
"To select text for copying, deleting, or replacing, press the left \
mouse button with the pointer at one end of the text you want to select, \
and drag it to the other end.  The text will become highlighted.  To \
select a whole word, double click (click twice quickly in succession).  \
Double clicking and then dragging the mouse will select a number of words.  \
Similarly, you can select \
a whole line or a number of lines by triple clicking or triple clicking and \
dragging. Clicking four times selects the entire contents of the window.  \
After releasing the mouse button, you can still adjust a selection \
by holding down the shift key and dragging on either end of the selection.  \
To delete the selected text, press delete or backspace.  To replace it, \
begin typing.\n\
\n\
Selected text can be dragged to a new location in the file, or to another \
window using the middle mouse button.  Holding the control key while \
dragging the text will copy the selected text, leaving the original \
text in place.\n\
\n\
Within a single window, the right mouse button can be used to make an \
additional selection (called the secondary selection).  As soon as the \
right mouse button is released, the contents of this selection will be \
copied to the current insert position.  If there is a (primary) selection, \
this new text will replace the selected text.  Holding the control key \
while making the secondary selection will delete the text rather than \
copying it.  Users of previous NEdit \
versions will note that the mouse button has changed, and that secondary \
operations between windows are no longer allowed.  Hopefully future versions \
of Motif will allow the return of this useful feature.",
#else
"To select text for copying, deleting, or replacing, press the left \
mouse button with the pointer at one end of the text you want to select, \
and drag it to the other end.  The text will become highlighted.  To \
select a whole word, double click (click twice quickly in succession).  \
Double clicking and then dragging the mouse will select a number of words.  \
Similarly, you can select \
a whole line or a number of lines by triple clicking or triple clicking and \
dragging. Clicking four times selects the entire contents of the window.  \
After releasing the mouse button, you can still adjust a selection \
by holding down the shift key and dragging on either end of the selection.  \
To delete the selected text, press delete or backspace.  To replace it, \
begin typing.\n\
\n\
Text selected with the middle mouse button is called a secondary selection \
(the previous paragraphs dealt with the primary selection).  The secondary \
selection is highlighted by underlining.  When you release the mouse button \
after making a secondary selection, NEdit immediately copies the selected \
text to the insert point.  Secondary selections are good for copying \
text to the place where you are currently typing, and for substituting text \
in the primary selection.",
#endif

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
to and from other Motif programs.\n\
\n\
There are actually two text insertion \
points, the blinking I-beam, and the caret.  The caret is the insert point for \
copied and pasted text, while the I-beam is destination for keyboard input.  \
Mostly the caret and the I-beam are in the same place and shown by just the \
blinking I-beam, \
but they can be moved independently.",

"Indentation\n\
\n\
With Auto Indent turned on (the default), NEdit keeps a running indent.  \
When you press the return key, \
space and tabs are inserted to line up the insert point under the start of \
the previous line.  Ctrl+Return in auto-indent mode acts like a normal \
return, With auto-indent turned off, Ctrl+Return does indentation.\n\
\n\
The Shift Left and Shift Right commands adjust the \
indentation for several lines at once.  To shift a block of text one character \
to the right, select the text, then choose Shift Right from the Edit menu.  \
Note that the accelerator keys for these menu items are Ctrl+9 and Ctrl+0, \
which correspond to  the right and left parenthesis on most keyboards.  \
Remember them as adjusting the text in the direction pointed to by the \
parenthesis character.  Holding the Shift key while selecting either \
Shift Left or Shift Right will shift the text by one full tab stop.\n\
\n\
Line Numbers\n\
\n\
To find a particular line in a source file by line number, choose Goto \
Line #... from the Search menu.  You can also directly select the line \
number text in the compiler message in the terminal emulator window \
(xterm, decterm, winterm, etc.) where you ran the compiler, and choose \
Goto Selected from the Search \
menu.  One exception to this is on AIXWindows (IBM) where the selection \
rules don't follow the current X standards.  On these \
systems, you will have to use Goto Line #... and retype the line number.\n\
\n\
To find out the line number of a particular line in your file, turn on \
Statistics Line in the Preferences menu and position the insertion point \
anywhere on the line.  The statistics line continuously updates the line \
number of the location of the insertion cursor.\n\
\n\
Matching Parentheses\n\
\n\
To help you inspect nested parentheses, brackets, braces, quotes, and other \
characters, NEdit has the \
Find Matching command.  To find the corresponding left parenthesis to match \
a right parenthesis, select or position the cursor after one of the \
parenthesis characters and choose Find Matching from the Search menu.  If \
the character matches itself, such as a quote or slash, select the first \
character of the pair.  NEdit will match {, (, [, <, \", \', `, /, and \\.\n\
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

"In addition to using the mouse and scroll bars to navigate through a file, \
you can use arrow keys and labeled function keys such as page up and page \
down.\n\
\n\
Holding down the control key while pressing a named key extends the scope \
of the action that it performs.  For example, Home normally moves the insert \
cursor the beginning of a line.  Ctrl+Home moves it to the beginning of \
the file. Backspace deletes one character, Ctrl+Backspace deletes one word.\n\
\n\
Holding down the shift key while pressing a named key begins or extends a \
selection.  Combining the shift and control keys combines their actions.  \
For example, to select a word without using the mouse, position the cursor \
at the beginning of the word and press Ctrl+Shift+RightArrow\n\
\n\
Under X and Motif, there are several levels of translation between keyboard \
keys and the actions they perform in a program.  Keyboards vary from machine \
to machine, and so do standards for the meaning of some keys, such as \
backspace and delete.  NEdit uses the same mapping of keys to program \
actions as other Motif programs that include text fields and editable \
text.  On most machines, these text editing actions are carefully matched \
to the labeling of arrow keys and other named keys, such as Insert, Home, \
Page-Up, etc..  If you prefer different key bindings, see the section titled \
Customization.",

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
the New or Open commands.  These settings can also be saved in a file \
that is automatically read by NEdit at startup time.\n\
\n\
    Auto Indent -- Maintain a running indent.  When\n\
        you press return, the insert point will line\n\
        up with the indent level of the previous line.\n\
\n\
    Periodic Backup -- Maintain a backup copy of the\n\
        file being edited under the name ~filename\n\
        on Unix or _filename on VMS (see Crash \n\
        Recovery).\n\
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
    Wrap Text -- Wrap text at the right margin rather\n\
        than allowing the text to extend beyond it.\n\
\n\
    Tab Distance -- Set the number of characters between\n\
        tab stops (individual tab stops are not setable).\n\
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
preferences menu under Default Settings.  NEdit comes pre-configured with \
a few useful Unix commands like spell and sort, however, the primary \
purpose for the menu is custom extensions.\n\
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
search and replace strings to find occurences of the \
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
parentheses.  Preceeding & or \\1-9 with \\U, \\u, \\L, or \\l adjusts the \
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
"nedit [-tags file] [-tabs n] [-wrap] [-nowrap] [-autosave]\n\
    [-noautosave] [-autoindent] [-noautoindent] [-rows n]\n\
    [-columns n] [-font font] [-xrm resourcestring]\n\
    [-display [host]:server[.screen] [-geometry geometry]\n\
    [file...]\n\
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
    -rows n -- Default width in characters for an editing\n\
        window.\n\
\n\
    -columns n -- Default height in characters for an\n\
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
        to override a default value (see Customizing NEdit).",
#else
"Command Format:\n\
\n\
    NEDIT [filespec[,...]]\n\
\n\
The following qualifiers are accepted:\n\
\n\
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
Unix-style command lines (but not file names) are also acceptable:\n\
\n\
    nedit -rows 20 -wrap file1.c file2.c\n\
\n\
is equivalent to:\n\
\n\
    nedit /rows=20/wrap file1.c, file2.c",
#endif /*VMS*/

"NEdit has additional options to those provided in the Preferences menu \
which are set using X resources.  \
Like most other X programs, NEdit can be customized to vastly unnecessary \
proportions, from initial window positions down to the font and shadow \
colors of each individual button (A complete discussion of how to do \
this is left to books on the X Windows System).  Key binding (see below) \
is one of the most useful of these resource settable options.\n\
\n\
X resources are usually specified in a file called .Xdefaults in your \
home directory (on VMS this is SYS$LOGIN).  On some systems, this file is \
read and its information \
attached to the X server (your screen) when you start X.  On other \
systems, the .Xdefaults file is read each time you run an X program.  \
When X defaults are attached to the server, you can use a program called \
xrdb to update them without restarting X.\n\
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
    nedit.printCommand: (system specific) -- Command used\n\
        by the print dialog to print a file, i.e. lp, lpr,\n\
        etc..\n\
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
    nedit*text.foreground: black -- Foreground color of\n\
        the text editing area of the NEdit window.\n\
\n\
    nedit*text.background: white -- Background color of\n\
        the text editing area of the NEdit window.\n\
\n\
    nedit*text.blinkRate: 600 -- Blink rate of the text\n\
        insertion cursor in milliseconds.  Set to zero\n\
        to stop blinking.\n\
\n\
    nedit*text.Translations: -- Modifies key bindings\n\
        (see below).\n\
\n\
Mapping Editor Commands to Keys\n\
\n\
One of the most useful customizations to NEdit is to change the bindings of \
editor actions to keys to fit a particular style of editing.  \
Appendix A of the NEdit user's \
manual lists the actions available in NEdit. \
To bind actions to keys, add lines similar to the following to your \
X resource file:\n\
\n\
  NEdit*text.Translations: #override \\\n\
    Ctrl<Key>v: insert-string(\"VEE!\") process-up()\\n\\\n\
    <Key>osfDelete: delete-previous-character()\\n\n\
\n\
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
and from eachother by spaces.  Actions \
are names followed by parentheses, optionally \
containing a single parameter.\n\
\n\
Changing Menu Accelerator Keys\n\
\n\
The menu shortcut keys shown at the right of NEdit menu items can also \
be changed via X resources.  Each menu item has two accelerator key \
resources associated with it, accelerator, and acceleratorText.  \
accelerator sets the actual event that triggers the menu item, and \
is in the same form as the events for translation table entries discussed \
in the previous section.  acceleratorText contains the entry that will \
be displayed with the menu item.  To construct the complete resource name, \
use one of *fileMenu, *editMenu, \
*preferencesMenu, *windowsMenu, or *shellMenu, followed by `.' and \
the name of the menu item (lower case, run together with words separated \
by caps and with all punctuation removed), followed by \
.accelerator, or .acceleratorText.  For example, to change Cut to Ctrl+X, \
you would add the following to your .Xdefaults file:\n\
\n\
    nedit*editMenu.cut.accelerator: Ctrl<Key>x\n\
    nedit*editMenu.cut.acceleratorText: Ctrl+X\n\
\n\
Accelerator keys with optional shift key modifiers, like Find..., have an \
additional .accelerator resource with Shift appended to the name.  To change \
the accelerator for the Find... command to Alt+F, you would use \
the following: \n\
\n\
   nedit*searchMenu.find.acceleratorText: [Shift]Alt+F\n\
   nedit*searchMenu.find.accelerator: Alt<Key>f\n\
   nedit*searchMenu.findShift.accelerator: Shift Alt<Key>f",

"Common Problems\n\
\n\
Columns and indentation don't line up -- \
NEdit is using a proportional width font.  Set the font to a fixed style \
(see Preferences).\n\
\n\
No files are shown in the \"Files\" list in the Open... dialog -- \
When you use the \"Filter\" field, include the file specification or a \
complete directory specification (including the trailing \"/\" on Unix, and \
the \"]\" on VMS.  See Help in the Open... dialog).\n\
\n\
Keyboard shortcuts for menu items don't work -- \
Be sure the Caps Lock and Num Lock keys are both unlocked.  In Motif programs, \
these keys prevent the menu accelerators from working.\n\
\n\
Commands added to the Shell Commands menu (Unix only) don't output \
anything until they are finished executing -- If the command output is \
directed to a dialog, or the input is from a selection, output is collected \
together and held until the command completes.  Deselect both of the options \
and the output will be shown incrementally as the command executes.\n\
\n\
Known Bugs\n\
\n\
Below is the list of known bugs which affect NEdit.  The bugs your copy \
of NEdit will exhibit depend on which system you are running and with which \
Motif libraries it was built.\n\
\n\
All Versions\n\
\n\
BUG: Scroll bars can get out of sync with text in a window.\n\
WORKAROUND: Drag the scroll bar to the top, then move or resize the window \
slightly. \n\
\n\
Motif 1.2 Versions\n\
\n\
BUG: Secondary operations between windows no longer work.\n\
WORKAROUND: None.  OSF may fix this problem in Motif 2.0\n\
\n\
Motif 1.1 Versions\n\
\n\
BUG: Deleting, cutting, pasting, or shifting a block of text larger than the \
window, or undoing Replace All does not appear to work, or displays \
strange or random characters.\n\
WORKAROUND: The change was made correctly to your file, but did not display \
properly on the screen.  Scroll the window up or down by one page, then scroll \
back to the changed area and it will display correctly.\n\
\n\
BUG: Statistics line statistics lag one character behind the last typed \
character.\n\
WORKAROUND: Use the mouse to position the insertion cursor \
to find a line or column number.\n\
\n\
BUG: In split-window mode, deleting characters in one pane cause another \
pane to scroll upward.\n\
WORKAROUND: scroll the other pane back down by hand.\n\
\n\
VMS Version\n\
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
IBM Versions Prior to Motif 1.2\n\
\n\
BUG: Commands to set font and wrap mode for the current window are not \
available.\n\
WORKAROUND: Use the Default Settings sub-menu to set the default \
font or wrap mode and then create a new window.\n\
\n\
BUG: NEdit can occasionally crash on window closing.\n\
WORKAROUND: Save files frequently, see Crash Recovery.",

"To subscribe to the NEdit mailing list, send mail to:\n\
\n\
    listproc@www.fnal.gov\n\
\n\
Including the following line in the body of the message:\n\
\n\
    subscribe nedit-list yourname\n\
\n\
Yourname, above, is your given name, not \
your account name.  Your address is taken from the address that you \
use to send the request, so be sure to subscribe from the machine at \
which you want to receive mail.\n\
\n\
After subscribing, you will receive copies of all of the email on the \
discussion list.  You may submit mail to the discussion list by \
sending it to:\n\
\n\
    nedit-list@www.fnal.gov\n\
\n\
Please note that you must subscribe to the mailing list before you \
may submit mail to it.\n\
\n\
To unsubscribe, send a message to listproc@www.fnal.gov including the \
line:\n\
\n\
    unsubscribe nedit-list\n\
\n\
Refer problems with the mailing list to:\n\
\n\
    hanson@fnal.fnal.gov" };

static Widget HelpWindows[NUM_TOPICS] =
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}; 

static Widget createHelpPanel(WindowInfo *window, int topic);
static void dismissCB(Widget w, int topic, caddr_t call_data);

void Help(WindowInfo *window, enum HelpTopic topic)
{
    if (HelpWindows[topic] != NULL)
    	XtDestroyWidget(HelpWindows[topic]);
    
    HelpWindows[topic] = createHelpPanel(window, topic);
}

static Widget createHelpPanel(WindowInfo *window, int topic)
{
    Arg al[50];
    int ac;
    Widget form, text, button;
    XmString st1;
    char *contents;
    
    contents = HelpText[topic];
    
    ac = 0;
    form = XmCreateFormDialog(window->shell, "helpForm", al, ac);

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
    XtManageChild(form);
    
    return XtParent(form);
}

static void dismissCB(Widget w, int topic, caddr_t call_data)
{
    XtDestroyWidget(XtParent(w));
    HelpWindows[topic] = NULL;
}

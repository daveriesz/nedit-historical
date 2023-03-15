$ !
$ ! DCL link procedure for NEdit
$ !
$ !	This file (and NEDIT_OPTIONS_FILE.OPT) need to be changed to avoid 
$ !	using shareable images for linking.
$ !
$ !
$ ! SCCSID: @(#)lnknedit.com	1.1     8/20/93
$ !
$!DEFINE LNK$LIBRARY SYS$LIBRARY:VAXCRTL
$ SET NOVERIFY
OBJS :=	nedit, file, menu, window, clipboard, search, undo, shift, -
	help, preferences, tags, regularExp
$ ON WARNING THEN EXIT
$ SET VERIFY
$ LINK 'OBJS', NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib

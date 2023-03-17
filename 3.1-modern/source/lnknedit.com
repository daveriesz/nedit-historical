$ !
$ ! DCL link procedure for NEdit
$ !
$ SET NOVERIFY
OBJS :=	nedit, file, menu, window, clipboard, search, undo, shift, -
	help, preferences, tags, regularExp
$ ON WARNING THEN EXIT
$ SET VERIFY
$ LINK 'OBJS', NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib

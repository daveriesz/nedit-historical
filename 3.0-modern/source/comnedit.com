$ !
$ ! VMS procedure to compile and link modules for NEdit
$ !
$ !
$ ! SCCSID: @(#)comnedit.com	1.1     8/19/93
$ !
$ SET NOVERIFY
$ ON ERROR THEN EXIT
$ ! COMPILE := CC/DEBUG/NOOPTIMIZE/OBJ=[.DBGOBJ]
$ COMPILE := CC
$ !
$ SET VERIFY
$ COMPILE clipboard.c
$ COMPILE file.c
$ COMPILE help.c
$ COMPILE menu.c
$ COMPILE nedit.c
$ COMPILE preferences.c
$ COMPILE regularExp.c
$ COMPILE search.c
$ COMPILE shift.c
$ COMPILE tags.c
$ COMPILE undo.c
$ COMPILE window.c
$ !
$ @LNKNEDIT

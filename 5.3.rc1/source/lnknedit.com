$ ! $Id: lnknedit.com,v 1.5.4.1 2002/04/24 20:40:10 edg Exp $
$ !
$ ! DCL link procedure for NEdit
$ !
$ ON ERROR THEN GOTO THE_END
$ VERIFY = 'F$VERIFY (1)'
$ OBJS :=	nedit, file, menu, window, selection, search, undo, shift, -
	help, preferences, tags, userCmds, regularExp, macro, text, -
	textSel, textDisp, textBuf, textDrag, server, highlight, -
        highlightData, interpret, parse, smartIndent, regexconvert, -
        rbTree, windowtitle, linkdate

$ LINK 'OBJS', NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib
$ LINK nc, NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib
$THE_END:
$ EXIT (F$VERIFY(VERIFY))

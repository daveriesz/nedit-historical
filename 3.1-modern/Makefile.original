#
# Makefile for NEdit text editor
#
# targets: sgi_nedit, hp_nedit, ultrix_nedit, ibm_nedit, sunos_nedit,
#	   solaris_nedit, osf_nedit, linux_nedit
#
# Builds an intermediate library in util directory, then builds
# the nedit executable in the source directory
#

sgi_nedit:
	(cd util; make -f Makefile.sgi libNUtil.a)
	(cd source; make -f Makefile.sgi nedit)

hp_nedit:
	(cd util; make -f Makefile.hp libNUtil.a)
	(cd source; make -f Makefile.hp nedit)

ultrix_nedit:
	(cd util; make -f Makefile.ultrix libNUtil.a)
	(cd source; make -f Makefile.ultrix nedit)

ibm_nedit:
	(cd util; make -f Makefile.ibm libNUtil.a)
	(cd source; make -f Makefile.ibm nedit)

sunos_nedit:
	(cd util; make -f Makefile.sunos libNUtil.a)
	(cd source; make -f Makefile.sunos nedit)

solaris_nedit:
	(cd util; make -f Makefile.solaris libNUtil.a)
	(cd source; make -f Makefile.solaris nedit)

osf_nedit:
	(cd util; make -f Makefile.osf libNUtil.a)
	(cd source; make -f Makefile.osf nedit)

linux_nedit:
	(cd util; make -f Makefile.linux libNUtil.a)
	(cd source; make -f Makefile.linux nedit)

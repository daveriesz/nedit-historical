# $Id: Makefile,v 1.11.2.5 2003/11/19 17:22:14 edg Exp $
SHELL=/bin/sh
#
# Makefile for NEdit text editor
#
# Targets are the suffixes of the system-specific makefiles in
# the makefiles/ directory.
# For example, to build NEdit for Solaris, give the command
#
#   make solaris
#
# This builds an intermediate library in the util/ directory,
# then builds the nedit and nc executables in the source/ directory.
#

all:
	@echo "Please specify target:"
	@echo "(For example, type \"make linux\" for a Linux system.)"
	@(cd makefiles && ls -C Makefile* | sed -e 's/Makefile.//g')

.DEFAULT:
	@- (cd util;   if [ -f ../makefiles/Makefile.$@ -a ! -f ./Makefile.$@ ];\
	   then ln -s ../makefiles/Makefile.$@ .; fi)
	@- (cd source; if [ -f ../makefiles/Makefile.$@ -a ! -f ./Makefile.$@ ];\
	   then ln -s ../makefiles/Makefile.$@ .; fi)
	(cd util; \
	    $(MAKE) -f Makefile.$@ verify_config && \
	    $(MAKE) -f Makefile.$@ libNUtil.a)
	(cd source; $(MAKE) -f Makefile.$@ nedit nc)

# This should not be in the default build, as users may not have Perl
# installed.  This is only interesting to developers.
docs:
	(cd doc; $(MAKE) all)

# We need a "dev-all" target that builds the docs plus binaries, but
# that doesn't work since we require the user to specify the target.  More
# thought is needed

clean:
	(cd util;   $(MAKE) -f Makefile.common clean)
	(cd source; $(MAKE) -f Makefile.common clean)

realclean: clean
	(cd doc;    $(MAKE) clean)

#
# The following is for creating binary packages of NEdit.
#
RELEASE=nedit-5.4-`uname -s`-`uname -p`
BINDIST-FILES=source/nedit source/nc README COPYRIGHT ReleaseNotes doc/nedit.doc doc/nedit.html doc/nedit.man doc/nc.man doc/faq.txt

dist-bin: $(BINDIST-FILES)
	rm -rf $(RELEASE)
	mkdir -p $(RELEASE)
	cp $(BINDIST-FILES) $(RELEASE)/
	strip $(RELEASE)/nedit $(RELEASE)/nc
	chmod 555 $(RELEASE)/nedit $(RELEASE)/nc
	tar cf $(RELEASE).tar $(RELEASE)
	compress -c $(RELEASE).tar > $(RELEASE).tar.Z
	-gzip -9 -c $(RELEASE).tar > $(RELEASE).tar.gz
	-bzip2 -9 -c $(RELEASE).tar > $(RELEASE).tar.bz2
	rm -rf $(RELEASE) $(RELEASE).tar

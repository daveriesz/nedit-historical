
all: util source

util: FORCE
	$(MAKE) -C util

source: FORCE
	$(MAKE) -C source

clean: utilclean sourceclean

utilclean:
	$(MAKE) -C util clean

sourceclean:
	$(MAKE) -C source clean

FORCE:


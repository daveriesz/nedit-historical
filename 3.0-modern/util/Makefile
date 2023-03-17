
LIBRARY = libNUtil.a

CSOURCES = DialogF.c fileUtils.c fontsel.c getfiles.c misc.c prefFile.c printUtils.c vmsUtils.c
COBJECTS = $(CSOURCES:.c=.o)

CFLAGS   = $(COPT) $(CDEF) $(CINC)
COPT     = -g -Werror
CDEF     = -DLINUX -DMOTIF12
CINC     = 

LDFLAGS  = $(LDOPT) $(LDDIR) $(LDLIB)
LDOPT    = 
LDDIR    = 
LDLIB    = 

all: $(LIBRARY)

$(LIBRARY): $(COBJECTS)
	$(AR) -urs $@ $^

clean: libclean objclean
libclean:
	$(RM) $(LIBRARY)
objclean:
	$(RM) $(COBJECTS)


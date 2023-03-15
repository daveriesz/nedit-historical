
PROGRAM = nedit

CSOURCES = clipboard.c file.c help.c menu.c nedit.c preferences.c regularExp.c search.c shell.c shift.c tags.c undo.c window.c
COBJECTS = $(CSOURCES:.c=.o)

CFLAGS   = $(COPT) $(CDEF) $(CINC)
COPT     = -g -Werror
CDEF     = -DLINUX -DMOTIF12 -DPREF_FILE_NAME="\".nedit-3.0\""
CINC     = -I../util

LDFLAGS  = $(LDOPT) $(LDDIR) $(LDLIB)
LDOPT    = 
LDDIR    = -L../util
LDLIB    = -lNUtil -lX11 -lXt -lXm

all: $(PROGRAM)

$(PROGRAM): $(COBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean: execlean objclean
execlean:
	$(RM) $(PROGRAM)
objclean:
	$(RM) $(COBJECTS)


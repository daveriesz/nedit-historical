/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
/* SCCS ID: regularExp.h 1.1 2/4/94 */
#define NSUBEXP  10
typedef struct regexp {
    char *startp[NSUBEXP];
    char *endp[NSUBEXP];
    char regstart;      /* Internal use only. */
    char reganch;       /* Internal use only. */
    char *regmust;      /* Internal use only. */
    int regmlen;        /* Internal use only. */
    char program[1];    /* Unwarranted chumminess with compiler. */
} regexp;

extern regexp *CompileRE();
extern int ExecRE();
extern void SubstituteRE();

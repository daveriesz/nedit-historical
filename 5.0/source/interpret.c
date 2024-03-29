#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#include <sys/param.h>
#endif /*VMS*/
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "textBuf.h"
#include "nedit.h"
#include "menu.h"
#include "text.h"
#include "interpret.h"

#define PROGRAM_SIZE  4096	/* Maximum program size */
#define MAX_ERR_MSG_LEN 256	/* Max. length for error messages */
#define LOOP_STACK_SIZE 200	/* (Approx.) Number of break/continue stmts
    	    	    	    	   allowed per program */
#define INSTRUCTION_LIMIT 100 	/* Number of instructions the interpreter is
    	    	    	    	   allowed to execute before preempting and
    	    	    	    	   returning to allow other things to run */

/* Temporary markers placed in a branch address location to designate
   which loop address (break or continue) the location needs */
#define NEEDS_BREAK (Inst)1
#define NEEDS_CONTINUE (Inst)2

#define N_ARGS_ARG_SYM -1   	/* special arg number meaning $n_args value */

enum opStatusCodes {STAT_OK=2, STAT_DONE, STAT_ERROR, STAT_PREEMPT};

static void addLoopAddr(Inst *addr);
static void saveContext(RestartData *context);
static void restoreContext(RestartData *context);
static int returnNoVal(void);
static int returnVal(void);
static int returnValOrNone(int valOnStack);
static int pushSymVal(void);
static int dupStack(void);
static int add(void);
static int subtract(void);
static int multiply(void);
static int divide(void);
static int modulo(void);
static int negate(void);
static int increment(void);
static int decrement(void);
static int gt(void);
static int lt(void);
static int ge(void);
static int le(void);
static int eq(void);
static int ne(void);
static int bitAnd(void);
static int bitOr(void);
static int and(void);
static int or(void);
static int not(void);
static int power(void);
static int concat(void);
static int assign(void);
static int callSubroutine(void);
static int fetchRetVal(void);
static int branch(void);
static int branchTrue(void);
static int branchFalse(void);
static int branchNever(void);
static void freeSymbolTable(Symbol *symTab);
static int errCheck(char *s);
static int execError(char *s1, char *s2);
static int stringToNum(char *string, int *number);
static void disasm(Program *prog, int nInstr);

/* Global symbols and function definitions */
static Symbol *GlobalSymList = NULL;

/* List of all memory allocated for strings */
static char *AllocatedStrings = NULL;

/* Message strings used in macros (so they don't get repeated every time
   the macros are used */
static char *StackOverflowMsg = "macro stack overflow";
static char *StackUnderflowMsg = "macro stack underflow";
static char *StringToNumberMsg = "string could not be converted to number";

/* Temporary global data for use while accumulating programs */
Symbol *LocalSymList = NULL;		 /* symbols local to the program */
static Inst Prog[PROGRAM_SIZE]; 	 /* the program */
static Inst *ProgP;			 /* next free spot for code gen. */
static Inst *LoopStack[LOOP_STACK_SIZE]; /* addresses of break, cont stmts */
static Inst **LoopStackPtr = LoopStack;  /*  to fill at the end of a loop */

/* Global data for the interpreter */
static DataValue *Stack;	    /* the stack */
static DataValue *StackP;	    /* next free spot on stack */
static DataValue *FrameP;   	    /* frame pointer (start of local variables
    	    	    	    	       for the current subroutine invocation) */
static Inst *PC;		    /* program counter during execution */
static char *ErrMsg;		    /* global for returning error messages
    	    	    	    	       from executing functions */
static WindowInfo *InitiatingWindow;/* window from which macro was run */
static WindowInfo *FocusWindow;	    /* window on which macro commands operate */
static int PreemptRequest;  	    /* passes preemption requests from called
    	    	    	    	       routines back up to the interpreter */

/* Array for mapping operations to functions for performing the operations
   Must correspond to the enum called "operations" in interpret.h */
static int (*OpFns[N_OPS])() = {returnNoVal, returnVal, pushSymVal, dupStack,
	add, subtract, multiply, divide, modulo, negate, increment, decrement,
	gt, lt, ge, le, eq, ne, bitAnd, bitOr, and, or, not, power, concat,
	assign, callSubroutine, fetchRetVal, branch, branchTrue, branchFalse,
	branchNever};

/*
** Initialize macro language global variables.  Must be called before
** any macros are even parsed, because the parser uses action routine
** symbols to comprehend hyphenated names.
*/
void InitMacroGlobals(void)
{
    XtActionsRec *actions;
    int i, nActions;
    static char argName[3] = "$x";
    static DataValue dv = {NO_TAG, {0}};

    /* Add action routines from NEdit menus and text widget */
    actions = GetMenuActions(&nActions);
    for (i=0; i<nActions; i++) {
    	dv.val.ptr = (void *)actions[i].proc;
    	InstallSymbol(actions[i].string, ACTION_ROUTINE_SYM, dv);
    }
    actions = TextGetActions(&nActions);
    for (i=0; i<nActions; i++) {
    	dv.val.ptr = (void *)actions[i].proc;
    	InstallSymbol(actions[i].string, ACTION_ROUTINE_SYM, dv);
    }
    
    /* Add subroutine argument symbols ($1, $2, ..., $9) */
    for (i=0; i<9; i++) {
	argName[1] = '1' + i;
	dv.val.n = i;
	InstallSymbol(argName, ARG_SYM, dv);
    }
    
    /* Add special symbol $n_args */
    dv.val.n = N_ARGS_ARG_SYM;
    InstallSymbol("$n_args", ARG_SYM, dv);
}

/*
** To build a program for the interpreter, call BeginCreatingProgram, to
** begin accumulating the program, followed by calls to AddOp, AddSym,
** and InstallSymbol to add symbols and operations.  When the new program
** is finished, collect the results with FinishCreatingProgram.  This returns
** a self contained program that can be run with ExecuteMacro.
*/

/*
** Start collecting instructions for a program. Clears the program
** and the symbol table.
*/
void BeginCreatingProgram(void)
{ 
    LocalSymList = NULL;
    ProgP = Prog;
    LoopStackPtr = LoopStack;
}

/*
** Finish up the program under construction, and return it (code and
** symbol table) as a package that ExecuteMacro can execute.  This
** program must be freed with FreeProgram.
*/
Program *FinishCreatingProgram(void)
{
    Program *newProg;
    int progLen, fpOffset = 0;
    Symbol *s;
    
    newProg = (Program *)XtMalloc(sizeof(Program));
    progLen = ((char *)ProgP) - ((char *)Prog);
    newProg->code = (Inst *)XtMalloc(progLen);
    memcpy(newProg->code, Prog, progLen);
    newProg->localSymList = LocalSymList;
    LocalSymList = NULL;
    
    /* Local variables' values are stored on the stack.  Here we assign
       frame pointer offsets to them. */
    for (s = newProg->localSymList; s != NULL; s = s->next)
	s->value.val.n = fpOffset++;
    
    /* disasm(newProg, ProgP - Prog); */
    
    return newProg;
}

void FreeProgram(Program *prog)
{
    freeSymbolTable(prog->localSymList);
    XtFree((char *)prog->code);
    XtFree((char *)prog);    
}

/*
** Add an operator (instruction) to the end of the current program
*/
int AddOp(int op, char **msg)
{
    if (ProgP >= &Prog[PROGRAM_SIZE]) {
	*msg = "macro too large";
	return 0;
    }
    *ProgP++ = OpFns[op];
    return 1;
}

/*
** Add a symbol operand to the current program
*/
int AddSym(Symbol *sym, char **msg)
{
    if (ProgP >= &Prog[PROGRAM_SIZE]) {
	*msg = "macro too large";
	return 0;
    }
    *ProgP++ = (Inst)sym;
    return 1;
}

/*
** Add an immediate value operand to the current program
*/
int AddImmediate(void *value, char **msg)
{
    if (ProgP >= &Prog[PROGRAM_SIZE]) {
	*msg = "macro too large";
	return 0;
    }
    *ProgP++ = (Inst)value;
    return 1;
}

/*
** Add a branch offset operand to the current program
*/
int AddBranchOffset(Inst *to, char **msg)
{
    if (ProgP >= &Prog[PROGRAM_SIZE]) {
	*msg = "macro too large";
	return 0;
    }
    *ProgP = (Inst)(to - ProgP);
    ProgP++;
    
    return 1;
}

/*
** Return the address at which the next instruction will be stored
*/
Inst *GetPC(void)
{
    return ProgP;
}

/*
** Swap the positions of two contiguous blocks of code.  The first block
** running between locations start and boundary, and the second between
** boundary and end.
*/
void SwapCode(Inst *start, Inst *boundary, Inst *end)
{
    char *temp;
    
    temp = XtMalloc((boundary - start) * sizeof(Inst*));
    memcpy(temp, start, (boundary-start) * sizeof(Inst*));
    memmove(start, boundary, (end-boundary) * sizeof(Inst*));
    memcpy(start+(end-boundary), temp, (boundary-start) * sizeof(Inst*));
    XtFree(temp);
}

/*
** Maintain a stack to save addresses of branch operations for break and
** continue statements, so they can be filled in once the information
** on where to branch is known.
**
** Call StartLoopAddrList at the beginning of a loop, AddBreakAddr or
** AddContinueAddr to register the address at which to store the branch
** address for a break or continue statement, and FillLoopAddrs to fill
** in all the addresses and return to the level of the enclosing loop.
*/
void StartLoopAddrList()
{
    addLoopAddr(NULL);
}
void AddBreakAddr(Inst *addr)
{
    addLoopAddr(addr);
    *addr = NEEDS_BREAK;
}
void AddContinueAddr(Inst *addr)
{   
    addLoopAddr(addr);
    *addr = NEEDS_CONTINUE;
}
static void addLoopAddr(Inst *addr)
{
    if (LoopStackPtr > &LoopStack[LOOP_STACK_SIZE-1]) {
    	fprintf(stderr, "NEdit: loop stack overflow in macro parser");
    	return;
    }
    *LoopStackPtr++ = addr;
}
void FillLoopAddrs(Inst *breakAddr, Inst *continueAddr)
{
    while (True) {
    	LoopStackPtr--;
    	if (LoopStackPtr < LoopStack) {
    	    fprintf(stderr, "NEdit: internal error (lsu) in macro parser\n");
    	    return;
    	}
    	if (*LoopStackPtr == NULL)
    	    break;
    	if (**LoopStackPtr == NEEDS_BREAK)
    	    **(Inst ***)LoopStackPtr = (Inst *)(breakAddr - *LoopStackPtr);
    	else if (**LoopStackPtr == NEEDS_CONTINUE)
    	    **(Inst ***)LoopStackPtr = (Inst *)(continueAddr - *LoopStackPtr);
    	else
    	    fprintf(stderr, "NEdit: internal error (uat) in macro parser\n");
    }
}

/*
** Execute a compiled macro, "prog", using the arguments in the array
** "args".  Returns one of MACRO_DONE, MACRO_PREEMPT, or MACRO_ERROR.
** if MACRO_DONE is returned, the macro completed, and the returned value
** (if any) can be read from "result".  If MACRO_PREEMPT is returned, the
** macro exceeded its alotted time-slice and scheduled...
*/
int ExecuteMacro(WindowInfo *window, Program *prog, int nArgs, DataValue *args,
    	DataValue *result, RestartData **continuation, char **msg)
{
    RestartData *context;
    static DataValue noValue = {NO_TAG, {0}};
    Symbol *s;
    int i;
    
    /* Create an execution context (a stack, a stack pointer, a frame pointer,
       and a program counter) which will retain the program state across
       preemption and resumption of execution */
    context = (RestartData *)XtMalloc(sizeof(RestartData));
    context->stack = (DataValue *)XtMalloc(sizeof(DataValue) * STACK_SIZE);
    *continuation = context;
    context->stackP = context->stack;
    context->pc = prog->code;
    context->runWindow = window;
    context->focusWindow = window;

    /* Push arguments and call information onto the stack */
    for (i=0; i<nArgs; i++)
    	*(context->stackP++) = args[i];
    context->stackP->val.ptr = NULL;
    context->stackP->tag = NO_TAG;
    context->stackP++;
    *(context->stackP++) = noValue;
    context->stackP->tag = NO_TAG;
    context->stackP->val.n = nArgs;
    context->stackP++;
    context->frameP = context->stackP;
    
    /* Initialize and make room on the stack for local variables */
    for (s = prog->localSymList; s != NULL; s = s->next) {
    	*(context->frameP + s->value.val.n) = noValue;
    	context->stackP++;
    }
    
    /* Begin execution, return on error or preemption */
    return ContinueMacro(context, result, msg);
}

/*
** Continue the execution of a suspended macro whose state is described in
** "continuation"
*/
int ContinueMacro(RestartData *continuation, DataValue *result, char **msg)
{
    register int status, instCount = 0;
    register Inst *inst;
    RestartData oldContext;
    
    /* To allow macros to be invoked arbitrarily (such as those automatically
       triggered within smart-indent) within executing macros, this call is
       reentrant. */
    saveContext(&oldContext);
    
    /*
    ** Execution Loop:  Call the succesive routine addresses in the program
    ** until one returns something other than STAT_OK, then take action
    */
    restoreContext(continuation);
    ErrMsg = NULL;
    for (;;) {
    	
    	/* Execute an instruction */
    	inst = PC++;
	status = (*inst)();
    	
    	/* If error return was not STAT_OK, return to caller */
    	if (status != STAT_OK) {
    	    if (status == STAT_PREEMPT) {
    		saveContext(continuation);
    		restoreContext(&oldContext);
    		return MACRO_PREEMPT;
    	    } else if (status == STAT_ERROR) {
		*msg = ErrMsg;
		FreeRestartData(continuation);
		restoreContext(&oldContext);
		return MACRO_ERROR;
	    } else if (status == STAT_DONE) {
		*msg = "";
		*result = *--StackP;
		FreeRestartData(continuation);
		restoreContext(&oldContext);
		return MACRO_DONE;
	    }
    	}
	
	/* Count instructions executed.  If the instruction limit is hit,
	   preempt, store re-start information in continuation and give
	   X, other macros, and other shell scripts a chance to execute */
    	instCount++;
	if (instCount >= INSTRUCTION_LIMIT) {
    	    saveContext(continuation);
    	    restoreContext(&oldContext);
    	    return MACRO_TIME_LIMIT;
	}
    }
}

/*
** If a macro is already executing, and requests that another macro be run,
** this can be called instead of ExecuteMacro to run it in the same context
** as if it were a subroutine.  This saves the caller from maintaining
** separate contexts, and serializes processing of the two macros without
** additional work.
*/
void RunMacroAsSubrCall(Program *prog)
{
    Symbol *s;
    static DataValue noValue = {NO_TAG, {0}};

    /* See subroutine "call" for a description of the stack frame for a
       subroutine call */
    StackP->tag = NO_TAG;
    StackP->val.ptr = PC;
    StackP++;
    StackP->tag = NO_TAG;
    StackP->val.ptr = FrameP;
    StackP++;
    StackP->tag = NO_TAG;
    StackP->val.n = 0;
    StackP++;
    FrameP = StackP;
    PC = prog->code;
    for (s = prog->localSymList; s != NULL; s = s->next) {
	*(FrameP + s->value.val.n) = noValue;
	StackP++;
    }
}

void FreeRestartData(RestartData *context)
{
    XtFree((char *)context->stack);
    XtFree((char *)context);
}

/*
** Cause a macro in progress to be preempted (called by commands which take
** a long time, or want to return to the event loop.  Call ResumeMacroExecution
** to resume.
*/
void PreemptMacro(void)
{
    PreemptRequest = True;
}

/*
** Reset the return value for a subroutine which caused preemption (this is
** how to return a value from a routine which preempts instead of returning
** a value directly).
*/
void ModifyReturnedValue(RestartData *context, DataValue dv)
{
    if (*(context->pc-1) == fetchRetVal)
	*(context->stackP-1) = dv;
}

/*
** Called within a routine invoked from a macro, returns the window in
** which the macro is executing (where the banner is, not where it is focused)
*/
WindowInfo *MacroRunWindow(void)
{
    return InitiatingWindow;
}

/*
** Set the window to which macro subroutines and actions which operate on an
** implied window are directed.
*/
void SetMacroFocusWindow(WindowInfo *window)
{
    FocusWindow = window;
}

/*
** find a symbol in the symbol table
*/
Symbol *LookupSymbol(char *name)
{
    Symbol *s;

    for (s = LocalSymList; s != NULL; s = s->next)
	if (strcmp(s->name, name) == 0)
	    return s;
    for (s = GlobalSymList; s != NULL; s = s->next)
	if (strcmp(s->name, name) == 0)
	    return s;
    return NULL;
}

/*
** install s in symbol table
*/
Symbol *InstallSymbol(char *name, int type, DataValue value)
{
    Symbol *s;

    s = (Symbol *)malloc(sizeof(Symbol));
    s->name = (char *)malloc(strlen(name)+1); /* +1 for '\0' */
    strcpy(s->name, name);
    s->type = type;
    s->value = value;
    if (type == LOCAL_SYM) {
    	s->next = LocalSymList;
    	LocalSymList = s;
    } else {
    	s->next = GlobalSymList;
    	GlobalSymList = s;
    }
    return s;
}

/*
** Promote a symbol from local to global, removing it from the local symbol
** list.
*/
Symbol *PromoteToGlobal(Symbol *sym)
{
    Symbol *s;
    static DataValue noValue = {NO_TAG, {0}};

    if (sym->type != LOCAL_SYM)
	return sym;

    /* Remove sym from the local symbol list */
    if (sym == LocalSymList)
	LocalSymList = sym->next;
    else {
	for (s = LocalSymList; s != NULL; s = s->next) {
	    if (s->next == sym) {
		s->next = sym->next;
		break;
	    }
	}
    }
    
    s = LookupSymbol(sym->name);
    if (s != NULL)
	return s;
    return InstallSymbol(sym->name, GLOBAL_SYM, noValue);
}

/*
** Allocate memory for a string, and keep track of it, such that it
** can be recovered later using GarbageCollectStrings.  (A linked list
** of pointers is maintained by threading through the memory behind
** the returned pointers).  Length does not include the terminating null
** character, so to allocate space for a string of strlen == n, you must
** use AllocString(n+1).
*/
char *AllocString(int length)
{
    char *mem;
    
    mem = XtMalloc(length + sizeof(char *) + 1);
    *((char **)mem) = AllocatedStrings;
    AllocatedStrings = mem;
    return mem + sizeof(char *) + 1;
}

/*
** Collect strings that are no longer referenced from the global symbol
** list.  THIS CAN NOT BE RUN WHILE ANY MACROS ARE EXECUTING.  It must
** only be run after all macro activity has ceased.
*/
void GarbageCollectStrings(void)
{
    char *p, *next;
    Symbol *s;
    
    /* mark all strings as unreferenced */
    for (p = AllocatedStrings; p != NULL; p = *((char **)p))
    	*(p + sizeof(char *)) = 0;
    
    /* Sweep the global symbol list, marking which strings are still
       referenced */
    for (s = GlobalSymList; s != NULL; s = s->next)
    	if (s->value.tag == STRING_TAG)
    	    *(s->value.val.str - 1) = 1;
    	    
    /* Collect all of the strings which remain unreferenced */
    next = AllocatedStrings;
    AllocatedStrings = NULL;
    while (next != NULL) {
    	p = next;
    	next = *((char **)p);
    	if (*(p + sizeof(char *)) != 0) {
    	    *((char **)p) = AllocatedStrings;
    	    AllocatedStrings = p;
    	} else {
    	    XtFree(p);
    	}
    }
}

/*
** Save and restore execution context to data structure "context"
*/
static void saveContext(RestartData *context)
{
    context->stack = Stack;
    context->stackP = StackP;
    context->frameP = FrameP;
    context->pc = PC;
    context->runWindow = InitiatingWindow;
    context->focusWindow = FocusWindow;
}
static void restoreContext(RestartData *context)
{
    Stack = context->stack;
    StackP = context->stackP;
    FrameP = context->frameP;
    PC = context->pc;
    InitiatingWindow = context->runWindow;
    FocusWindow = context->focusWindow;
}

static void freeSymbolTable(Symbol *symTab)
{
    Symbol *s;
    
    while(symTab != NULL) {
    	s = symTab;
    	free(s->name);
    	symTab = s->next;
    	free((char *)s);
    }    
}

#define POP(dataVal) \
    if (StackP == Stack) \
	return execError(StackUnderflowMsg, ""); \
    dataVal = *--StackP;
   
#define PUSH(dataVal) \
    if (StackP >= &Stack[STACK_SIZE]) \
    	return execError(StackOverflowMsg, ""); \
    *StackP++ = dataVal;

#define POP_INT(number) \
    if (StackP == Stack) \
	return execError(StackUnderflowMsg, ""); \
    --StackP; \
    if (StackP->tag == STRING_TAG) { \
    	if (!stringToNum(StackP->val.str, &number)) \
    	    return execError(StringToNumberMsg, ""); \
    } else \
    	number = StackP->val.n;

#define POP_STRING(string) \
    if (StackP == Stack) \
	return execError(StackUnderflowMsg, ""); \
    --StackP; \
    if (StackP->tag == INT_TAG) { \
    	string = AllocString(21); \
    	sprintf(string, "%d", StackP->val.n); \
    } else \
    	string = StackP->val.str; \
   
#define PUSH_INT(number) \
    if (StackP >= &Stack[STACK_SIZE]) \
    	return execError(StackOverflowMsg, ""); \
    StackP->tag = INT_TAG; \
    StackP->val.n = number; \
    StackP++;
    
#define PUSH_STRING(string) \
    if (StackP >= &Stack[STACK_SIZE]) \
    	return execError(StackOverflowMsg, ""); \
    StackP->tag = STRING_TAG; \
    StackP->val.str = string; \
    StackP++;

#define BINARY_NUMERIC_OPERATION(operator) \
    int n1, n2; \
    POP_INT(n2) \
    POP_INT(n1) \
    PUSH_INT(n1 operator n2) \
    return STAT_OK;

#define UNARY_NUMERIC_OPERATION(operator) \
    int n; \
    POP_INT(n) \
    PUSH_INT(operator n) \
    return STAT_OK;

static int pushSymVal()
{
    Symbol *s;
    int nArgs, argNum;
    
    s = (Symbol *)*PC++;
    if (s->type == LOCAL_SYM) {
    	*StackP = *(FrameP + s->value.val.n);
    } else if (s->type == GLOBAL_SYM || s->type == CONST_SYM) {
    	*StackP = s->value;
    } else if (s->type == ARG_SYM) {
    	nArgs = (FrameP-1)->val.n;
    	argNum = s->value.val.n;
    	if (argNum >= nArgs)
    	    return execError("referenced undefined argument: %s",  s->name);
    	if (argNum == N_ARGS_ARG_SYM) {
    	    StackP->tag = INT_TAG;
    	    StackP->val.n = nArgs;
    	} else
    	    *StackP = *(FrameP + argNum - nArgs - 3);
    } else if (s->type == PROC_VALUE_SYM) {
	DataValue result;
	char *errMsg;
	if (!((BuiltInSubr)s->value.val.ptr)(FocusWindow, NULL, 0,
	    	&result, &errMsg))
	    return execError(errMsg, s->name);
    	*StackP = result;
    } else
    	return execError("reading non-variable: %s", s->name);
    if (StackP->tag == NO_TAG)
    	return execError("variable not set: %s", s->name);
    StackP++;
    if (StackP >= &Stack[STACK_SIZE])
    	return execError(StackOverflowMsg, "");
    return STAT_OK;
}

static int assign()      /* assign top value to next symbol */
{
    Symbol *sym;
    sym = (Symbol *)(*PC++);
    if (sym->type != GLOBAL_SYM && sym->type != LOCAL_SYM)
	if (sym->type == ARG_SYM)
	    return execError("assignment to function argument: %s",  sym->name);
    	else if (sym->type == PROC_VALUE_SYM)
    	    return execError("assignment to read-only variable: %s", sym->name);
    	else
	    return execError("assignment to non-variable: %s", sym->name);
    if (StackP == Stack)
	return execError(StackUnderflowMsg, "");
    --StackP;
    if (sym->type == LOCAL_SYM)
    	*(FrameP + sym->value.val.n) = *StackP;
    else
	sym->value = *StackP;
    return STAT_OK;
}

static int dupStack()
{
    if (StackP >= &Stack[STACK_SIZE])
    	return execError(StackOverflowMsg, "");
    *StackP = *(StackP - 1);
    StackP++;
    return STAT_OK;
}

static int add()
{
    BINARY_NUMERIC_OPERATION(+)
}

static int subtract()
{
    BINARY_NUMERIC_OPERATION(-)
}

static int multiply()
{
    BINARY_NUMERIC_OPERATION(*)
}

static int divide()
{
    int n1, n2;
    POP_INT(n2)
    POP_INT(n1)
    if (n2 == 0) 
	return execError("division by zero", "");
    PUSH_INT(n1 / n2)
    return STAT_OK;
}

static int modulo()
{
    BINARY_NUMERIC_OPERATION(%)
}

static int negate()
{
    UNARY_NUMERIC_OPERATION(-)
}

static int increment()
{
    UNARY_NUMERIC_OPERATION(++)
}

static int decrement()
{
    UNARY_NUMERIC_OPERATION(--)
}

static int gt()
{
    BINARY_NUMERIC_OPERATION(>)
}

static int lt()
{
    BINARY_NUMERIC_OPERATION(<)
}

static int ge()
{
    BINARY_NUMERIC_OPERATION(>=)
}

static int le()
{
    BINARY_NUMERIC_OPERATION(<=)
}

static int eq()
{
    DataValue v1, v2;
    
    POP(v1)
    POP(v2)
    if (v1.tag == INT_TAG && v2.tag == INT_TAG)
	v1.val.n = v1.val.n == v2.val.n;
    else if (v1.tag == STRING_TAG && v2.tag == STRING_TAG)
    	v1.val.n = !strcmp(v1.val.str, v2.val.str);
    else if (v1.tag == STRING_TAG) {
 	int number;
 	if (!stringToNum(v1.val.str, &number))
    	    v1.val.n = 0;
    	else
    	    v1.val.n = number == v2.val.n;
    } else {
    	int number;
 	if (!stringToNum(v2.val.str, &number))
    	    v1.val.n = 0;
    	else
    	    v1.val.n = number == v1.val.n;
    }
    v1.tag = INT_TAG;
    PUSH(v1)
    return STAT_OK;
}

static int ne()
{
    eq();
    return not();
}

static int bitAnd()
{ 
    BINARY_NUMERIC_OPERATION(&)
}

static int bitOr()
{ 
    BINARY_NUMERIC_OPERATION(|)
}

static int and()
{ 
    BINARY_NUMERIC_OPERATION(&&)
}

static int or()
{
    BINARY_NUMERIC_OPERATION(||)
}
    
static int not()
{
    UNARY_NUMERIC_OPERATION(!)
}

static int power()
{
    int n1, n2;
    POP_INT(n2)
    POP_INT(n1)
    PUSH_INT((int)pow((double)n1, (double)n2))
    return errCheck("exponentiation");
}

static int concat()
{
    char *s1, *s2, *out;
    int len1, len2;
    POP_STRING(s2)
    POP_STRING(s1)
    len1 = strlen(s1);
    len2 = strlen(s2);
    out = AllocString(len1 + len2 + 1);
    strncpy(out, s1, len1);
    strcpy(&out[len1], s2);
    PUSH_STRING(out)
    return STAT_OK;
}

/*
** Call a subroutine or function (user defined or built-in).  Args are the
** subroutine's symbol, and the number of arguments which have been pushed
** on the stack.
**
** For a macro subroutine, the return address, frame pointer, number of
** arguments and space for local variables are added to the stack, and the
** PC is set to point to the new function. For a built-in routine, the
** arguments are popped off the stack, and the routine is just called.
**
**
**   The call stack for a subroutine call looks like
**
**   SP after return -> arg1
**      		arg2
**      		arg3
**      		.
**      		.
**      		.
**    SP before call -> ReturnAddress
**      		Saved FP
**      		nArgs
**      	  FP -> local1
**      		local2
**      		local3
**      		.
**      		.
**      		.
**     SP after call ->
*/
static int callSubroutine(void)
{
    Symbol *sym, *s;
    int i, nArgs;
    static DataValue noValue = {NO_TAG, {0}};
    Program *prog;
    char *errMsg;
    
    sym = (Symbol *)*PC++;
    nArgs = (int)*PC++;
    
    if (nArgs > MAX_ARGS)
    	return execError("too many arguments to subroutine %s (max 9)",
    	    	sym->name);
    
    /*
    ** If the subroutine is built-in, call the built-in routine
    */
    if (sym->type == C_FUNCTION_SYM) {
    	DataValue result, argList[MAX_ARGS];
    
	/* pop arguments off the stack and put them in the argument list */
	for (i=nArgs-1; i>=0; i--) {
    	    POP(argList[i])
	}
    	
    	/* Call the function and check for preemption */
    	PreemptRequest = False;
	if (!((BuiltInSubr)sym->value.val.ptr)(FocusWindow, argList,
	    	nArgs, &result, &errMsg))
	    return execError(errMsg, sym->name);
    	if (*PC == fetchRetVal) {
    	    if (result.tag == NO_TAG)
    	    	return execError("%s does not return a value", sym->name);
    	    PUSH(result);
	    PC++;
    	}
    	return PreemptRequest ? STAT_PREEMPT : STAT_OK;
    }
    
    /*
    ** Call a macro subroutine:
    **
    ** Push all of the required information to resume, and make space on the
    ** stack for local variables (and initialize them), on top of the argument
    ** values which are already there.
    */
    if (sym->type == MACRO_FUNCTION_SYM) {
    	StackP->tag = NO_TAG;
    	StackP->val.ptr = PC;
    	StackP++;
    	StackP->tag = NO_TAG;
    	StackP->val.ptr = FrameP;
    	StackP++;
    	StackP->tag = NO_TAG;
    	StackP->val.n = nArgs;
    	StackP++;
    	FrameP = StackP;
    	prog = (Program *)sym->value.val.str;
    	PC = prog->code;
	for (s = prog->localSymList; s != NULL; s = s->next) {
	    *(FrameP + s->value.val.n) = noValue;
	    StackP++;
	}
   	return STAT_OK;
    }
    
    /*
    ** Call an action routine
    */
    if (sym->type == ACTION_ROUTINE_SYM) {
    	String argList[MAX_ARGS];
    	Cardinal numArgs = nArgs;
    	XKeyEvent event;
    
	/* Create a fake event with a timestamp suitable for actions which need
	   timestamps, a marker to indicate that the call was from a macro
	   (to stop shell commands from putting up their own separate banner) */
	event.type = KeyPress;
	event.send_event = MACRO_EVENT_MARKER;
	event.time=XtLastTimestampProcessed(XtDisplay(InitiatingWindow->shell));
    
	/* pop arguments off the stack and put them in the argument list */
	for (i=nArgs-1; i>=0; i--) {
    	    POP_STRING(argList[i])
	}

    	/* Call the action routine and check for preemption */
    	PreemptRequest = False;
    	((XtActionProc)sym->value.val.ptr)(FocusWindow->lastFocus,
    	    	(XEvent *)&event, argList, &numArgs);
    	if (*PC == fetchRetVal)
    	    return execError("%s does not return a value", sym->name);
    	return PreemptRequest ? STAT_PREEMPT : STAT_OK;
    }

    /* Calling a non subroutine symbol */
    return execError("%s is not a function or subroutine", sym->name);
}

/*
** This should never be executed, returnVal checks for the presence of this
** instruction at the PC to decide whether to push the function's return
** value, then skips over it without executing.
*/
static int fetchRetVal(void)
{
    return execError("internal error: frv", NULL);
}

static int returnNoVal(void)
{
    return returnValOrNone(False);
}
static int returnVal(void)
{
    return returnValOrNone(True);
}

/*
** Return from a subroutine call
*/
static int returnValOrNone(int valOnStack)
{
    DataValue retVal;
    static DataValue noValue = {NO_TAG, {0}};
    int nArgs;
    
    /* return value is on the stack */
    if (valOnStack) {
    	POP(retVal);
    }
    
    /* pop past local variables */
    StackP = FrameP;
    
    /* get stored return information */
    nArgs = (--StackP)->val.n;
    FrameP = (--StackP)->val.ptr;
    PC = (--StackP)->val.ptr;
    
    /* pop past function arguments */
    StackP -= nArgs;
    
    /* push returned value, if requsted */
    if (PC == NULL) {
	if (valOnStack) {
    	    PUSH(retVal);
	} else {
	    PUSH(noValue);
	}
    } else if (*PC == fetchRetVal) {
	if (valOnStack) {
    	    PUSH(retVal);
	    PC++;
	} else {
	    return execError(
	    	"using return value of %s which does not return a value",
	    	((Symbol *)*(PC - 2))->name);
	}
    }
    
    /* NULL return PC indicates end of program */
    return PC == NULL ? STAT_DONE : STAT_OK;
}

/*
** Unconditional branch offset by immediate operand
*/
static int branch(void)
{
    PC += (int)*PC;
    return STAT_OK;
}

/*
** Conditional branches if stack value is True/False (non-zero/0) to address
** of immediate operand (pops stack)
*/
static int branchTrue(void)
{
    int value;
    Inst *addr;
    
    POP_INT(value)
    addr = PC + (int)*PC;
    PC++;
    
    if (value)
    	PC = addr;
    return STAT_OK;
}
static int branchFalse(void)
{
    int value;
    Inst *addr;
    
    POP_INT(value)
    addr = PC + (int)*PC;
    PC++;
    
    if (!value)
    	PC = addr;
    return STAT_OK;
}

/*
** Ignore the address following the instruction and continue.  Why? So
** some code that uses conditional branching doesn't have to figure out
** whether to store a branch address.
*/
static int branchNever(void)
{
    PC++;
    return STAT_OK;
}

/*
** checks errno after operations which can set it.  If an error occured,
** creates appropriate error messages and returns false
*/
static int errCheck(char *s)
{
    if (errno == EDOM)
	return execError("%s argument out of domain", s);
    else if (errno == ERANGE)
	return execError("%s result out of range", s);
    return STAT_OK;
}

/*
** combine two strings in a static area and set ErrMsg to point to the
** result.  Returns false so a single return execError() statement can
** be used to both process the message and return.
*/
static int execError(char *s1, char *s2)
{
    static char msg[MAX_ERR_MSG_LEN];
    
    sprintf(msg, s1, s2);
    ErrMsg = msg;
    return STAT_ERROR;
}

static int stringToNum(char *string, int *number)
{
    int i;
    char *c;
    
    /*... this is still not finished */
    for (c=string, i=0; *c != '\0'; i++, c++)
    	if (!(isdigit(*c) || *c != ' ' || *c != '\t'))
    	    return False;
    sscanf(string, "%d", number);
    return True;
}

#ifdef notdef /* For debugging code generation */
static void disasm(Program *prog, int nInstr)
{
    static char *opNames[N_OPS] = {"returnNoVal", "returnVal", "pushSymVal",
    	"dupStack", "add", "subtract", "multiply", "divide", "modulo",
        "negate", "increment", "decrement", "gt", "lt", "ge", "le", "eq",
	"ne", "bitAnd", "bitOr", "and", "or", "not", "power", "concat",
	"assign", "callSubroutine", "fetchRetVal", "branch", "branchTrue",
	"branchFalse", "branchNever"};
    int i, j;
    
    for (i=0; i<nInstr; i++) {
    	printf("%x ", &prog->code[i]);
    	for (j=0; j<N_OPS; j++) {
    	    if (prog->code[i] == OpFns[j]) {
    	    	printf("%s", opNames[j]);
    	    	if (j == OP_PUSH_SYM || j == OP_ASSIGN) {
    	    	    printf(" %s", ((Symbol *)prog->code[i+1])->name);
    	    	    i++;
    	    	} else if (j == OP_BRANCH || j == OP_BRANCH_FALSE ||
    	    	    	j == OP_BRANCH_NEVER) {
    	    	    printf(" (%d) %x", (int)prog->code[i+1],
    	    	    	    &prog->code[i+1] + (int)prog->code[i+1]);
    	    	    i++;
    	    	} else if (j == OP_SUBR_CALL) {
    	    	    printf(" %s (%d arg)", ((Symbol *)prog->code[i+1])->name,
    	    	    	    prog->code[i+2]);
    	    	    i += 2;
    	    	}
    	    	    
    	    	printf("\n");
    	    	break;
    	    }
    	}
    	if (j == N_OPS)
    	    printf("%x\n", prog->code[i]);
    }
}
#endif

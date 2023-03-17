void RegisterMacroSubroutines(void);
void AddLastCommandActionHook(XtAppContext context);
void BeginLearn(WindowInfo *window);
void FinishLearn(void);
void CancelMacroOrLearn(WindowInfo *window);
void Replay(WindowInfo *window);
void DoMacro(WindowInfo *window, char *macro, char *errInName);
void ResumeMacroExecution(WindowInfo *window);
void AbortMacroCommand(WindowInfo *window);
void RepeatDialog(WindowInfo *window);
void RepeatMacro(WindowInfo *window, char *command, int nTimes);
int ReadMacroFile(WindowInfo *window, char *fileName, int warnNotExist);
int ReadMacroString(WindowInfo *window, char *string, char *errIn);
int CheckMacroString(Widget dialogParent, char *string, char *errIn,
	char **errPos);
char *GetReplayMacro(void);
void ReadMacroInitFile(WindowInfo *window);
void ReturnShellCommandOutput(WindowInfo *window, char *outText, int status);

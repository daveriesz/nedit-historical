typedef struct {
    int nArgs;
    char **args;
} argList;

typedef struct {
    int nActions;
    char **actions;
    argList **args;
} actionList;

actionList *ParseActionList(char **inPtr, char **errMsg);
void FreeActionList(actionList *actionL);
void BeginLearn(WindowInfo *window);
void FinishLearn();
void CancelLearn();
void Replay(WindowInfo *window);
void DoMacro(WindowInfo *window, char *macro);
char *GetReplayMacro(void);
char *ActionListToString(actionList *actionL, char *indentStr);

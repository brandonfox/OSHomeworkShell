char** parseCommand(char* line);
char* getRedirect(char** cmds, int *resStatus);
void freeCommands(char** cmds);
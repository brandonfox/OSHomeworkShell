int handleSigInt();
int handleSigStop();
int hasActiveJob();
int lastExitStatus();
void printBackgroundJobs();
void initJobs();
void createNewProcess(char* command, char *args[], char* input);
void killAllProcesses();
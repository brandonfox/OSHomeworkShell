#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<pthread.h>
#include<stdlib.h>

int runningJobs = 0;
int activeJobIndex = 0;
pid_t *processes;
int processesSize = 16;
int lastExitCode = 0;
char ** commandStrings;

void initJobs(){
    processes = malloc(sizeof(pid_t) * processesSize);
    commandStrings = malloc(sizeof(char*) * processesSize);
}

int storeProcess(pid_t pid, char *args){
    if(runningJobs == processesSize){
        processesSize *= 2;
        processes = realloc(processes,sizeof(pid_t*) * processesSize);
        commandStrings = realloc(commandStrings,sizeof(char*) * processesSize);
    }
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) <= 0){
            printf("Process pid %d stored at index %d\n",pid,i);
            *(processes + i) = pid;
            *(commandStrings + i) = args;
            return i;
        }
    }
    //This means an error has occured
    return 0;
}

void moveToBackground(){
    printf("[%d] Has been moved to background\n",activeJobIndex);
    activeJobIndex = 0;
}

int hasActiveJob(){
    return activeJobIndex;
}

int handleSigInt(){
    if(activeJobIndex > 0){
        kill(*(processes + activeJobIndex),SIGINT);
        return 1;
    }
    else return 0;
}
int handleSigStop(){
    if(activeJobIndex > 0){
        kill(*(processes + activeJobIndex),SIGTSTP);
        printf("\n");
        fflush(stdout);
        moveToBackground(activeJobIndex);
        return 1;
    }else return 0;
}

void printBackgroundJobs(){
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) > 0)
            printf("[%d] %s\n",i,*(commandStrings + i));
    }
}

int removeProcess(pid_t pid){
    runningJobs--;
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) == pid){
            *(processes + i) = 0;
            free(*(commandStrings + i));
            return i;
        }
    }
    return -1;
}

void *waitForChildTermination(void* pidPointer){
    pid_t pid = *(pid_t*)pidPointer;
    printf("Created thread to moniter pid %d\n",pid);
    int stat;
    waitpid(pid,&stat,0);
    if(WIFEXITED(stat)){
        printf("Removing pid %d, activejobindex:%d\n",pid,activeJobIndex);
        if(*(processes + activeJobIndex) == pid){
            printf("Removing active process\n");
            activeJobIndex = 0;
            lastExitCode = stat;
        }
        removeProcess(pid);
        pthread_exit(NULL);
    }
    else
    {
        waitForChildTermination(&pid);
    }
}

void createNewProcess(char* args){
    pid_t pid = fork();
    if(pid == 0){
        execlp(args,args,NULL);
        printf("Command not found\n");
        exit(0);
    }
    printf("Created process with pid %d\n",pid);
    pthread_t childMoniterThread;
    pthread_create(&childMoniterThread,NULL,waitForChildTermination,&pid);
    runningJobs++;
    activeJobIndex = storeProcess(pid,args);
}
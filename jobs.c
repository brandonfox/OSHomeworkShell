#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<pthread.h>
#include<stdlib.h>

#define idle -1

int runningJobs = 0;
int activeJobIndex = idle;
pid_t *processes;
int processesSize = 2;
int lastExitCode = 0;
char **commandStrings;

void continueJob(int job){
    if(*(processes + job - 1) > 0){
        printf("Bringing job to fg\n");
        activeJobIndex = job - 1;
        kill(*(processes + job - 1),SIGCONT);
    }else printf("Invalid job number");
}

void initCmdStringsArr(char** arr, int size){
    for(int i = 0; i < size; i++){
        *(arr + i) = NULL;
    }
}

void initProcessArr(pid_t *arr, int size){
    for(int i = 0; i < size; i++){
        *(arr + i) = 0;
    }
}

int removeProcess(pid_t pid){
    runningJobs--;
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) == pid){
            //printf("Removing process with pid %d and cmdString %s\n",pid,*(commandStrings + i));
            *(processes + i) = 0;
            free(*(commandStrings + i));
            if(activeJobIndex == i) activeJobIndex = idle;
            return i;
        }
    }
    return -1;
}

void childHandler(int sig){
    int status;
    pid_t pid = wait3(&status,WNOHANG,NULL);
    if(WIFEXITED(status) && pid > 0){
        //printf("pid %d exited with exit status %d\n",pid,WEXITSTATUS(status));
        removeProcess(pid);
    }
}

void initJobs(){
    processes = malloc(sizeof(pid_t) * processesSize);
    initProcessArr(processes,processesSize);
    commandStrings = malloc(sizeof(char*) * processesSize);
    initCmdStringsArr(commandStrings,processesSize);
    signal(SIGCHLD,childHandler);
}

int storeProcess(pid_t pid, char* input){
    //printf("Running jobs: %d, Current process limit %d\n",runningJobs,processesSize);
    if(runningJobs == processesSize){
        processesSize *= 2;
        //printf("Expanding process array\n");
        processes = realloc(processes,sizeof(pid_t*) * processesSize);
        initProcessArr(processes + sizeof(pid_t*) * (processesSize/2),processesSize/2);
        commandStrings = realloc(commandStrings,sizeof(char*) * processesSize);
        initCmdStringsArr(commandStrings + sizeof(char**) * (processesSize / 2),processesSize / 2);
    }
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) <= 0){
            //printf("Storing process with pid %d and cmdString %s\n",pid,input);
            *(processes + i) = pid;
            *(commandStrings + i) = input;
            return i;
        }
    }
    //This means an error has occured
    return 0;
}

void moveToBackground(){
    printf("[%d] '%s' has been moved to background\n",activeJobIndex+1,*(commandStrings + activeJobIndex));
    activeJobIndex = idle;
}

int hasActiveJob(){
    return activeJobIndex;
}

int handleSigInt(){
    if(activeJobIndex > idle){
        kill(*(processes + activeJobIndex),SIGINT);
        return 1;
    }
    else return 0;
}
int handleSigStop(){
    if(activeJobIndex > idle){
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
            printf("[%d] %s\n",i+1,*(commandStrings + i));
    }
}

void createNewProcess(char* command,char **args,char *input){
    pid_t pid = fork();
    if(pid == 0){
        execvp(command,args);
        printf("Command '%s' not found\n",input);
        fflush(stdout);
        exit(0);
    }
    else{
        runningJobs++;
        activeJobIndex = storeProcess(pid,input);
    }
}

int lastExitStatus(){
    return lastExitCode;
}

void killAllProcesses(){
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) > 0) kill(*(processes + i),SIGKILL);
    }
}
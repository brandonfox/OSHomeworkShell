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
int processesSize = 16;
int lastExitCode = 0;
char **commandStrings;

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
    //printf("Child process pid %d exited with status code %d\n",pid,status);
    removeProcess(pid);
}

void initJobs(){
    processes = malloc(sizeof(pid_t) * processesSize);
    commandStrings = malloc(sizeof(char*) * processesSize);
    signal(SIGCHLD,childHandler);
}

int storeProcess(pid_t pid, char* input){
    if(runningJobs == processesSize){
        processesSize *= 2;
        processes = realloc(processes,sizeof(pid_t*) * processesSize);
        commandStrings = realloc(commandStrings,sizeof(char*) * processesSize);
    }
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) <= 0){
            //printf("Storing process with pid %d and cmdString %s\n",pid,*(commandStrings + i));
            *(processes + i) = pid;
            *(commandStrings + i) = input;
            return i;
        }
    }
    //This means an error has occured
    return 0;
}

void moveToBackground(){
    printf("[%d] Has been moved to background\n",activeJobIndex);
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
    activeJobIndex = storeProcess(pid,input);
    runningJobs++;
    if(pid == 0){
        execvp(command,args);
        printf("Command '%s' not found\n",input);
        fflush(stdout);
        exit(0);
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
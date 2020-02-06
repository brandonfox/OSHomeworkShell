#define _GNU_SOURCE
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<pthread.h>
#include<stdlib.h>
#include<semaphore.h>
#include<fcntl.h>
#include<errno.h>

#define idle -1

int runningJobs = 0;
int activeJobIndex = idle;
pid_t *processes;
int processesSize = 2;
int lastExitCode = 0;
char **commandStrings;

int continueJob(int job){
    pid_t pid = *(processes + job - 1);
    if(pid > 0){
        //printf("Sending continue command to process %d\n",pid);
        setpgid(0,pid);
        kill(pid,SIGCONT);
        return job-1;
    }
    else{
        printf("Invalid job number\n");
        return idle;
    } 
}

int continueJobFg(int job){
    activeJobIndex = continueJob(job);
    return activeJobIndex;
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
            if(activeJobIndex == i){
                activeJobIndex = idle;
                //printf("Setting fg group to this\n");
            }
            return i;
        }
    }
    return -1;
}

void childHandler(){
    //printf("Something has happened to a child\n");
    int status;
    pid_t pid = wait3(&status,WUNTRACED|WCONTINUED|WNOHANG,NULL);
    if(pid > 0){
        //printf("%d,%d\n",pid,status);
        if(WIFEXITED(status)){
            //printf("pid %d exited with exit status %d\n",pid,WEXITSTATUS(status));
            removeProcess(pid);
        }
        else if(WIFSIGNALED(status)){
            //printf("Pid: %d exited due to signal %d\n",pid,WTERMSIG(status));
            if(WTERMSIG(status) == SIGKILL){
                removeProcess(pid);
            }
        }
        else if (WIFSTOPPED(status)){
            //printf("Pid: %d stopped with code %d\n",pid,WSTOPSIG(status));
        }else{
            //printf("Pid: %d continued \n",pid);
        }
    }
}

void initJobs(){
    printf("This shell pid is %d\n",getpid());
    processes = malloc(sizeof(pid_t) * processesSize);
    initProcessArr(processes,processesSize);
    commandStrings = malloc(sizeof(char*) * processesSize);
    initCmdStringsArr(commandStrings,processesSize);
    struct sigaction childsig = {0};
    childsig.sa_handler = childHandler;
    childsig.sa_flags = SA_SIGINFO|SA_RESTART;
    sigaction(SIGCHLD,&childsig,NULL);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
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

void moveToBackground(int jobIndex){
    pid_t pid = *(processes + jobIndex);
    printf("[%d] '%s' has been moved to background\n",jobIndex+1,*(commandStrings + jobIndex));
    setpgid(pid,pid);
    activeJobIndex = idle;
}

int hasActiveJob(){
    return activeJobIndex;
}

int handleSigInt(){
    pid_t pid = *(processes + activeJobIndex);
    if(activeJobIndex > idle){
        //printf("Sending kill command to job %d, process id %d\n",activeJobIndex,pid);
        setpgid(pid,pid);
        kill(pid,SIGKILL);
        return 1;
    }
    else return 0;
}
int handleSigStop(){
    pid_t pid = *(processes + activeJobIndex);
    if(activeJobIndex > idle){
        //printf("Sending stop signal to process %d\n",pid);
        moveToBackground(activeJobIndex);
        kill(pid,SIGTSTP);
        return 1;
    }else return 0;
}

void printBackgroundJobs(){
    for(int i = 0; i < processesSize; i++){
        if(*(processes + i) > 0)
            printf("[%d] %s\n",i+1,*(commandStrings + i));
    }
}

void createNewProcess(char* command,char **args,char *input,int redirectStatus, char* redirectFile,int background){

    if(redirectStatus != 0 && redirectFile == NULL){
        printf("Must specify a file for I/O redirection\n");
        return;
    }
    int indup = dup(STDIN_FILENO);
    int outdup = dup(STDOUT_FILENO);
    int f1;
    if(redirectFile != NULL){
        f1 = creat(redirectFile,0666);
        //printf("Setting pipe to file %s, %d, errno: %d\n",redirectFile,f1,errno);
    }   
    pid_t pid = fork();
    if(pid == 0){
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset,SIGTTIN);
        sigaddset(&sigset,SIGTTOU);
        sigaddset(&sigset,SIGCHLD);
        sigprocmask(SIG_BLOCK,&sigset,NULL);
        if(redirectStatus == 0){
            dup2(outdup,STDOUT_FILENO);
        }
        else if(redirectStatus > 0){
            dup2(f1,STDOUT_FILENO);
        }
        else{
            dup2(f1,STDIN_FILENO);
        }

        execvp(command,args);
        printf("Command '%s' not found\n",input);
        exit(0);
    }
    else{
        runningJobs++;
        activeJobIndex = storeProcess(pid,input);
        if(background){
            activeJobIndex = idle;
            setpgid(pid,pid);
        }
        else setpgid(0,pid);
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
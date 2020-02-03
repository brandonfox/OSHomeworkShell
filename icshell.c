#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include "jobs.h"
#include "commandparser.h"

void resetPrompt(){
    printf("\nicsh>");
    fflush(stdout);
}

void sigIntHandler(int sig){
    if(!handleSigInt()){
        resetPrompt();
    }
}

void sigStopHandler(int sig){
    if(!handleSigStop())
        resetPrompt();
}

char exitCmd[3] = {'$','?'};

void printArgs(char** args){
    for(char **c = args + 1; *c != (char*)-1; c++){
        if(strcmp(*c,exitCmd) == 0) printf("%d ",lastExitStatus());
        else printf("%s ",*c);
    }
    printf("\n");
}

int main(){

    //Assign signal handlers here
    signal(SIGINT,sigIntHandler);
    signal(SIGTSTP,sigStopHandler);

    printf("Welcome to Brandon's Ic Shell (icsh)\n");
    
    char exit[5] = {'e','x','i','t'};
    char jobs[5] = {'j','o','b','s'};
    char echo[5] = {'e','c','h','o'};

    initJobs();

    while(1){

        char *input = malloc(512);
        printf("icsh>");
        fgets(input,512,stdin);
        input[strlen(input) - 1] = '\0';
        char **commandArgs = parseCommand(input);
        if(strlen(input) == 0) {
            free(input);
            continue;
        }
        else if(strcmp(*commandArgs,exit) == 0){
            free(input);
            freeCommands(commandArgs);
            killAllProcesses();
            return 0;
        }
        else if(strcmp(*commandArgs,jobs) == 0){
            printBackgroundJobs();
            free(input);
            freeCommands(commandArgs);
        }
        else if(strcmp(*commandArgs,echo) == 0){
            printArgs(commandArgs);
            free(input);
            freeCommands(commandArgs);
        }
        else{
            createNewProcess(*commandArgs,commandArgs + 1,input);
        }

        while(hasActiveJob()){
            continue;
        }
    }
}
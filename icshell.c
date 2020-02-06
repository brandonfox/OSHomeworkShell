#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include "jobs.h"
#include "commandparser.h"

void sigIntHandler(){
    handleSigInt();
    printf("\n");
}

void sigStopHandler(){
    handleSigStop();
    printf("\n");
}

char exitCmd[3] = {'$','?'};

void replaceEchoVars(char** args){
    for(char **c = args; *c != NULL; c++){
        if(strcmp(*c,exitCmd) == 0){
            free(*c);
            int length = snprintf(NULL,0,"%d",lastExitStatus());
            *c = malloc(length + 1);
            snprintf(*c,length + 1,"%d",lastExitStatus());
        }
    }
}

int sendToBackground(char** args){
    for(char **c = args; *c != NULL; c++){
        if(**c == '&'){
            *c = NULL;
            return 1;
        }
    }
    return 0;
}

int main(){

    struct sigaction inthandler = {0}; //Initialise other values
    inthandler.sa_handler = sigIntHandler;

    struct sigaction stophandler = {0};
    stophandler.sa_handler = sigStopHandler;
    
    //Assign signal handlers here
    sigaction(SIGINT,&inthandler,NULL);
    sigaction(SIGTSTP,&stophandler,NULL);

    printf("Welcome to Brandon's Ic Shell (icsh)\n");
    
    char exit[5] = {'e','x','i','t'};
    char jobs[5] = {'j','o','b','s'};
    char echo[5] = {'e','c','h','o'};
    char fg[3] = {'f','g'};
    char bg[3] = {'b','g'};

    initJobs();

    while(1){

        while(hasActiveJob() >= 0){
            continue;
        }

        char *input = malloc(512);
        printf("icsh>");
        fgets(input,512,stdin);
        input[strlen(input) - 1] = '\0';
        char **commandArgs = parseCommand(input);
        if(strlen(input) == 0) {
            free(input);
            freeCommands(commandArgs);
            continue;
        }
        else if(strcmp(*commandArgs,exit) == 0){
            killAllProcesses();
            return 0;
        }
        else if(strcmp(*commandArgs,jobs) == 0){
            printBackgroundJobs();
            free(input);
            freeCommands(commandArgs);
        }
        else if(strcmp(*commandArgs,fg) == 0){
            printf("Bringing job to fg\n");
            continueJobFg(atoi(*(commandArgs + 1)));
        }
        else if(strcmp(*commandArgs,bg) == 0){
            printf("Continuing background job\n");
            continueJob(atoi(*(commandArgs + 1)));
        }
        else{
            int inOut = 0; //-1 for input 1 for output 0 for none
            char* IOredirect = getRedirect(commandArgs,&inOut);
            if(strcmp(*commandArgs,echo) == 0){
                replaceEchoVars(commandArgs);
            }
            //printf("Got redirect status %d\n",inOut);
            createNewProcess(*commandArgs,commandArgs,input,inOut,IOredirect,sendToBackground(commandArgs));
            freeCommands(commandArgs);
        }
    }
}
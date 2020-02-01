#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include "jobs.h"

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

int main(){

    //Assign signal handlers here
    signal(SIGINT,sigIntHandler);
    signal(SIGTSTP,sigStopHandler);

    printf("Welcome to Brandon's Ic Shell (icsh)\n");
    
    char exit[5] = {'e','x','i','t'};
    char jobs[5] = {'j','o','b','s'};

    initJobs();

    while(1){

        while(hasActiveJob()){
            continue;
        }

        char *input = malloc(256);

        printf("icsh>");
        fgets(input,sizeof(input),stdin);
        input[strlen(input)-1] = 0;
        if(strlen(input) == 0) {
            continue;
        }
        else if(strcmp(input,exit) == 0){
            printf("Got exit command\n");
        }
        else if(strcmp(input,jobs) == 0){
            printBackgroundJobs();
        }
        else{
            createNewProcess(input);
        }
    }
}
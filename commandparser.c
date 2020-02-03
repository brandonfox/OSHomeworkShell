#include<stdlib.h>

//Remove later
#include<stdio.h>

char** parseCommand(char* line){
    char **argv = malloc(sizeof(char**));
    int command = -1;
    char* curentWord = malloc(sizeof(char*));
    int wordLen = -1;
    for(char *c = line; *c != '\0'; c++){
        if(*c == ' ' || *c == '\n'){
            command++;
            argv = realloc(argv,sizeof(char**) * (command + 1));
            *(argv + command)= curentWord;
            wordLen = -1;
            curentWord = malloc(sizeof(char*));
        }
        else{
            wordLen++;
            curentWord = realloc(curentWord,sizeof(char*) * (wordLen + 2));
            *(curentWord + wordLen) = *c;
        }
    }
    if(wordLen >= 0){
        command++;
        argv = realloc(argv,sizeof(char**) * (command + 1));
        *(argv + command) = curentWord;
    }else free(curentWord);
    argv = realloc(argv,sizeof(char**) * (command + 2));
    *(argv + command + 1) = (char*)-1;
    return argv;
}
void freeCommands(char** cmd){
    for(char **c = cmd; *c != (char*)-1; c++){
        free(*c);
    }
    free(cmd);
}
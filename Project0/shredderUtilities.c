#include <stdio.h>
#include <string.h>
#include "shredderUtilities.h"

int getNumArgs(char * command) {
    int numArgs;
    numArgs = 0;

    char * commandArg;
    commandArg = strtok(command, " ");
    while(commandArg != NULL){
        if (strlen(commandArg) > 0){
            numArgs++;
        }
        commandArg = strtok(NULL, " ");
    }
    return numArgs;
}

int stripCpy(char * destination, char * source) {
    int needNewLine = 0;
    char *tempPtr = destination;
    while(*source != '\0'){
        if(*source != '\n' && *source != '\t'){
            *destination = *source;
            needNewLine = 1;
        } else if(*source == '\n'){
            *destination = ' ';
            needNewLine = 0;
        } else if(*source == '\t'){
            *destination = ' ';
            needNewLine = 1;
        }
        destination++;
        source++;
    }
    *destination = '\0';
    destination = tempPtr;
    return needNewLine;
}

int isdigitImpl(int c){
    if (c >= '0' && c <= '9'){
        return 1;
    } else{
        return 0;
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include "main.h"
#include "shredderUtilities.h"

#define MAX_LINE_LENGTH 4096

static unsigned int running = 1;
static int delay;
static int childStatus = 0;
static int childFinished = 0;
static pid_t pid;

/*
Handles Execution Timeout Arguments, Runs Program until Finished
*/
int main(int argc, char **argv) {
    if(argc > 2){
        printf("Error: Incorrect Num of Arguments Supplied: Expected 1, Got %d\n", argc - 1);
        exit(EXIT_FAILURE);
    } else if (argc == 2){
        char* delayInput = argv[1];
        int containsNonNum = 0;
        for(int i = 0; i < strlen(argv[1]); i ++){
            if (!isdigit(delayInput[i]) && delayInput[i] != '-') {
                containsNonNum |= 1;
            }
        }
        if(!containsNonNum){
            delay = atoi(argv[1]);
            if (delay < 0) { 
                printf("Error: Delay Must Be Non-Negative\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    while(running){
        runProgram();
    }
}

void runProgram() {
    if(signal(SIGINT, sigIntHandler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    char * command = (char *) malloc(MAX_LINE_LENGTH);

    write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
    
    //Handle Control-D
    int numBytes = read(STDIN_FILENO, command, MAX_LINE_LENGTH);
    if(numBytes == 0){
        write(STDOUT_FILENO, "\n", 1);
        running = 0;
    } else if(numBytes == -1){
        perror("read");
        free(command);
        exit(EXIT_FAILURE);
    } else {
        //Execute Command
        command[numBytes] = '\0';
        executeCommand(command);
    }
    free(command);
}

void executeCommand(char * command) {
    
    char * commandCopy = (char *) malloc(strlen(command) + 1);
    stripCpy(commandCopy, command);

    //Remove Tabs and Newlines, Handle Control-D
    char * strippedCommand = (char *) malloc(strlen(commandCopy) + 1);
    if(stripCpy(strippedCommand, command)){
        write(STDOUT_FILENO, "\n", 1);
    }
    int numArgs;
    numArgs = getNumArgs(commandCopy);
    free(commandCopy);

    //Handle Empty Line
    if (numArgs == 0){
        free(strippedCommand);
        return;
    }

    char **argList = (char **) malloc(sizeof(char *) * (numArgs + 1));
    
    childFinished = 0;
    pid = fork();
    if(pid == -1){
        perror("fork");
        free(strippedCommand);
        free(argList);
        free(command);
        exit(EXIT_FAILURE);
    }

    if (pid == 0){ //Child
        int exArgI = 0;
        char * commandArg;
        commandArg = strtok(strippedCommand, " ");
        while(commandArg != NULL){
            if (strlen(commandArg) > 0){
                argList[exArgI] = (char *) malloc(sizeof(char) * (strlen(commandArg) + 1));
                stripCpy(argList[exArgI], commandArg);
                exArgI++;
            }
            commandArg = strtok(NULL, " ");
        }
        argList[numArgs] = NULL;
        if(execve(argList[0], argList, (char*[]) {NULL}) == -1){
            perror("Execution Error");
            free(strippedCommand);
            for(int i = 0; argList[i]; i++){
                free(argList[i]);
            }
            free(argList);
            free(command);
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0){ //Parent
        if(delay){ 
            if(signal(SIGALRM, sigAlarmHandler) == SIG_ERR){
                perror("Parent Signal");
                free(strippedCommand);
                free(argList);
                exit(EXIT_FAILURE);
            }
            alarm(delay);
        }
        pid_t waitPID;
        do {
            waitPID = wait(&childStatus);
            if (waitPID == -1){
                perror("waitpid");
                free(strippedCommand);
                free(argList);
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(childStatus)){
                childFinished = 1;
            }
        } while(!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));
        alarm(0);
        //Free Execution Argument List
        free(argList);
    }
    free(strippedCommand);
}

void sigAlarmHandler(int signum) {
    if(signum == SIGALRM && childFinished == 0){
        write(STDOUT_FILENO, CATCHPHRASE, strlen(CATCHPHRASE));
        if(kill(pid, SIGKILL) == -1){
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }
}

void sigIntHandler(int signum) {
    write(STDOUT_FILENO, "\n", 1);
}
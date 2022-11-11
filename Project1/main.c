#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "parser.h"
#include "pidManager.h"

#define MAX_LINE_LENGTH 4096

int * signalPidArray = NULL;
int signalHandlerGPID = -1;
char * signalHandlerCommand = NULL;
struct PidNode * signalHeadProcess = NULL;

/**
 * Problems to fix:
 * TODO: fg 1 --> removes all jobs except for the running one (stopped) || recreate --> sleep 30 (stopped), sleep 40 (running), sleep 50 (stopped), fg 1
 * 
 * TODO: Control C broken
 */

/**
 * ==========================================
 * Function: waitForChildren
 * ==========================================
 * 
 * Waits for a child command
 * 
 * [params]:
 * int * pidArr: the array of PIDs of the commands
 * int * childrenFinished: a sudo boolean value which 
 * represents if all children up to this child have exited
 * or not
 * 
 * [returns]: the status of the child
 */
int waitForChildren(int * pidArr, int numChildren, int gpid, char * command, struct PidNode ** head) {
    // printf("Waiting 1\n");
    //need to make sure not to touch pidArr itself so that we 
    for(int i = 0; i < numChildren; i++){
        // printf("Waiting 2\n");
        int status;
        if(waitpid(pidArr[i], &status, WUNTRACED) == -1){
            // printf("Waiting 3\n");
            return -1;
        }
        if (WIFSTOPPED(status) && gpid != -1){
            insertJob(pidArr[i], gpid, JobStoppped, command, head);
        }
        // else {
        //     signalPidArray = NULL;
        //     signalHandlerGPID = -1;
        //     signalHandlerCommand = NULL;
        //     signalHeadProcess = NULL;
        // }
    }
    // printf("Waiting 5\n");
    return 0;
}

/**
 * ==========================================
 * Function: strCpy
 * ==========================================
 * 
 * Copies a string
 * 
 * [params]:
 * char *dest: the string that is being copied to
 * char *src: the string that is being copies from
 * 
 * [returns]: the copied string
 */
char *strCpy(char *dest, char *src) {
    char *cmpCopy = dest;
    while(*src != '\0'){
        *dest = *src;
        src++;
        dest++;
    }
    *dest = '\0';
    return cmpCopy;
}

char * getCleanCommand(struct parsed_command * pCmd){
    if (pCmd->num_commands == 0){
        return NULL;
    }
    char * pipeDelimeter = "| ";
    char * argDelimeter = " ";
    int stringLen = 0;
    for (int pipeIndex = 0; pipeIndex < pCmd->num_commands; pipeIndex++){
        char ** pipeArguments = pCmd->commands[pipeIndex];
        if (pipeArguments == NULL) {
            return NULL;
        }
        for (char ** ptr = pipeArguments; *ptr; ptr++){
            stringLen += strlen(*ptr) + strlen(argDelimeter);
        }
        if (pipeIndex != pCmd->num_commands - 1){
            stringLen += strlen(pipeDelimeter);
        }
    }
    char * cleanCommand = (char *) malloc(stringLen + 1);
    int charIndex = 0;
    for (int pipeIndex = 0; pipeIndex < pCmd->num_commands; pipeIndex++){
        char ** pipeArguments = pCmd->commands[pipeIndex];
        for (char ** argumentPtr = pipeArguments; *argumentPtr; argumentPtr++){
            for(char * wordPtr = *argumentPtr; *wordPtr; wordPtr++){
                cleanCommand[charIndex] = *wordPtr;
                charIndex++;
            }
            //if (*argumentPtr[0] = '\n'){
            cleanCommand[charIndex] = ' ';
            charIndex++;
            //}
        }
        if (pipeIndex != pCmd->num_commands - 1){
            cleanCommand[charIndex] = '|';
            charIndex++;
            cleanCommand[charIndex] = ' ';
            charIndex++;
        }
    }
    //printf("\n");
    cleanCommand[stringLen] = '\0'; //TODO: free this like wes and nolan did proj 0, or everytime we remove from linked list
    return cleanCommand;
}


/**
 * ==========================================
 * Function: signalFunction
 * ==========================================
 * 
 * Handles ^C
 * 
 * [params]:
 * int signo: the signal number
 * 
 * [returns]: void
 */
void signalFunction(int signo) { //TODO: Fix Control-C Logic (Or don't)
    if (signo == SIGINT) { //Control-C
        write(STDERR_FILENO, "\n", 2);
        if (signalHandlerGPID != -1 && signalPidArray != NULL){
            if(killpg(signalHandlerGPID, SIGKILL) == -1) {
                perror("Kill GPID [main.c 148]");
                exit(EXIT_FAILURE);
            }
        } else {
            write(STDERR_FILENO, PROMPT, strlen(PROMPT));
        }
    } else if (signo == SIGTSTP) { //Control-Z
        if (signalHandlerGPID != -1 && signalPidArray != NULL){
            write(STDERR_FILENO, "\nStopped: ", 11);
            write(STDERR_FILENO, signalHandlerCommand, strlen(signalHandlerCommand));
            write(STDERR_FILENO, "\n", 2);
            if(killpg(signalHandlerGPID, SIGTSTP) == -1) {
                perror("Kill GPID [main.c 155]");
                exit(EXIT_FAILURE);
            }
        }
    }/* else if (signo == SIGTTIN){

    } else if (signo == SIGTTOU){
        
    }*/

    
    signalPidArray = NULL;
    signalHandlerGPID = -1;
    signalHandlerCommand = NULL;
    signalHeadProcess = NULL;
}

void closePipeWrites(int ** fdArr, int numPipes){
    for (int i = 0; i < numPipes; i++){
        close(fdArr[i][1]);
    }
}

int setChildrenPGID(int * pidArr, int numPipes, int newPGID){
    for (int i = 0; i < numPipes; i++) {
        int childPID = pidArr[i];
        setpgid(childPID, newPGID);
    }
    return 0;
}

int executeCommand(int pipeIndex, struct parsed_command * pCmd, int * pidArr, int ** fdArr, int * groupPID, struct PidNode ** headProcess, char * cleanCommand){
    char ** newargv = pCmd->commands[pipeIndex];

    /*
    ================================
    Handle just pressing enter
    ================================
    */
    if (newargv == NULL) {
        return -1;
    }
    /*
    ================================
    Fork and execute
    ================================
    */
    int pid = fork();
    pidArr[pipeIndex] = pid;
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { //Child Process
        int new_stdout, new_stdin;
        /**
         * First Pipe 
        */
        if(pipeIndex == 0){
            if(pCmd->stdin_file){
                new_stdin = open(pCmd->stdin_file, O_RDONLY, 0644);
                dup2(new_stdin, STDIN_FILENO);
                close(new_stdin);
            }
            if (pCmd->num_commands > 1) {
                dup2(fdArr[pipeIndex][1], STDOUT_FILENO); //Write to Standard Out into Pipe 0
            }
        }

        /**
         * Intermediate Pipe 
        */
        if(pipeIndex > 0 && pipeIndex < pCmd->num_commands - 1) {
            dup2(fdArr[pipeIndex - 1][0], STDIN_FILENO); //read from standard-in into last pipe
            dup2(fdArr[pipeIndex][1], STDOUT_FILENO); // write to standard-out into pipe
        }

        /**
         * Last Pipe
        */
        if(pipeIndex == pCmd->num_commands - 1){ //Another If Statement In case of 1 pipe
            if(pCmd->stdout_file){
                int flags = O_WRONLY | O_CREAT;
                if(pCmd->is_file_append) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }

                new_stdout = open(pCmd->stdout_file, flags, 0644);
                dup2(new_stdout, STDOUT_FILENO);
                close(new_stdout);
            }
            if(pCmd -> num_commands > 1){
                dup2(fdArr[pipeIndex - 1][0], STDIN_FILENO);
            }
        }
        
        //Close all Sibling Write Sides
        closePipeWrites(fdArr, pCmd->num_commands - 1);
        if (execvp(newargv[0], newargv) == -1) {
            free(pCmd);
            perror("execve");
            exit(EXIT_FAILURE);
        }
    } else {
        if (pipeIndex == 0){
            *groupPID = pid;
        }
        if (pCmd->is_background){
            insertJob(pid, *groupPID, JobRunning, cleanCommand, headProcess);
        }
    }
    return 0;
}

void waitForBackgroundProcess(struct PidNode ** headProcess){
    //Check on background processes
    //printf("INSIDE waitForBackgroundProcess\n");
    int status;
    int backGroundPID = waitpid(-1, &status, WNOHANG | WUNTRACED);
    //printf("segfault check 5.1\n");
    if(backGroundPID > 0) {
        //printf("segfault check 5.2\n");
        struct PidNode * finishedNode = removePID(backGroundPID, headProcess);
        //printf("segfault check 5.3\n");
        if (finishedNode != NULL && groupFinished(finishedNode, *headProcess) ){
            //printf("segfault check 5.4\n");
            printf("Finished: %s\n", finishedNode->command);
            //printf("segfault check 5.5\n");
        }
    }
}

char * resumeAllJobsByGPID(int jobId, struct PidNode ** head){
    if (jobId == -1){
        printf("ERROR [BG]: There exists no stopped processes.\n");
        return NULL;
    }
    int targetGPID = getStoppedGPID(jobId, *head);
    if (targetGPID == -1){
        printf("ERROR [BG]: JobID Does Not Exist Or Is Not Stopped.\n");
        return NULL;
    }

    if(killpg(targetGPID, SIGCONT) == -1) {
        perror("CONTINUE GPID [main.c 302]");
        exit(EXIT_FAILURE);
    }
    char * finalCommand;
    struct PidNode * curNode = *head;
    while(curNode != NULL){
        if (curNode->gpid == targetGPID){
            curNode->status = JobRunning;
            finalCommand = curNode->command;
        }
        curNode = curNode->next;
    }
    return finalCommand;
}


/**
 * ==========================================
 * Function: parseInput
 * ==========================================
 * 
 * Runs the penn-shell shell
 * 
 * [params]:
 * 
 * [returns]: an integer
 */
int runShell() {
    struct PidNode * headProcess = NULL;
    while(1){
        //printf("=================\nRUNNING SHELL\n=================\n");
        int curGPID = -1;
        //stopFlag = 1;
        /*
        ================================
        1) Handle ^C
        ================================
        */
        if(signal(SIGINT, signalFunction) == SIG_ERR || signal(SIGTSTP, signalFunction) == SIG_ERR){ //TODO: REIMPLEMENT LATER
            printf("ERROR: SIG_ERR.\n");
            exit(EXIT_FAILURE);
        }
        int numBytes;

        char cmd[MAX_LINE_LENGTH];
        if(isatty(STDIN_FILENO)){ //CHECK IF PROMPT IS REDIRECTED
            /*
            ================================
            2) Print prompt and read in 
            from command line
            ================================
            */
            write(STDERR_FILENO, PROMPT, strlen(PROMPT));
            numBytes = read(STDIN_FILENO, cmd, MAX_LINE_LENGTH);
        } else {
            size_t bufsize = MAX_LINE_LENGTH;
            char *cmdPtr = cmd;
            numBytes = getline(&cmdPtr,&bufsize,stdin);
            if (numBytes == -1) {
                break;
            }
        }

        cmd[numBytes] = 0;

        /*
        ================================
        3) Handle ^D
        ================================
        */
        if(numBytes == 0) {
            if(isatty(STDIN_FILENO)){
                write(STDOUT_FILENO, "\n", 2);
            }
            exit(EXIT_SUCCESS);
        }

        struct parsed_command *pCmd;
        int parseCode = parse_command((char *) cmd, &pCmd);
        if(parseCode != 0){
            printf("Parse Error [%d]: Invalid Command\n", parseCode);
            continue;
        }
        
        //Handle Control-D With No Arguments
        if(cmd[strlen(cmd) - 1] != '\n'){
            write(STDOUT_FILENO, "\n", 2);
        }

        int jobFlag = 0;
        if(pCmd->num_commands == 1 && !pCmd->is_background) {
            char ** jobCtlArgs = pCmd->commands[0];
            int numCtrlArgs = 0;
            while (jobCtlArgs[numCtrlArgs] != NULL){
                numCtrlArgs++;
            }

            if (strcmp("fg", jobCtlArgs[0]) == 0) {
                if (numCtrlArgs > 2){
                    printf("ERROR: fg usage: fg [job_id]\n");
                }

                int removedGPID;
                int removedStatus;
                int * groupPids;
                int groupSize = 0;
                char * removedCommand;
                if (numCtrlArgs == 1) {
                    groupSize = removeLastGroup(&groupPids, &removedCommand, &removedGPID,  &headProcess, &removedStatus);
                } else if (numCtrlArgs == 2) {
                    int jobId = atoi(jobCtlArgs[1]);
                    if (jobId == 0){ //Note: atoi returns 0 if string is not a num
                        if (strcmp(jobCtlArgs[1], "0") == 0) {
                            printf("fg: no such id 0\n");
                        }
                    } else if (getNumGroupsTotal(&headProcess) < jobId) {
                        printf("ERROR: fg usage: fg %d\n", jobId);
                    } else {
                        groupSize = removeJobGroup(jobId, &removedCommand, &groupPids, &removedGPID, &headProcess, &removedStatus);
                    }
                }
                if (groupSize > 0){
                    if(killpg(removedGPID, SIGCONT) == -1) {
                        perror("CONTINUE GPID [main.c 155]");
                        exit(EXIT_FAILURE);
                    }
                    // tcsetpgrp(STDIN_FILENO, removedGPID);

                    //TODO: Check if running or stopped job --> if stopped, then output "Restarting"
                    if (removedStatus == JobStoppped) {
                        printf("Restarting: %s\n", removedCommand);
                    } else {
                        printf("%s\n", removedCommand);
                    }

                    signalPidArray = groupPids;
                    signalHandlerGPID = removedGPID;
                    signalHandlerCommand = removedCommand;
                    signalHeadProcess = headProcess;

                    if(waitForChildren(groupPids, groupSize, removedGPID, removedCommand, &headProcess) == -1){
                        free(pCmd);
                        perror("pidwait [main.c 326]\n");
                        exit(EXIT_FAILURE);
                    }
                }
                jobFlag = 1;
            } else if (strcmp("bg", jobCtlArgs[0]) == 0) {
                if (numCtrlArgs > 2){
                    printf("ERROR: bg usage: bg [job_id]\n");
                }

                char * runningCommand = NULL;
                if (numCtrlArgs == 1){
                    runningCommand = resumeAllJobsByGPID(getNumGroupsBG(&headProcess), &headProcess);
                } else if (numCtrlArgs == 2){
                    int jobId = atoi(jobCtlArgs[1]);
                    if (jobId == 0){ //Note: atoi returns 0 if string is not a num
                        if (strcmp(jobCtlArgs[1], "0") == 0) {
                            printf("bg: no such id 0\n");
                        }
                    } else if (getNumGroupsTotal(&headProcess) < jobId) {
                        printf("ERROR: bg usage: bg %d\n", jobId);
                    } else {
                        runningCommand = resumeAllJobsByGPID(jobId, &headProcess);
                    }
                    jobFlag = 1;
                }
                if (runningCommand != NULL){
                    printf("Running: %s\n", runningCommand);
                }

                jobFlag = 1;
            } else if (strcmp("jobs", jobCtlArgs[0]) == 0) {
                struct PidNode * curNode = headProcess;
                int groupId = -1;
                int listPos = 1;
                while(curNode != NULL){
                    int curGPID = curNode->gpid;
                    if (groupId != curGPID){
                        groupId = curGPID;
                        printf("[%d] %s (%s)\n", listPos, curNode->command, STATUS_STRING[curNode->status]);
                        listPos++;
                    }
                    curNode = curNode->next;
                }
                jobFlag = 1;
            }
        } else if (pCmd->num_commands == 1 && (strcmp("fg", pCmd->commands[0][0]) == 0 || strcmp("bg", pCmd->commands[0][0]) == 0)){
            printf("ERROR: fg usage: fg [job_id] || bg usage: bg [job_id]\n");
            jobFlag = 1;
        }
        if (!jobFlag){
            int * pidArr = (int *) malloc(pCmd->num_commands * sizeof(int));
            int ** fdArr = (int **) malloc((pCmd->num_commands) * sizeof(int *)); //NOTE: Write Error: Changed from num_commands - 1 to num_commands
            for(int i = 0; i < pCmd->num_commands; i++){
                fdArr[i] = (int *) malloc(2 * sizeof(int));
                pipe(fdArr[i]);
            }

            char * cleanCommand = getCleanCommand(pCmd);
            for(int i = 0; i < pCmd->num_commands; i++){
                if(executeCommand(i, pCmd, pidArr, fdArr, &curGPID, &headProcess, cleanCommand) == -1) {
                    continue;
                }
            }
            setChildrenPGID(pidArr, pCmd->num_commands, curGPID);
            if (pCmd->is_background) {
                signalPidArray = NULL;
                signalHandlerGPID = -1;
                signalHandlerCommand = NULL;
                signalHeadProcess = NULL;
            } else {
                signalPidArray = pidArr;
                signalHandlerGPID = curGPID;
                signalHandlerCommand = cleanCommand;
                signalHeadProcess = headProcess;
            }

            //Close all parent pipes
            closePipeWrites(fdArr, pCmd->num_commands - 1);

            //Parent logic
            if(!pCmd->is_background) {
                // printf("waiting for children\n");
                if(waitForChildren(pidArr, pCmd->num_commands, curGPID, cleanCommand, &headProcess) == -1){
                    free(pCmd);
                    perror("pidwait");
                    exit(EXIT_FAILURE);
                }
            }
            //printf("segfault check 1\n");
            for (int i = 0; i < pCmd->num_commands; i++){
                //printf("segfault check 2\n");
                free(fdArr[i]);
            }
            free(fdArr);
            free(pidArr);
            free(cleanCommand);
        }

        //Check on background processes
        //printf("segfault check 5\n");
        waitForBackgroundProcess(&headProcess);
        //printf("segfault check 6\n");
        free(pCmd);
        free(headProcess);
        //printf("segfault check 7\n");
    }
    //printf("segfault check 8\n");
    return 0;
}



/**
 * ==========================================
 * Function: main
 * ==========================================
 * 
 * The entry point for penn-shell
 * 
 * [params]:
 * int argc: number of arguments
 * cahr **argv: the arguments
 * 
 * [returns]: an integer
 */
int main(int argc, char **argv) {
    if (argc == 1) {
        runShell();
    } else {
        char errorPrompt[] = "ERROR. Usage: penn-shell\n";
        int errorPromptLength = strlen(errorPrompt);
        write(STDERR_FILENO, errorPrompt, errorPromptLength);
        exit(EXIT_FAILURE);
    }
}
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

static pid_t pid;
static int cntrlC;

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
void signalFunction(int signo) {
    int promptLength = strlen(PROMPT);
    write(STDERR_FILENO, "\n", 2);
    write(STDERR_FILENO, PROMPT, promptLength);
    cntrlC = 1;
}



/**
 * ==========================================
 * Function: signalHandler
 * ==========================================
 * 
 * kills the process
 * 
 * [params]:
 * int signo: the signal number
 * 
 * [returns]: void
 */
void signalHandler(int signo) {
    if (signo == SIGALRM) {
        kill(pid, SIGKILL);
    }
}



/**
 * ==========================================
 * Function: parseInput
 * ==========================================
 * 
 * removes whitespace form penn-shredder input
 * and returns the tokens
 * 
 * [params]:
 * char cmd[]: the command fed to penn-shredder
 * 
 * [returns]: a char** (representing a list of strings)
 * which contains pointers to each of the parameters
 * that are given to penn-shredder through the command line.
 * These parameters are stripped of white space.
 */
char **parseInput(char cmd[]) {
    /*
    ================================
    1) Initialize token pointers
    ================================
    */
    const char delimiters[] = " \t\n";
    char *tokens = NULL; 
    char *copyTokens = NULL;

    /*
    ================================
    2) Copy cmd string
    ================================
    */
    int size = strlen(cmd);
    char *cmdCopy = (char *) malloc(size + 1);
    strCpy(cmdCopy, cmd);

    /*
    ================================
    3) Strip whitespace and tokenize 
    cmd string to count the number of 
    tokens given to penn-shredder
    ================================
    */
    tokens = strtok(cmd, delimiters);
    int numTokens = 0;
    while( tokens != NULL ) {
        numTokens++;
        tokens = strtok(NULL, delimiters);
    }

    /*
    ================================
    4) Handle just pressing enter
    ================================
    */
    if (numTokens == 0){
        free(cmdCopy);
        return NULL;
    }

    /*
    ================================
    5) Malloc the space for the 
    double pointer that will be returned
    to runShredder()
    ================================
    */
    char **cleanedTokens =  malloc((numTokens + 1)*sizeof(char*));

    /*
    ================================
    6) Tokenize to coppied cmd string,
    malloc space for each token, and
    insert the tokens into 
    char **cleanedTokens
    ================================
    */
    int cleanedTokensIndex = 0;
    copyTokens = strtok(cmdCopy, delimiters);

    int size2 = strlen(copyTokens);
    char *copyTokensFirst = (char *) malloc(size2 + 1);
    strCpy(copyTokensFirst, copyTokens);

    cleanedTokens[cleanedTokensIndex] = copyTokensFirst;
    while( copyTokens != NULL ) {
        cleanedTokensIndex++;
        copyTokens = strtok(NULL, delimiters);
        if (copyTokens == NULL) {
            break;
        }
        int size3 = strlen(copyTokens);
        char *copyTokensNext = (char *) malloc(size3 + 1);
        strCpy(copyTokensNext, copyTokens);
        cleanedTokens[cleanedTokensIndex] = copyTokensNext;
    }

    cleanedTokens[cleanedTokensIndex] = NULL;
    
    free(cmdCopy);

    return cleanedTokens;
}



/**
 * ==========================================
 * Function: freeArray
 * ==========================================
 * 
 * frees the memory malloced for a double pointer
 * 
 * [params]:
 * char ** arr: the array
 * 
 * [returns]: void
 */
void freeArray(char ** arr){
    int arrIndex = 0;
        
    while (arr[arrIndex] != NULL) {
        free(arr[arrIndex]);
        arrIndex++;
    }

    free(arr);
}



/**
 * ==========================================
 * Function: parseInput
 * ==========================================
 * 
 * Runs the penn-shredder shell
 * 
 * [params]:
 * 
 * [returns]: an integer
 */
int runShredder() {
    while (1) {
        /*
        ================================
        1) Handle ^C
        ================================
        */
        if(signal(SIGINT, signalFunction) == SIG_ERR){
            printf("ERROR: SIG_ERR.");
            exit(EXIT_FAILURE);
        }

        /*
        ================================
        2) Print prompt and read in 
        from command line
        ================================
        */
        int promptLength = strlen(PROMPT);
        if (cntrlC != 1) {
            //printf("printing prompt again (cntrlC = 0)\n");
            write(STDERR_FILENO, PROMPT, promptLength);
        }
        const int maxLineLength = 4096;
        char cmd[maxLineLength];

        for (int index = 0; index < maxLineLength; index++) {
            cmd[index] = 0;
        }

        read(STDIN_FILENO, cmd, maxLineLength);

        /*
        ================================
        3) Handle ^D
        ================================
        */
        if(strlen(cmd) == 0) {
            cntrlC = 0;
            write(STDOUT_FILENO, "\n", 2);
            exit(EXIT_SUCCESS);
        }

        /*
        ================================
        4) parse the input to cmd (remove
        whitespace and get parameters)
        ================================
        */
        char **newargv = parseInput(cmd);

        /*
        ================================
        5) Handle just pressing enter
        ================================
        */
        if (newargv == NULL) {
            continue;
        }

        /*
        ================================
        6) Fork and execute
        ================================
        */
        pid = fork();
        if (pid == -1) {
            freeArray(newargv);
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            cntrlC = 0;
            char *newenviron[] = { NULL };
            int execveErrorChecker = execve(newargv[0], newargv, newenviron);
            if (execveErrorChecker == -1) {
                freeArray(newargv);
                perror("execve");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            
            int returnStatus = 0;

            pid_t pidWait;
            
            do {
                pidWait = wait(&returnStatus);

                //handle errors
                if (pidWait == -1){
                    freeArray(newargv);
                    perror("pidwait");
                    exit(EXIT_FAILURE);
                }
                
                //if child signaled and died
                else if (WIFSIGNALED(returnStatus) && cntrlC != 1) {
                    write(STDOUT_FILENO, CATCHPHRASE, strlen(CATCHPHRASE));
                }
            } while(!WIFEXITED(returnStatus) && !WIFSIGNALED(returnStatus));

            cntrlC = 0;
        }

        /*
        ================================
        7) Free memory from newargv (free
        the pointers within newargv then 
        free newargv itself)
        ================================
        */
       freeArray(newargv);
        
    }
    return 0;
}



/**
 * ==========================================
 * Function: main
 * ==========================================
 * 
 * The entry point for penn-shredder
 * 
 * [params]:
 * int argc: number of arguments
 * cahr **argv: the arguments
 * 
 * [returns]: an integer
 */
int main(int argc, char **argv) {  
    if (argc == 1) {
        runShredder();
    } else {
        char errorPrompt[] = "Usage: penn-shredder\n";
        int errorPromptLength = strlen(errorPrompt);
        write(STDERR_FILENO, errorPrompt, errorPromptLength);
        exit(EXIT_FAILURE);
    }
}
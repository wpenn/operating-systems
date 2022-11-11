# 21fa-project-0-wpenn

## Name and PennKey
* Name: Wesley Penn
* Pennkey: wespenn

## List of Submitted Source Files
* main.c
* main.h
* shredderUtilities.c
* shredderUtilities.h

## Overview of Work Accomplished
The attached project is a shell that executes files containing typical terminal commands. The shell can take in arguments of execution timeout, which kills the executable in a defined amount of time. Control-C kills the executable process but does not exit the shell. Control-D exits the shell at the beginning of the input line and executes the line if non-empty. The code dynamically handles memory without leaks or memory violations. It also handles errors accross System calls and Library functions: *execve*, *fork*, *kill*, *read*, *signal*, *wait*, and *malloc*. 

## Description of Code and Code Layout
The code is split between the main function in **main.c** which runs the shell, and the helper functions in **shredderUtilities.c**. The header files are also included for both c files, which contain definitions of the functions and descriptions.

In **main.c**, the main method is in charge of processing the arguments for Execution Timeout, then runs the program until finished. The **runProgram** function runs one cycle of the shell (prompt then command execution); It reads in *STDIN* and passes it to the **ExecuteCommand** method. The **ExecuteCommand** method takes in a command string argument, removes unwanted characters, processes it into execution arguments, and executes the corresponding exexutable file (with appropriate additional arguments).

In **shredderUtilities.c**, all the methods are helper functions used in **main.c**. The **getNumArgs** takes in a command string, trims it for unwanted characters, and returns the number of command arguments for the execution. The **stripCpy** takes in a destination and source string, trims the source string of unwanted characters, copies the source string into the destination, and returns a boolean integer if the string ends in a newline. Note that the boolean is needed for Control-D behavior where the EOF is not from a newline. The **isdigitImpl** is an implementation of the **isdigit** function from the C Standard *stdio* library.
/*
Takes in Command String
Returns Number of Command Arguments
*/
int getNumArgs(char * command);

/*
Copies Source String into Destination
Replaces newlines and tabs with spaces
Returns 0 if the end of source is a newline and 1 otherwise
*/
int stripCpy(char * destination, char * source);

/*
Implementation of isdigit function
*/
int isdigitImpl(int c);
/*
Runs the Penn Shredder Program
Reads STDIN, Passes STDIN to ExecuteCommand
*/
void runProgram();


/*
Takes in Command String
Handles Processing Command String into Execution Arguments
Executes Command
*/
void executeCommand(char * command);


/*
Handles SIGALARM in Parent Process
*/
void sigAlarmHandler(int signum);

/*
Handles SIGINT in Main Program
*/
void sigIntHandler(int signum);
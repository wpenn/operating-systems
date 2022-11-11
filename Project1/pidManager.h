enum Status {
    JobRunning = 0,
    JobDone = 1,
    JobStoppped = 2,
    JobSignaled = 3
};

static const char * STATUS_STRING[] = {
    "running",
    "done",
    "stopped",
    "signal"
};

struct PidNode{
    int pid; //Key
    int gpid;
    enum Status status;
    char * command;
    struct PidNode * next;
};

/**
 * Insert New PID Node to Front of List
 */
void insertJob(int pid, int gpid, enum Status status, char *command, struct PidNode ** head);

/**
 * Remove PID from Linked List
 * Returns the Removed Node
 */
struct PidNode * removePID(int pid, struct PidNode ** head);

int removeLastGroup(int ** pidArr, char ** command, int * finalGPID, struct PidNode ** head, int * status);

int removeJobGroup(int job_id, char ** command, int ** pidArr, int * finalGPID, struct PidNode ** head, int * status);

int runLastGroup(int ** pidArr, char ** command, int * finalGPID, struct PidNode ** head, int * status);

int getGroupSize(int job_id, struct PidNode ** head);

int getNumGroups(struct PidNode ** head);

int getNumGroupsBG(struct PidNode ** head);

int getNumGroupsTotal(struct PidNode ** head);

int getStoppedGPID(int jobId, struct PidNode * head);

/**
 * Check if PIDs group is finished
 * Returns 1 if Group of PID is finished, 0 otherwise
 */
int groupFinished(struct PidNode * targetNode, struct PidNode * head);

/**
 * Get Length of List
 */
int getListLength(struct PidNode * head);

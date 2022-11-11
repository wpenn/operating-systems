#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "pidManager.h"

void insertJob(int pid, int gpid, enum Status status, char *command, struct PidNode ** head){
    //printf("INSIDE INSERT JOB 1 \n");
    struct PidNode *newNode = (struct PidNode *) malloc(sizeof(struct PidNode));
    //printf("INSIDE INSERT JOB 2 \n");
    newNode->pid = pid;
    newNode->gpid = gpid;
    newNode->status = status;
    newNode->command = command;
    newNode->next = NULL;
    //printf("INSIDE INSERT JOB 3 \n");
    struct PidNode * curNode = *head;
    if (curNode == NULL){
        *head = newNode;
        return;
    }
    //printf("INSIDE INSERT JOB 4 \n");
    while (curNode->next != NULL){
        //printf("INSIDE INSERT JOB 4.1 \n");
        curNode = curNode->next;
        //printf("INSIDE INSERT JOB 4.2 \n");
    }
    //printf("INSIDE INSERT JOB 5 \n");
    curNode->next = newNode;
}

struct PidNode * removePID(int pid, struct PidNode ** head) {
    //printf("segfault check 5.4.1.0\n");
    if(*head == NULL){
        return NULL;
    }
    //printf("segfault check 5.4.1\n");
    struct PidNode * curNode = *head;
    struct PidNode * lastNode = NULL;
    //printf("segfault check 5.4.2\n");

    while(curNode->pid != pid) {
        //printf("segfault check 5.4.3\n");
        if(curNode->next == NULL){
            return NULL;
        }
        lastNode = curNode;
        curNode = curNode->next;
    }
    //printf("segfault check 5.4.4\n");
    if (curNode == *head){
        *head = (*head)->next;
    } else {
        lastNode->next = curNode->next;
    }
    //printf("segfault check 5.4.5\n");
    return curNode;
}

/**
 * Remove First PGID Group, fill pidArr array (which will contain 
 * the pids of the commands with the same gpid), return length of groupPids
*/
int removeLastGroup(int ** pidArr, char ** command, int * finalGPID, struct PidNode ** head, int * status) {
    int numGroups = getNumGroups(head);
    //printf("[pidManager.c] Num Groups: %d\n", numGroups); //Testing
    return removeJobGroup(numGroups, command,  pidArr, finalGPID, head, status);
}

/**
 * Remove job_id Group from head, return length of groupPids
*/
int removeJobGroup(int job_id, char ** command, int ** pidArr, int * finalGPID, struct PidNode ** head, int * status) {
    int groupSize = getGroupSize(job_id, head);
    if (groupSize == 0){
        return 0;
    }

    *pidArr = malloc(sizeof(int *) * groupSize);
    struct PidNode * curNode = *head;
    struct PidNode * lastNode = NULL;

    int curJobId = 0;
    int gpid = -1;
    int pidArrIndex = 0;
    while(job_id >= curJobId && curNode != NULL){
        if (gpid != curNode->gpid){
            curJobId++;
            gpid = curNode->gpid;
        }
        if (job_id != curJobId){
            lastNode = curNode;
        } else {
            //add to pidArr, set gpid
            *finalGPID = curNode->gpid;
            *status = curNode->status;
            (*pidArr)[pidArrIndex] = curNode->pid;
            pidArrIndex++;
            *command = curNode->command;
            if (job_id == 1){
                *head = (*head)->next;
            } else {
                if (curNode == NULL) {
                    lastNode->next = NULL;
                } else {
                    lastNode->next = curNode->next;
                }
            }
        }
        curNode = curNode->next;
    }
    
    return groupSize;
}


int runLastGroup(int ** pidArr, char ** command, int * finalGPID, struct PidNode ** head, int * status) {
    int numGroups = getNumGroupsBG(head);
    //printf("[pidManager.c] Num Groups: %d\n", numGroups); //Testing
    return removeJobGroup(numGroups, command, pidArr, finalGPID, head, status);
}

int getGpidFromLastGroup(int job_id, struct PidNode ** head) {
    return 0; // gpid; TODO:
}

int getGroupSize(int job_id, struct PidNode ** head){
    int curJobId = 0;
    int groupSize = 0;

    struct PidNode * curNode = *head;
    int gpid = -1;
    while(curNode != NULL){
        if (gpid != curNode->gpid){
            curJobId++;
            gpid = curNode->gpid;
        }
        if (job_id == curJobId){
            groupSize++;
        } else if (curJobId > job_id){
            return groupSize;
        }
        curNode = curNode->next;
    }
    return groupSize;
}

int getStoppedGPID(int jobId, struct PidNode * head) {
    int curJobId = 0;
    struct PidNode * curNode = head;
    int gpid = -1;
    while(curNode != NULL){
        if (gpid != curNode->gpid){
            curJobId++;
            gpid = curNode->gpid;
        }
        if (jobId == curJobId){
            if (curNode->status == JobStoppped){
                return gpid;
            } else {
                return -1;
            }
        }
        curNode = curNode->next;
    }
    return -1;
}

int getNumGroupsBG(struct PidNode ** head) {
    struct PidNode * curNode = *head;
    int len = 0;
    int highestStopIndex = -1;
    int groupPid = -1;
    //int finalgpid = -1;
    while(curNode != NULL){
        if (groupPid != curNode->gpid){
            len++;
            groupPid = curNode->gpid;
            if (curNode->status == 2) {
                highestStopIndex = len;
                
            }
        }
        curNode = curNode->next;
    }
    return highestStopIndex; //returning gpid
}

int getNumGroups(struct PidNode ** head){
    struct PidNode * curNode = *head;
    int len = 0;
    int highestStopIndex = 0;
    int groupPid = -1;
    while(curNode != NULL){
        if (groupPid != curNode->gpid){
            len++;
            groupPid = curNode->gpid;
            if (curNode->status == 2) {
                highestStopIndex = len;
            }
        }
        curNode = curNode->next;
    }
    if (highestStopIndex > 0) {
        return highestStopIndex;
    }
    return len;
}

int getNumGroupsTotal(struct PidNode ** head){
    struct PidNode * curNode = *head;
    int len = 0;
    int groupPid = -1;
    while(curNode != NULL){
        if (groupPid != curNode->gpid){
            len++;
            groupPid = curNode->gpid;
        }
        curNode = curNode->next;
    }
    return len;
}

int groupFinished(struct PidNode * targetNode, struct PidNode * head){
    int targetGPID = targetNode->gpid;
    struct PidNode * curNode = head;
    while(curNode != NULL){
        if (targetGPID == curNode->gpid){
            return 0;
        }
        curNode = curNode->next;
    }
    return 1;
}

int getListLength(struct PidNode * head) {
    struct PidNode * curNode = head;
    int len = 0;
    while(curNode != NULL){
        len++;
        // printf("GET LIST LENGTH ITEM: %s || %s \n", curNode->command, STATUS_STRING[curNode->status]);
        curNode = curNode->next;
    }
    return len;
}
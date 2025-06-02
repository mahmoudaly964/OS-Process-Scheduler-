#include <stdio.h>
#include <stdlib.h>
// Node structure
typedef struct ProcessNode {
    int arrivalTime;
    int runTime;
    int priority;
    int responseTime;
    int finishTime;
    int remainingTime;
    int pid;
    int id;
   
    struct ProcessNode* next;
} ProcessNode;

// Initialize a new process node
ProcessNode* initializeProcessNode(int id, int arrivalTime, int runTime, int priority) {
    ProcessNode* newNode = (ProcessNode*)malloc(sizeof(ProcessNode));
    if (!newNode) {
        return NULL;
    }
    newNode->arrivalTime = arrivalTime;
    newNode->runTime = runTime;
    newNode->priority = priority;
    newNode->responseTime = -1;  // -1 mark if responseTime  is set or not
    newNode->finishTime = -1;
    newNode->remainingTime = runTime;
    newNode->pid = -1;
    newNode->id= id;

    newNode->next = NULL;
    return newNode;
}
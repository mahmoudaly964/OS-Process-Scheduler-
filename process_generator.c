#include "headers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include "DataStructures.h"

Node *processList = NULL;  // Start with an empty linked list
int processCount = 0;  // Keep track of the number of processes
int schedulerPid;

// Function to clear resources when exiting
void clearResources(int signum) {
    key_t msgKey = ftok("keyFile", 65);
    int msgq = msgget(msgKey, 0666);
    if (msgq != -1) {
        msgctl(msgq, IPC_RMID, NULL); // Remove message queue
        printf("Message queue removed successfully\n");
    } else {
        printf("Message queue not found or already removed\n");
    }

    // Free the linked list
    Node *temp;
    while (processList != NULL) {
        temp = processList;
        processList = processList->next;
        free(temp);
    }

    // Destroy clock resources
    destroyClk(true);

    exit(0);
}

// Function to read the input file
void readInputFile(const char *filename, Node **processList, int *processCount) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    *processCount = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;  // Skip comment lines

        Node *newNode;
        int id, arrivalTime, runTime, priority, memorySize;

        // Read the process details, including memory size
        sscanf(line, "%d %d %d %d %d", &id, &arrivalTime, &runTime, &priority, &memorySize);

        // Allocate memory for newNode and its data
        newNode = malloc(sizeof(Node));
        if (!newNode) {
            printf("Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }

        // Initialize the process data and assign to newNode
        newNode->data = *Process(id, arrivalTime, runTime, priority,memorySize);

        newNode->next = NULL;

        // Insert the new node at the end of the list
        if (*processList == NULL) {
            *processList = newNode;
        } else {
            Node *temp = *processList;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = newNode;
        }
        (*processCount)++;
    }

    fclose(file);
}


// Function to read the chosen algorithm and quantum time
void readAlgo(int *algorithmChosen, int *quantumTime, int argc, char *argv[]) {
    if (argc < 4) {
        printf("Not enough arguments. Usage: ./process_generator.o testcase.txt -sch <algo_number> -q <quantum>\n");
        exit(EXIT_FAILURE);
    }

    // Initialize quantumTime to a sentinel value
    *quantumTime = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-sch") == 0) {
            *algorithmChosen = atoi(argv[i + 1]);
            if (*algorithmChosen < 1 || *algorithmChosen > 4) {
                printf("Invalid scheduler type. Must be between 1 and 4.\n");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(argv[i], "-q") == 0) {
            *quantumTime = atoi(argv[i + 1]);
            if (*quantumTime <= 0) {
                printf("Quantum time must be greater than 0.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    if ((*algorithmChosen == 3 || *algorithmChosen == 4) && *quantumTime == -1) {
        printf("Quantum time is required for Round Robin (Scheduler 3) and Multilevel Queue (Scheduler 4).\n");
        exit(EXIT_FAILURE);
    }
}


// Function to fork the clock and scheduler processes
void forkClkAndScheduler(char *algorithm, char *quantum, char *countProcesses) {
    int clkPid = fork();
    if (clkPid == -1) {
        perror("Error in fork");
        exit(EXIT_FAILURE);
    } else if (clkPid == 0) {
        execl("./clk.out", "clk.out", NULL);
        perror("Error in execl for clk");
        exit(EXIT_FAILURE);
    }
    sleep(1); // Ensure the clock process starts first

    // Fork another child process to execute the scheduler process
    schedulerPid = fork();
    if (schedulerPid == -1) {
        perror("Error in forking");
        exit(EXIT_FAILURE);
    } else if (schedulerPid == 0) {
        execl("./scheduler.out", "scheduler.out", algorithm, quantum, countProcesses, NULL);
        perror("Error in execl for scheduler");
        exit(EXIT_FAILURE);
    }
}

// Function to send a process to the scheduler via message queue
void sendToScheduler(int msqid, Node *process) {
    struct msgbuff message;
    message.mtype = 1;  // Set message type (could be adjusted for different types)
    message.process = process->data;
    /*
    message.process.id = process->data.id;
    message.process.arrivalTime = process->data.arrivalTime;
    message.process.runTime = process->data.runTime;
    message.process.priority = process->data.priority;
    */
    if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("Error in msgsnd");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, clearResources);

    // Generate key for the message queue
    key_t msgKey = ftok("keyFile", 65);
    // Create or get the message queue if it is already created before
    int msgq = msgget(msgKey, IPC_CREAT | 0666);
    if (msgq == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }



    // 1. Read the input files
    readInputFile("processes.txt", &processList, &processCount);

    // 2. Read the chosen scheduling algorithm and its parameters
    int algorithmChosen = -1, quantumTime = -1;
    readAlgo(&algorithmChosen, &quantumTime, argc, argv);

    // 3. Initiate and create the scheduler and clock processes
    char algoChosenString[10], quantumTimeString[10], processesCountString[10];
    sprintf(algoChosenString, "%d", algorithmChosen); // Convert int to string
    sprintf(quantumTimeString, "%d", quantumTime);
    sprintf(processesCountString, "%d", processCount);
    forkClkAndScheduler(algoChosenString, quantumTimeString, processesCountString);

    // 4. Initialize the clock
    initClk();


    Node *currentProcess = processList;


    while (currentProcess != NULL) {
        int currentTime = getClk(); // Get the current clock time
        if (currentProcess->data.arrivalTime == currentTime) {
            sendToScheduler(msgq, currentProcess); // Send the process
            printf("Sending process %d to the scheduler at time = %d\n", currentProcess->data.id, currentTime);
            currentProcess = currentProcess->next; // Move to the next process
        }
    }

    int status;
    waitpid(schedulerPid, &status, 0); // Wait for scheduler to finish
    printf("Scheduler terminated. Cleaning up resources...\n");

    // 7. Cleanup and exit
    clearResources(0);
    return 0;
}

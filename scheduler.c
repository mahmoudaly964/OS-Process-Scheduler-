#include "headers.h"
#include <errno.h>
#include "math.h"
#include "DataStructures.h"
#define decrementTime SIGUSR1
#define MEMORY_SIZE 1024



void PrintPerf(FILE *fout, float util, float totalWTA, int totalWait, int ProcNum, float *WTA);
void PrintPer(FILE *fout, float util, float totalWTA, int totalWait, int ProcNum);
void SJF(FILE *fptr, FILE *fout);
void RR(FILE *fptr, FILE *fout,int quantum);
void HPF(FILE *fptr, FILE *fout, MemoryBlock *rootMemoryBlock);
void MultiLevel(FILE *fptr, FILE *fout, int quantum);

int msgq_id_generator ;
int remainingProcesses;
int quantumCount=0;
int ProcessNum;
MemoryBlock*rootMemoryBlock;

struct Process runningProcess;

void runProcess()
{
    kill(runningProcess.pid, SIGCONT);
    kill(runningProcess.pid, decrementTime);
    runningProcess.remainingTime--;
    printf("decrementing process with id %d\n",runningProcess.id);

}

struct Process recieveProcess()
{
    struct msgbuff message;
    message.process.id = -1;
    message.process.pid = -1;

    // Use IPC_NOWAIT to avoid blocking if no message is present
    if (msgrcv(msgq_id_generator, &message, sizeof(message.process), 1, IPC_NOWAIT) == -1)
    {
        // Check if the error is due to no messages
        if (errno == ENOMSG)
        {
            // No message in the queue; return a process with id = -1
            return message.process;
        }
        else
        {
            perror("Error in receiving process from message queue");
            exit(-1);
        }
    }

    if (message.process.id != -1)
    {
        printf("Scheduler: received process with id: %d at time = %d \n", message.process.id, getClk());
    }

    return message.process;
}



int main(int argc, char *argv[])
{
    //TODO: upon termination release the clock resources.
    initClk();
    //varibles
    int algoNumber = atoi(argv[1]);
    int quantum = atoi(argv[2]);
    ProcessNum= atoi(argv[3]);
    remainingProcesses = ProcessNum;
    rootMemoryBlock=initializeMemoryBlock(0,MEMORY_SIZE);

    runningProcess.id=-1;
    runningProcess.arrivalTime=-1;
    int pid;
    float totalWTA = 0.0;
    int totalWait = 0;
    int totalUtil = 0;

    float *WTA = (float *)malloc(ProcessNum * sizeof(float));

    //Files
    FILE *fptr = fopen("scheduler.log", "w");
    FILE *fout = fopen("scheduler.perf", "w");
    FILE *fmem = fopen("memory.log", "w");
    if (fptr == NULL) {
        printf("Unable to open the log file.");
    }
    if (fout == NULL) {
        printf("Unable to open the perf file.");
    }
    if (fmem == NULL) {
        printf("Unable to open the mem file.");
    }

    //init IPC between schedular and generator
    key_t key_id;
    int send_val; int rec_val;
    key_id = ftok("keyFile", 65);
    if (key_id == -1) {
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }
    msgq_id_generator = msgget(key_id, 0666); // get queue id
    if (msgq_id_generator == -1) {
        perror("Error in msgget");
        exit(EXIT_FAILURE);
    } else {
        printf("Message Queue ID: %d\n", msgq_id_generator);  // Print the message queue ID
    }

    //Choosing the Algorithm    
    if (algoNumber == 1) // SJF
    {
        SJF(fptr,fout);
    }

    else if (algoNumber == 2) //HPF
    {
        /* code */
        printf("HPF: %d\n", ProcessNum);
        HPF(fptr, fout, rootMemoryBlock);
    }
    else if (algoNumber == 3) // RR
    {
        RR(fptr,fout,quantum);

    }
    else if (algoNumber == 4) //Multiple level Feedback Loop 
    {
        MultiLevel(fptr, fout, quantum);
    }     

    fclose(fptr);
    fclose(fout);
    printf("Scheduler Terminated\n");

    destroyClk(false);
}

void PrintPer(FILE *fout, float util, float totalWTA, int totalWait, int ProcNum) {
    double sum = 0.0;
    fprintf(fout, "CPU utitilization = %.2f%%\n", util);

    fprintf(fout, "Avg WTA = %.2f\n", totalWTA / ProcNum);
    fprintf(fout, "Avg Waiting = %.2f\n", (float)totalWait / ProcNum);

}

void PrintPerf(FILE *fout, float util, float totalWTA, int totalWait, int ProcNum, float *WTA) {
    double sum = 0.0;
    fprintf(fout, "CPU utitilization = %.2f%%\n", util);

    fprintf(fout, "Avg WTA = %.2f\n", totalWTA / ProcNum);
    fprintf(fout, "Avg Waiting = %.2f\n", (float)totalWait / ProcNum);
    for (int i = 0; i < ProcNum; i++) {
        sum += (WTA[i] - totalWTA / ProcNum) * (WTA[i] - totalWTA / ProcNum);
    }
}


void RR(FILE *fptr, FILE *fout, int quantum) {
    Queue readyQueueRR;
    initializeQueue(&readyQueueRR); // Initialize queue

    int totalWait = 0;
    float totalWTA = 0;
    int totalExecutionTime=0;
    int ProcessNum = remainingProcesses; // Total number of processes
    float WTA[ProcessNum]; // Array to store WTA for each process
    int totalIdle = 0;
    int prevClk = 0; // Track the previous clock tick
    Queue waitingList;
    initializeQueue(&waitingList);
    while (remainingProcesses > 0) {
        int clk = getClk(); // Get the current system clock
        if (clk > prevClk) {
            printf("Entering loop at time %d...\n", clk);

            // Handle new arrivals
            struct Process newProcess;
            while ((newProcess = recieveProcess()).id != -1) {
                newProcess.remainingTime = newProcess.runTime;
                newProcess.startTime = -1;
                newProcess.waitingTime = 0;
                newProcess.arrivalTime = clk;
                totalExecutionTime+=newProcess.runTime;

                pid_t pid = fork();
                if (pid == 0) {
                    char str[10], id[10];
                    sprintf(str, "%d", newProcess.remainingTime);
                    sprintf(id, "%d", newProcess.id);
                    execl("./process.out", "process.out", str, id, NULL);
                    perror("execl failed");
                    exit(EXIT_FAILURE);
                } else {
                    
                    newProcess.pid = pid;
                     // Check if memory is available
                    MemoryBlock* mem=addProcess(rootMemoryBlock, &newProcess);
                    if (mem!=NULL) {
                        push(&readyQueueRR, newProcess); // Push to ready queue only if memory is allocated
                    } else {
                        push(&waitingList, newProcess); // Push to waiting list if memory is not available
                        printf("At time %d process %d not enough memory. Added to waiting list.\n", clk, newProcess.id);
                    }
                    kill(newProcess.pid, SIGSTOP); // Stop the new process initially
                }
            }


            // Handle the running process
            if (runningProcess.id != -1) {
                if (quantumCount == quantum || runningProcess.remainingTime == 0) {
                    if (runningProcess.remainingTime == 0) {
                        // Process finished
                        int finishTime = getClk();
                        int turnaroundTime = finishTime - runningProcess.arrivalTime;
                        float WTA_val = (float)turnaroundTime / runningProcess.runTime;
                        totalWTA += WTA_val;
                        totalWait+=turnaroundTime-runningProcess.runTime;
                        WTA[runningProcess.id] = WTA_val;
                        runningProcess.lastTimeToRun=clk;

                        fprintf(fptr,"At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f\n",
                               finishTime, runningProcess.id, runningProcess.arrivalTime,
                               runningProcess.runTime, 
                               finishTime - runningProcess.arrivalTime - runningProcess.runTime,
                               turnaroundTime, WTA_val);

                        waitpid(runningProcess.pid, NULL, 0); // Clean up the process
                         if(updateMemory(rootMemoryBlock, &runningProcess)){
                        // Free memory block used by the process
                        //logMemoryEvent(currentClk, &runningProcess, rootMemoryBlock, "free");
                        printf("out of update memory");
                    }
                        remainingProcesses--;
                        runningProcess.id = -1; // Reset running process
                        quantumCount = 0;
                        while (!isEmpty(&waitingList)) {
    struct Process *frontProcess = frontQueue(&waitingList); // Peek at the front process
    MemoryBlock* mem = addProcess(rootMemoryBlock, frontProcess);
    if (mem != NULL) {
        // Successfully allocated memory, move to the ready queue
        push(&readyQueueRR, *frontProcess);
        printf("At time %d process %d allocated memory and moved to ready queue.\n", getClk(), frontProcess->id);
        pop(&waitingList); // Remove the process from the waiting list
    } else {
        // Not enough memory; stop checking further
        break;
    }
}


                    } else if (quantumCount == quantum) {
                        // Time slice expired
                        runningProcess.lastTimeToRun=clk;
                        fprintf(fptr,"At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                               clk, runningProcess.id, runningProcess.arrivalTime, runningProcess.runTime,
                               runningProcess.remainingTime,
                               runningProcess.waitingTime);
                        kill(runningProcess.pid, SIGSTOP); // Pause the process
                        push(&readyQueueRR, runningProcess); // Requeue the process
                        runningProcess.id = -1;
                        quantumCount = 0;
                    }
                } else {
                    quantumCount++;
                    runProcess(); // Continue running the process
                }
            }

            // Schedule a new process
            if (runningProcess.id == -1 && !isEmpty(&readyQueueRR)) {
                runningProcess = pop(&readyQueueRR);

                if (runningProcess.startTime == -1) {
                    // First-time execution

                    runningProcess.startTime = clk;
                    runningProcess.waitingTime=clk-runningProcess.arrivalTime;
                    fprintf(fptr,"At time %d process %d started arr %d total %d remain %d wait %d\n",
                           clk, runningProcess.id, runningProcess.arrivalTime, runningProcess.runTime,
                           runningProcess.remainingTime, runningProcess.waitingTime);
                } else {
                    // Resuming a paused process
                    runningProcess.waitingTime+=clk-runningProcess.lastTimeToRun;
                    fprintf(fptr,"At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                           clk, runningProcess.id, runningProcess.arrivalTime, runningProcess.runTime,
                           runningProcess.remainingTime, runningProcess.waitingTime);
                }

                runProcess(); // Start or resume the process
                quantumCount = 1; // Reset quantum count
            }

            prevClk = clk; // Update clock reference
        }
    }

    int finishTime = getClk();
    
    float totalUtil = ((float)totalExecutionTime / (finishTime-1)) * 100;
    // Print performance metrics
    PrintPerf(fout, totalUtil, totalWTA, totalWait, ProcessNum, WTA);
}


void HPF(FILE *fptr, FILE *fout, MemoryBlock *memoryRoot) {
    PriorityQueue readyQueue;
    initializePriorityQueue(&readyQueue);

    Queue waitingQueue;
    initializeQueue(&waitingQueue);
    int totalWait = 0;
    float totalWTA = 0;
    int ProcessNum = remainingProcesses; // Total number of processes
    float WTA[ProcessNum]; // Array to store WTA for each process
    int prevClk = getClk(); // Previous clock for idle calculation
    int totalExecutionTime = 0;

    while (remainingProcesses > 0) {
        int clk = getClk();

        // Handle new arrivals
        while (1) {
            struct Process newProcess = recieveProcess();
            if (newProcess.id == -1)
                break;
            
            // Initialize fields for the new process
            newProcess.remainingTime = newProcess.runTime; // Initially, the remaining time is the total runtime
            newProcess.startTime = -1; // Not yet started
            newProcess.waitingTime = 0; // Initialize waiting time
            
            MemoryBlock *allocatedBlock = addProcess(memoryRoot, &newProcess);
            if (allocatedBlock == NULL) {
                push(&waitingQueue, newProcess); // Add to waiting queue if memory not available
            } else {
                pushPriorityQueueHPF(&readyQueue, newProcess);
                totalExecutionTime += newProcess.runTime;

                // Preemption: Check if the new process has higher priority than the running process
                if (runningProcess.id != -1 && newProcess.priority < runningProcess.priority) {
                    // Update the remaining time for the running process
                    int elapsed = clk - runningProcess.lastTimeToRun;
                    runningProcess.remainingTime -= elapsed;

                    // Pause the running process and push it back to the queue
                    kill(runningProcess.pid, SIGSTOP);
                    pushPriorityQueueHPF(&readyQueue, runningProcess);

                    // Reset running process to allow the higher-priority one to take over
                    runningProcess.id = -1;
                }
            }
        }

        // Start a new process if no process is currently running
        if (runningProcess.id == -1 && !isEmptyPriorityQueue(&readyQueue)) {
            runningProcess = popPriorityQueue(&readyQueue);
            prevClk = getClk();

            int pid = fork();
            if (pid == 0) { // Child process
                char runtime[10];
                char id[10];
                sprintf(runtime, "%d", runningProcess.remainingTime);
                sprintf(id, "%d", runningProcess.id);
                execl("./process.out", "./process.out", runtime, id, NULL);
                perror("execl failed");
                exit(EXIT_FAILURE);
            } else if (pid > 0) { // Parent process
                runningProcess.pid = pid;
                runningProcess.startTime = (runningProcess.startTime == -1) ? clk : runningProcess.startTime;
                runningProcess.lastTimeToRun = clk; // Record when the process starts or resumes
                runningProcess.waitingTime += (clk - runningProcess.arrivalTime - (runningProcess.runTime - runningProcess.remainingTime));
                totalWait += runningProcess.waitingTime;
                fprintf(fptr, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                        runningProcess.lastTimeToRun, runningProcess.id,
                        runningProcess.arrivalTime, runningProcess.runTime,
                        runningProcess.remainingTime, runningProcess.waitingTime);
            } else {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
        }

        // Check if the running process is complete
        if (runningProcess.id != -1) {
            int elapsed = clk - runningProcess.lastTimeToRun;
            //totalExecutionTime += elapsed;
            // Only allow completion after at least one tick
            if (elapsed > 0) {
                runningProcess.remainingTime -= elapsed;
                runningProcess.lastTimeToRun = clk;

                if (runningProcess.remainingTime <= 0 && clk > runningProcess.startTime) { // Process is complete
                    int finishTime = clk;
                    //runningProcess.finishTime = finishTime;
                    int turnaroundTime = finishTime - runningProcess.arrivalTime;
                    float WTA_val = (float)turnaroundTime / runningProcess.runTime;
                    totalWTA += WTA_val;
                    WTA[runningProcess.id] = WTA_val;

                    fprintf(fptr, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                            finishTime, runningProcess.id, runningProcess.arrivalTime,
                            runningProcess.runTime, 0,
                            runningProcess.waitingTime, turnaroundTime, WTA_val);

                    remainingProcesses--;

                    // Free memory
                    updateMemory(memoryRoot, &runningProcess);

                    int allocations = 0;
                    // Try to allocate memory for processes in the waiting queue
                    while (!isEmpty(&waitingQueue)) {
                        struct Process *process = frontQueue(&waitingQueue); 
                        MemoryBlock *allocatedBlock = addProcess(memoryRoot, process);
                        if (allocatedBlock != NULL) {
                            pushPriorityQueueHPF(&readyQueue, *process);
                            pop(&waitingQueue); // Dequeue only if allocation succeeds
                        } else {
                            break;
                        }
                    }

                    runningProcess.id = -1; // Reset the running process
                }
            }
        }
    }

    // Calculate CPU utilization
    int finishTime = getClk();
    float totalUtil = ((float)totalExecutionTime / (finishTime-1)) * 100;

    // Print performance metrics
    PrintPerf(fout, totalUtil, totalWTA, totalWait, ProcessNum, WTA);
}

void SJF(FILE *fptr, FILE *fout){
    PriorityQueue readyQueue;
        initializePriorityQueue(&readyQueue);
        int totalWait = 0;
        float totalWTA = 0;
        int ProcessNum = remainingProcesses; // Total number of processes
        float WTA[ProcessNum]; // Array to store WTA for each process
        int totalIdle = 0;
        int prevClk = getClk(); // Previous clock for idle calculation

        Queue waitingList;
        initializeQueue(&waitingList);

        while (remainingProcesses > 0) {
            int clk = getClk();

            // Handle new arrivals
            while (1) {
                struct Process newProcess = recieveProcess();
                if (newProcess.id == -1)
                    break;
                // Check if memory is available
                if (addProcess(rootMemoryBlock, &newProcess)) {
                    pushPriorityQueue(&readyQueue, newProcess); // Push to ready queue only if memory is allocated
                    printf("At time %d process %d allocated memory and added to ready queue.\n", clk, newProcess.id);
                    //logMemoryEvent(clk, &newProcess, rootMemoryBlock, "allocate");

                } else {
                    push(&waitingList, newProcess); // Push to waiting list if memory is not available
                    printf("At time %d process %d not enough memory. Added to waiting list.\n", clk, newProcess.id);
                }
            }

            // Handle idle time (if there is no process running and empty ready queue)
            if (runningProcess.id == -1 && isEmptyPriorityQueue(&readyQueue)) {
                totalIdle += (clk - prevClk);
                prevClk = clk;
                continue;
            }

            // Start a new process if no process is running
            if (runningProcess.id == -1 && !isEmptyPriorityQueue(&readyQueue)) {
                runningProcess = popPriorityQueue(&readyQueue);
                prevClk = clk;


                int pid = fork();
                if (pid == 0) { // Child process
                    char runtime[10];
                    char id[10];
                    sprintf(runtime, "%d", runningProcess.runTime);
                    sprintf(id, "%d", runningProcess.id);
                    execl("./process.out", "./process.out", runtime, id, NULL);
                    perror("execl failed");
                    exit(EXIT_FAILURE);
                } else if (pid > 0) { // Parent process
                    runningProcess.pid = pid;
                    runningProcess.startTime = getClk();
                    totalWait += (runningProcess.startTime - runningProcess.arrivalTime);
                    fprintf(fptr, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                            runningProcess.startTime, runningProcess.id,
                            runningProcess.arrivalTime, runningProcess.runTime,
                            runningProcess.remainingTime,
                            runningProcess.startTime - runningProcess.arrivalTime);
                } else {
                    perror("fork failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Run the current process
            if (runningProcess.id != -1) {
                int currentClk = getClk();
                if (currentClk != prevClk) { // Process executes every clock tick
                    runProcess(); // Decrements the remaining time
                    prevClk = currentClk;
                }

                // Check if the current process has finished
                if (runningProcess.remainingTime <= 0) {
                    kill(runningProcess.pid, SIGINT); // Send termination signal
                    int status;
                    waitpid(runningProcess.pid, &status, 0); // Wait for child to terminate

                    // Deallocate memory once process finishes
                    if(updateMemory(rootMemoryBlock, &runningProcess)){
                        // Free memory block used by the process
                        //logMemoryEvent(currentClk, &runningProcess, rootMemoryBlock, "free");
                    }

                    int finishTime = getClk();
                    //unningProcess.finishTime = finishTime;
                    int turnaroundTime = finishTime - runningProcess.arrivalTime;
                    float WTA_val = (float)turnaroundTime / runningProcess.runTime;
                    totalWTA += WTA_val;
                    WTA[runningProcess.id - 1] = WTA_val; // Store WTA for process ID (adjusted for array indexing)

                    fprintf(fptr, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                            finishTime, runningProcess.id, runningProcess.arrivalTime,
                            runningProcess.runTime, 0,
                            runningProcess.startTime - runningProcess.arrivalTime,
                            turnaroundTime, WTA_val);

                    remainingProcesses--;
                    runningProcess.id = -1; // Reset to indicate no process running

                    // Check the waiting list for processes that can now be allocated
                    while (!isEmpty(&waitingList)) {
                        struct Process *frontProcess = frontQueue(&waitingList); // Peek at the front process

                        if (addProcess(rootMemoryBlock, frontProcess)) {
                            // Successfully allocated memory, move to the ready queue
                            pushPriorityQueue(&readyQueue, *frontProcess); // Push a copy to the ready queue
                            printf("At time %d process %d allocated memory and moved to ready queue.\n", getClk(), frontProcess->id);
                            pop(&waitingList); // Remove the process from the waiting list
                        } else {
                            // Not enough memory; stop checking further
                            break;
                        }
                    }

                }
            }
        }

        // Calculate CPU utilization
        int finishTime = getClk();
        int totalExecutionTime = finishTime - totalIdle; // Time CPU was executing processes
        float totalUtil = ((float)totalExecutionTime / finishTime) * 100;

        PrintPerf(fout, totalUtil, totalWTA, totalWait, ProcessNum, WTA);

        fclose(fptr);
        fclose(fout);
        //fclose(fmem);
        printf("Scheduler Terminated\n");
}

void MultiLevel(FILE *fptr, FILE *fout, int quantum)
{
    Queue* outerLoopArray [12];
        for (size_t i = 0; i <12 ; i++)
        {
            outerLoopArray[i] = (Queue*)malloc(sizeof(Queue));
            if (outerLoopArray[i] == NULL) {
                fprintf(stderr, "Memory allocation failed for outerLoopArray[%zu]\n", i);
                return;
            }
            initializeQueue(outerLoopArray[i]);
        }
        Node* tempNode= (Node*)malloc(sizeof(Node));
        struct Process* p = (struct Process*)malloc(sizeof(struct Process));
        struct msgbuff msg;
        msg.mtype = 1;
        int totalRunTime=0;
        int totalWaitingTime=0;
        float totalWTA =0;
        int FinishedProcesses =0;
        int pid;
        int rec_val;
        
        Queue waitingList; //for memory
        initializeQueue(&waitingList);
        
        while (FinishedProcesses < ProcessNum)
        {
            for (size_t i = 0; i < 12; i++)
            {
                do
                {
                    //try to recieve a new process from the generator
                    *p =recieveProcess(msgq_id_generator);

                    if(p->id != -1 ) //we recived a new process
                    {
                        printf("rec process %d\n", p->id);
                        //insert the process in its prio level
                        push(outerLoopArray[p->priority],*p);

                        totalRunTime += p->runTime;

                        //when reciving a new process which has higher priority return back to its level
                        if (i > p->priority)
                        {
                            i = p->priority;
                        }
                    }
                }while (p->id != -1);

                while (outerLoopArray[i]->front != NULL)
                {
                    if(i <11)
                    {
                         do
                        {
                            *p =recieveProcess(msgq_id_generator);
                            if(p->id != -1 ) //we recived a new process
                            {
                                printf("rec process %d\n", p->id);
                                //insert the process in its prio level
                                push(outerLoopArray[p->priority],*p);

                                totalRunTime += p->runTime;

                                //when reciving a new process which has higher priority return back to its level
                                if (i > p->priority)
                                {
                                    i = p->priority;
                                }
                            }
                        }while (p->id != -1);
                       
                        struct Process* processPtr = (struct Process*)malloc(sizeof(struct Process));
                        *processPtr = popq(outerLoopArray[i])->data;
                     
                        //Runnig the process
                        int clkStart =getClk();
                        if(processPtr->startTime <= -1) //first time to run
                        {
                            //adding to memory
                            // -2 to mark the process added before when another process finished
                            if(processPtr->startTime != -2 && !addProcess(rootMemoryBlock, processPtr) )
                            {  
                                push(&waitingList, *processPtr); // Push to waiting list if memory is not available
                                printf("At time %d process %d not enough memory. Added to waiting list.\n", clkStart, processPtr->id);
                            }
                            else
                            {
                                processPtr->startTime = clkStart;
                                //fork the process
                                pid = fork();
                                if (pid < 0) { perror("fork failed"); exit(1);}

                                else if (pid == 0) {  // Child process
                                    char runtime[10];
                                    char id[10];
                                    sprintf(runtime, "%d", processPtr->runTime);
                                    sprintf(id, "%d", processPtr->id);
                                    // Replace the child process with a new program
                                    if (execl("./process.out", "./process.out",runtime,id,"4", NULL) == -1)
                                    {  // If execl returns, an error occurred
                                        perror("execl failed");
                                        exit(1);
                                    }
                                }
                                processPtr->pid = pid;
                                processPtr->waitingTime += clkStart - processPtr->arrivalTime ;

                                fprintf(fptr, "At time %d Process %d started arr %d total %d remain %d, wait %d\n"
                                    ,clkStart,processPtr->id, processPtr->arrivalTime,
                                    processPtr->runTime, processPtr->runTime, processPtr->waitingTime );
                            }
                        }
                        else //it is not the first time to run this process
                        {
                            kill(processPtr->pid, SIGCONT);

                            processPtr->waitingTime += clkStart - processPtr->lastTimeToRun;

                            fprintf(fptr,"At time %d Process %d resumed arr %d total %d remain %d wait %d\n"
                                ,clkStart,processPtr->id, processPtr->arrivalTime,
                                processPtr->runTime, processPtr->remainingTime, processPtr->waitingTime );
                        }

                        if (processPtr->startTime > -1)
                        {
                            if (processPtr->remainingTime < quantum)
                            {
                                //run the remaining time to finish
                                int targetTime =clkStart + processPtr->remainingTime;
                                while (getClk() < targetTime );
                                processPtr->remainingTime = 0;
                            }
                            else
                            {
                                //run for one quantum
                                int targetTime =clkStart + quantum;
                                while (getClk()- clkStart < quantum);
                                processPtr->remainingTime = processPtr->remainingTime - quantum;
                            }

                            //end processing
                            int clkEnd = getClk();
                            //check process statues
                            if (processPtr->remainingTime <= 0) //process finished
                            {
                                FinishedProcesses ++;

                                //processPtr->finishTime = clkEnd;

                                int TA = clkEnd - processPtr->arrivalTime ;

                                float WTA = (float)TA/processPtr->runTime;
                                totalWTA += WTA;

                                totalWaitingTime += (TA - processPtr->runTime);

                                fprintf(fptr,"At time %d Process %d finished arr %d total %d remain %d wait %d TA %d WTA %f\n"
                                    ,clkEnd ,processPtr->id, processPtr->arrivalTime,
                                    processPtr->runTime, processPtr->remainingTime,
                                    ( TA - processPtr->runTime ), TA,
                                    WTA);

                                //removing from the memory
                                updateMemory(rootMemoryBlock, processPtr);
                                // Check the waiting list for processes that can now be allocated
                                while (!isEmpty(&waitingList)) {
                                    struct Process *frontProcess = frontQueue(&waitingList); // Peek at the front process
                                    if (addProcess(rootMemoryBlock, frontProcess)) {
                                        // Successfully allocated memory, move to the ready queue
                                        int priority = frontProcess->priority;
                                        frontProcess->startTime = -2;
                                        push(outerLoopArray[priority], *frontProcess);
                                        printf("At time %d process %d allocated memory and moved to ready queue.\n", getClk(), frontProcess->id);
                                        pop(&waitingList); // Remove the process from the waiting list
                                    } else {
                                        // Not enough memory; stop checking further
                                        break;
                                    }
                                }
                                free(processPtr); //delete process info
                            }
                            else //not finish so push it to lower priority
                            {
                                kill(processPtr->pid, SIGSTOP);

                                processPtr->lastTimeToRun = clkEnd; //helping calc waiting time

                                push(outerLoopArray[i+1], *processPtr);

                                fprintf(fptr, "At time %d Process %d stopped arr %d total %d remain %d wait %d\n"
                                    ,clkEnd ,processPtr->id, processPtr->arrivalTime,
                                    processPtr->runTime, processPtr->remainingTime,
                                    processPtr->waitingTime);
                            }
                        }   
                    }
                    else //the loop ended now return the processes back to its orginal priority level
                    {
                        Node* current = outerLoopArray[i]->front;
                        while (current != NULL)
                        {
                            int priority = current->data.priority;
                            popq(outerLoopArray[i]);
                            push(outerLoopArray[priority], current->data);
                            current = current->next;
                        }
                    }
                }
            }
        }
    int endSimuClk = getClk();
    float CPU_Util = ((float)totalRunTime / (endSimuClk-1)) * 100;
    PrintPer(fout, CPU_Util, totalWTA, totalWaitingTime, ProcessNum );

    free(tempNode);
    free(p);
    for (size_t i = 0; i <12 ; i++)
    {
        free(outerLoopArray[i]);
    }
}

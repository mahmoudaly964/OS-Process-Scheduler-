#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

struct Process{
    int arrivalTime;
    int runTime;
    int priority;
    //int finishTime;
    int remainingTime;
    int pid;
    int id;
    int startTime; //time when the process starts running

    int waitingTime;
    int lastTimeToRun;
    int memorySize;
};

// Initialize a new process node
struct Process *Process(int id,int arrivalTime, int runTime, int priority, int memorySize) {
    struct Process* newNode = malloc(sizeof(struct Process));
    if (!newNode) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    newNode->arrivalTime = arrivalTime;
    newNode->runTime = runTime;
    newNode->priority = priority;
    newNode->memorySize=memorySize;
    //newNode->finishTime = -1;
    newNode->remainingTime = runTime;
    newNode->pid = -1;
    newNode->id = id;
    newNode->startTime = -1;
    newNode->waitingTime = 0;
    newNode->lastTimeToRun = -1;

    return newNode;
}

void freeProcess(struct Process *p) {
    free(p);  // Free Memory
}


struct msgbuff
{
    long mtype;
    struct Process process;
};

typedef struct Node {
    struct Process data;
    struct Node *next;
} Node;


typedef struct Queue {
    Node* front;
    Node* rear;
    int size;
} Queue;


typedef struct MemoryBlock {
    int start;
    int end;
    int size;
    int processId; 
    bool isEmpty;
    struct MemoryBlock *left;
    struct MemoryBlock *right;
    struct MemoryBlock *parent;
} MemoryBlock;

// Initialize the queue
void initializeQueue(Queue* q) {
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}


Node* createNode(struct Process process) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        return NULL;
    }
    newNode->data = process;
    newNode->next = NULL;
    return newNode;
}


// Push (enqueue) a process node into the queue

void push(struct Queue* q, struct Process process) {
     
    Node* newNode = createNode(process);
    if (!newNode) {
        printf("allocation failure");
        return; // Handle memory allocation failure gracefully
    }

    if (!q->rear) { // If the queue is empty
        q->front = q->rear = newNode;
    } else { // If the queue is not empty
        q->rear->next = newNode;
        q->rear = newNode;
    }

    q->size++;
}
bool isEmpty(Queue*q)
{
    return(q->size==0);
}
// Pop (dequeue) a value from the queue
struct Process pop(Queue* q) {
    if (q->size == 0) {
        perror("Priority Queue is empty");
        exit(EXIT_FAILURE);
    }

    Node* temp = q->front;
    q->front = q->front->next;

    if (!q->front) {
        q->rear = NULL;
    }
    struct Process dequedProcess=temp->data;
    free(temp);
    q->size--;
    return dequedProcess;
}

// Get the length of the queue
int len(Queue* q) {
    return q->size;
}

// Peek at the front of the queue without removing it
struct Process* frontQueue(Queue* q) {
    if (q->size == 0) {
        perror("Queue is empty");
        exit(EXIT_FAILURE); 
    }

    return &q->front->data; // Return a pointer to the process at the front
}



/////// priority queue

typedef struct PriorityQueue {
    Node* front; // The front of the queue, holding the highest priority process
    int size;
} PriorityQueue;



// Initialize the priority queue
void initializePriorityQueue(PriorityQueue* pq) {
    pq->front = NULL;
    pq->size = 0;
}

// Function to create a new node


void pushPriorityQueue(PriorityQueue* pq, struct Process process) {
    Node* newNode = createNode(process);
    if (!newNode) return;

    // If the queue is empty or the new process has the shortest remaining time
    if (!pq->front || process.remainingTime < pq->front->data.remainingTime) {
        newNode->next = pq->front;
        pq->front = newNode;
    } else {
        // Traverse to find the correct position
        Node* current = pq->front;
        while (current->next && current->next->data.remainingTime <= process.remainingTime) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    pq->size++;
}

void pushPriorityQueueHPF(PriorityQueue* pq, struct Process process) {
    Node* newNode = createNode(process);
    if (!newNode) return;

    // If the queue is empty or the new process has the shortest remaining time
    if (!pq->front || process.priority < pq->front->data.priority) {
        newNode->next = pq->front;
        pq->front = newNode;
    } else {
        // Traverse to find the correct position
        Node* current = pq->front;
        while (current->next && current->next->data.priority <= process.priority) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    pq->size++;
}

struct Process popPriorityQueue(PriorityQueue* pq) {
    if (pq->size == 0) {
        perror("Priority Queue underflow");
        exit(EXIT_FAILURE);
    }

    Node* temp = pq->front;
    pq->front = pq->front->next;
    struct Process dequeuedProcess = temp->data;
    free(temp);
    pq->size--;

    return dequeuedProcess;
}

// Peek at the process with the highest priority without removing it
struct Process peekPriorityQueue(PriorityQueue* pq) {
    if (pq->size == 0) {
        perror("Priority Queue is empty");
        exit(EXIT_FAILURE);
    }

    return pq->front->data;
}

// Check if the priority queue is empty
int isEmptyPriorityQueue(PriorityQueue* pq) {
    return pq->size == 0;
}

// Get the size of the priority queue
int sizePriorityQueue(PriorityQueue* pq) {
    return pq->size;
}

// Pop (dequeue) a value from the queue
Node* popq(Queue* q) {
    if (q->size == 0) {
        perror("Priority Queue is emptymmmmmmmmmm\n");
        exit(EXIT_FAILURE);
    }

    Node* temp = q->front;
    q->front = q->front->next;

    if (!q->front) {
        q->rear = NULL;
    }
    // struct Process dequedProcess=temp->data;
    // free(temp);
    q->size--;
    return temp;
    // return dequedProcess;
}


///memory

MemoryBlock *initializeMemoryBlock(int start, int size) {
    MemoryBlock *block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    block->start = start;
    block->end = start + size - 1;
    block->size = size;
    block->processId = -1;
    block->isEmpty = true;
    block->left = NULL;
    block->right = NULL;
    block->parent = NULL;
    return block;
}


MemoryBlock *findBestFitBlock(MemoryBlock *node, int size) {
   if(node->left && node->right)
   {
        struct MemoryBlock *leftB=findBestFitBlock(node->left,size);
        struct MemoryBlock *rightB=findBestFitBlock(node->right,size);
        // if no block found return null
        if(!leftB && !rightB)
        {
            return NULL;
        }
        if(leftB==NULL)// if no block suitaable in left 
        {
            return rightB;
        }
        if(rightB==NULL)// if no block suitaable in right 
        {
            return leftB;
        }
        // if there is block suitable in both
        if (leftB->size <= rightB->size)
        {
            return leftB;
        }
        else
        {
            return rightB;
        }

   }
   else // if it is a leaf node 
   {
        if (node->size >= size && node->processId == -1)
        {
            return node;
        }
        else
        {
            return NULL;
        }
   }
}

// Function to add a process to memory
MemoryBlock *addProcess(MemoryBlock* root, struct Process* process) {
   
    if (root == NULL)
    {
        return NULL;
    }

    int size = process->memorySize;
    int processId = process->id;
    
    struct MemoryBlock *temp = findBestFitBlock(root, size);
    if (temp == NULL)
    {
        return NULL;
    }
    while (temp->size/2>=size)
    {
        temp->left = initializeMemoryBlock(temp->start, temp->size / 2);
        temp->left->parent = temp;
        temp->right = initializeMemoryBlock(temp->start + temp->size / 2, temp->size / 2);
        temp->right->parent = temp;
        temp = temp->left;
    }
    temp->processId = processId;
    temp->isEmpty = false;
    struct MemoryBlock *parent = temp->parent;
    while (parent != NULL)
    {
        parent->isEmpty = false;
        parent = parent->parent;
    }
    int currentTime = getClk();
        FILE *logFile = fopen("memory.log", "a");
        if (!logFile) {
            perror("Error opening memory.log");
            exit(EXIT_FAILURE);
        }
        fprintf(logFile, "At time %d allocated %d bytes from process %d from %d to %d\n",
                currentTime, process->memorySize, process->id, temp->start, temp->end);
        fclose(logFile);
    return temp;
}

// Function to update memory and free the block used by a process
bool updateMemory(MemoryBlock* memBlock, struct Process* process) {
    if (memBlock == NULL) {
        return false;
    }

    if (memBlock->processId == process->id) {
        // Free the memory block
        memBlock->processId = -1;
        memBlock->isEmpty = true;

        // Log the deallocation
        int currentTime = getClk();
        FILE *logFile = fopen("memory.log", "a");
        if (!logFile) {
            perror("Error opening memory.log");
            exit(EXIT_FAILURE);
        }
        fprintf(logFile, "At time %d freed %d bytes from process %d from %d to %d\n",
                currentTime, process->memorySize, process->id, memBlock->start, memBlock->end);
        fclose(logFile);
        return true;
    }

    // Recursively search left and right children
    bool freeLeft = updateMemory(memBlock->left, process);
    bool freeRight = updateMemory(memBlock->right, process);

    // Merge blocks if both children are free
    if (memBlock->left && memBlock->right) {
        if (memBlock->left->isEmpty && memBlock->right->isEmpty) {
            printf("Merging memory blocks [%d-%d] and [%d-%d]\n", 
                   memBlock->left->start, memBlock->left->end, 
                   memBlock->right->start, memBlock->right->end);

            memBlock->isEmpty = true;
            free(memBlock->left);
            free(memBlock->right);
            memBlock->left = NULL;
            memBlock->right = NULL;
        }
    }

    return freeLeft || freeRight;
}


#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int id;

void decrementTime(int signum)
{
    remainingtime--;
    printf("Scheduler: process with id: %d is running and  has remaining time %d at TIME: %d \n", id, remainingtime ,getClk());
    if(remainingtime==0)
    {
        destroyClk(false);
        exit(0);
    }
}

void terminate(int signum)
{
    destroyClk(false);
    exit(0);
}



int main(int agrc, char * argv[])
{
    remainingtime = atoi(argv[1]);
    id=atoi(argv[2]);
    int algoNumber = -1;
    if (agrc > 3)
    {
        algoNumber = atoi(argv[3]);
    }
    
    printf("Scheduler: process with id: %d is ready with remaining time  %d \n", id , remainingtime);
    
    if (algoNumber != 4)
    {
        signal(SIGUSR1, decrementTime);
        signal(SIGUSR2, terminate);
    }
    initClk();
    //remaining time will be passed from the scheduler
   
    int statrtClk = getClk();
    //TODO it needs to get the remaining time from somewhere
    while (remainingtime > 0)
    {
        if (getClk() - statrtClk == 1 && algoNumber == 4)
        {
            remainingtime --;
            statrtClk = getClk();
        }
    }
    printf("Scheduler: process with id: %d has finished\n", getpid());
   
   if (algoNumber != 4)
   {
        
   }
   else
   {
        destroyClk(false);
        exit(0);
   }
    
    return 0;
}

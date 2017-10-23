// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling SelectNextReadyThread(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// ProcessScheduler::ProcessScheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

ProcessScheduler::ProcessScheduler()
{ 
    listOfReadyThreads = new List;
    type = 1; //added by me 
} 

//----------------------------------------------------------------------
// ProcessScheduler::~ProcessScheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

ProcessScheduler::~ProcessScheduler()
{ 
    delete listOfReadyThreads; 
} 

//----------------------------------------------------------------------
// ProcessScheduler::MoveThreadToReadyQueue
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
ProcessScheduler::MoveThreadToReadyQueue (NachOSThread *thread)
{
    DEBUG('t', "Putting thread %s with PID %d on ready list.\n", thread->getName(), thread->GetPID());

    int currenttime = stats->totalTicks;
    //if it is being executed for the first time...
    if(thread -> Entry_Time_Ready_Queue==0 && thread->GetPID()!=0)
    {
        thread ->Thread_Start_Time=currenttime;
       // printf("Call thread as pid , %d\n",thread->GetPID());    
    }

    //if it was sleeping then add duration of sleep.
    thread ->Entry_Time_Ready_Queue=currenttime; 
    if (thread->getStatus() == BLOCKED)
        thread->Total_Sleep = thread->Total_Sleep + currenttime - (thread->Start_Sleep);

    thread->setStatus(READY);
    listOfReadyThreads->Append((void *)thread);
}

//----------------------------------------------------------------------
// ProcessScheduler::SelectNextReadyThread
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	NachOSThread is removed from the ready list.
//----------------------------------------------------------------------

NachOSThread *
ProcessScheduler::SelectNextReadyThread ()
{
    
    if(type == 2)
    {
        int min_burst=1<<16 , entry_time=0;
        NachOSThread* selectedThread;
        List* ptr = listOfReadyThreads->getFirst();
        if(ptr==NULL) return NULL;   //empty-list
        while(ptr!=NULL)
        {
            if(ptr->item != NULL)
            {
                if(ptr->item->Estimated_CPU_Burst < min_burst)
                {
                    min_burst = ptr->item->Estimated_CPU_Burst;
                    entry_time = ptr->item->Entry_Time_Ready_Queue;
                    selectedThread = (NachOSThread *)ptr->item;
                }
                if(ptr->item->Estimated_CPU_Burst == min_burst)
                {
                    if(entry_time > ptr->item->Entry_Time_Ready_Queue)
                    {   
                        entry_time = ptr->item->Entry_Time_Ready_Queue;
                        selectedThread = (NachOSThread *)ptr->item;
                    }
                }   
            }
            else
                printf("ERROR: element present in ready queue with null thread attached." );
        }

        if(selectedThread != NULL) 
        {
            ptr = listOfReadyThreads->getFirst();
            if(ptr==NULL) return NULL;
            while(ptr!=NULL)
            {
                ptr = (NachOSThread *)listOfReadyThreads->Remove();
                if(ptr->item != NULL)
                {
                    if(selectedThread == (NachOSThread *)ptr->item)
                        return (NachOSThread *)ptr->item;
                    else
                        listOfReadyThreads->Append(void *ptr);
                }
                else
                    printf("ERROR: element present in ready queue with null thread attached." );
            }       
        }
    }
    // UNIX ---------------------------------------------
    else if(type >=7 && type <= 10)
    {
        NachOSThread* selectedThread;
        List* ptr = listOfReadyThreads->getFirst();
        if(ptr==NULL) return NULL;   //empty-list
        while(ptr!=NULL)
        {
            if(ptr->item != NULL)
            {
                if(ptr->item->Estimated_CPU_Burst < min_burst)
                {
                    min_burst = ptr->item->Estimated_CPU_Burst;
                    entry_time = ptr->item->Entry_Time_Ready_Queue;
                    selectedThread = (NachOSThread *)ptr->item;
                }
                if(ptr->item->Estimated_CPU_Burst == min_burst)
                {
                    if(entry_time > ptr->item->Entry_Time_Ready_Queue)
                    {   
                        entry_time = ptr->item->Entry_Time_Ready_Queue;
                        selectedThread = (NachOSThread *)ptr->item;
                    }
                }   
            }
            else
                printf("ERROR: element present in ready queue with null thread attached." );
        }   
    }

    else
        return (NachOSThread *)listOfReadyThreads->Remove();
}

//----------------------------------------------------------------------
// ProcessScheduler::ScheduleThread
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
ProcessScheduler::ScheduleThread (NachOSThread *nextThread)
{
    NachOSThread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveContextOnSwitch();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" with pid %d to thread \"%s\" with pid %d\n",
	  oldThread->getName(), oldThread->GetPID(), nextThread->getName(), nextThread->GetPID());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    _SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\" with pid %d\n", currentThread->getName(), currentThread->GetPID());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreContextOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// ProcessScheduler::Tail
//      This is the portion of ProcessScheduler::ScheduleThread after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
ProcessScheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreContextOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// ProcessScheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
ProcessScheduler::Print()
{
    printf("Ready list contents:\n");
    listOfReadyThreads->Mapcar((VoidFunctionPtr) ThreadPrint);
}

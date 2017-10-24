// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"


#define BUFFER_SIZE   1024
//----------------------------------------------------------------------
// LaunchUserProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
LaunchUserProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    ProcessAddressSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new ProcessAddressSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitUserModeCPURegisters();		// set the initial register values
    space->RestoreContextOnSwitch();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

//----------------------------------------------------------------------
// LaunchBatchProcess
//  Run user programs.  Open the executables, load it into
//  memory, and jump to it.
//----------------------------------------------------------------------


void
LaunchBatchProcess(char *filename)
{
    OpenFile *executablelist = fileSystem->Open(filename);
    OpenFile* executable;
    ProcessAddressSpace *space;
    NachOSThread* childThread
    if (executablelist == NULL) {
    printf("Unable to open file %s\n", filename);
    return;
    }
    
    int filelength = executablelist->Length();
    char filebuffer[BUFFER_SIZE];
    executablelist->ReadAt(filebuffer, filelength-1, 0); //filelength - 1 ??

    char name[128], priority[10];
    int priority_int;
    int i=0, j=0; 
    int sched_type;

    //for main thread.
    priorityValue[currentThread->GetPID()] = 0;
    basePriorityValue[currentThread->GetPID()] = 50;

    while(filebuffer[i] != '\n')
    {
        name[j] = filebuffer[i];
        ++i, ++j;
    }
    name[j] = '\0';
    sched_type = atoi(name);
    scheduler->type = sched_type;

    while( i < filelength )
    {
        while(filebuffer[i] =='\n' || filebuffer[i] == ' ') ++i;

        j=0;
        while(filebuffer[i] != ' ' || filebuffer[i] != '\n' )
        {
            name[j++] = filebuffer[i++];
        }
        name[j] = '\0';
        while(filebuffer[i]==' ') ++i;

        if(filebuffer[i] == '\n')
        {
            priority_int = 100;
        }
        else
        {
            //read priority
            j=0;
            while(filebuffer[i]!='\n' || filebuffer!=' ')
            {
                priority[j++] = filebuffer[i++];
            }
            priority[j] = '\0';
            priority_int = atoi(priority);

            while(filebuffer[i]!='\n') ++i;
        }

        executable = fileSystem->Open(name);

        if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
        }
        childThread = new NachOSThread(name);
        childThread->space = new ProcessAddressSpace(executable);

        basePriorityValue[childThread->GetPID()] = 50;
        priorityValue[childThread->GetPID()] = priority_int + basePriorityValue[childThread->GetPID()];
        

        delete executable;          // close file

        childThread->space->InitUserModeCPURegisters();      // set the initial register values
        childThread->space->RestoreContextOnSwitch();        // load page table register

        childThread->ThreadFork(*ForkStartFunction, 0);
        
    }

    delete executablelist;

    syscall_wrapper_Exit(0);
}
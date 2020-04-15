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

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------


void
SecondThread(int dummy)
{
	printf("Second thread starts.\n");
	machine->Run();
}


void
StartProcess(char *filename)
{
#ifdef MULTI_PROCESS
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;
    OpenFile *executable1 = fileSystem->Open(filename);
    AddrSpace *space1;
    Thread *thread = new Thread("1");
    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
	printf("Allocate first thread address space.\n");
    	space = new AddrSpace(executable); 
    	printf("Allocate second thread address space.\n");
    	space1 = new AddrSpace(executable);
	currentThread->space = space;
	space1->InitRegisters();		// set the initial register values
	space1->RestoreState();
    	thread->space = space1;
	thread->Fork(SecondThread, 0);
    	currentThread->Yield();
	delete executable;
	delete executable1;
    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register
    printf("First thread starts.\n");
    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"

#else
	OpenFile *executable = fileSystem->Open(filename);
    	AddrSpace *space;
 	if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
    	}
	space = new AddrSpace(executable);  
	delete executable;
	space->InitRegisters();		// set the initial register values
    	space->RestoreState();		// load page table register
    	printf("First thread starts.\n");
    	machine->Run();	
	ASSERT(FALSE);		
#endif

}



// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

//static Console *console;
static SynchConsole *console;
//static Semaphore *readAvail;
//static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

//static void ReadAvail(int arg) { readAvail->V(); }
//static void WriteDone(int arg) { writeDone->V(); }


//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    //console = new Console(in, out, ReadAvail, WriteDone, 0);
    console = new SynchConsole(in, out);
    //readAvail = new Semaphore("read avail", 0);
    //writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	//readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	//writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

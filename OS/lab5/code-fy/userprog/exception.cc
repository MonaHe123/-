// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------




void
tlbMiss()
{
	int vpn = (unsigned) machine->registers[BadVAddrReg] / PageSize;
	int k = -1; 	
	for (int i = 0; i < TLBSize; i++)
	{
		if (machine->tlb[i].valid == false)
		{
			k = i;
#ifdef tlb_LRU
			//maintaining LRU_count[]			
			machine->LRU_count[k] = 1;
			for (int j = 0; j < TLBSize; j++)
			{
				if (j == k)
					continue;
				if (machine->LRU_count[j] == -1)
					continue;
				machine->LRU_count[j]++;		
			}
#endif
			break;
		}
	}
	if (k == -1) //all entries are valid, substitute 
	{
#ifdef tlb_FIFO
		//substitute the last entry for the missing entry
		//move entries forward by 1		
		k = TLBSize - 1;
		for (int i = 0; i < TLBSize - 1; i++)
			machine->tlb[i] = machine->tlb[i+1];
#endif
#ifdef tlb_LRU
		//all entries are valid, and there must be one entry having 
		//the biggest LRU_count == TLBSize, find it 
		for (int i = 0; i < TLBSize; i++)
		{	
			if (machine->LRU_count[i] == TLBSize)
			{
				k = i;
				break;
			}
		}
		machine->LRU_count[k] = 1;
		//maintaining LRU_count[]		
		for (int i = 0; i < TLBSize; i++)
		{
			if (i == k)
				continue;
			machine->LRU_count[i]++;
		}
#endif		
	}
	machine->tlb[k].valid = true;
	machine->tlb[k].virtualPage = vpn;
	machine->tlb[k].physicalPage = machine->pageTable[vpn].physicalPage;
	machine->tlb[k].use = false;
	machine->tlb[k].dirty = false;
	machine->tlb[k].readOnly = false;
}


void
pageMiss()
{
 	printf("Page miss\n");	
	OpenFile* file = fileSystem->Open("virtual_memory");
	if (file == NULL)
		ASSERT(false);
	int vpn = (unsigned) machine->registers[BadVAddrReg] / PageSize;
	int k = machine->AllocateNewProgram();

#ifdef USE_RPT
	if (k == -1)
	{
		k = 0; //substitute the first physical page
		if(machine->rPageTable[k].dirty == true)	
		{
			file->WriteAt(&(machine->mainMemory[k*PageSize]), 
				PageSize, machine->rPageTable[k].virtualPage*PageSize);
			machine->rPageTable[k].valid = false;
		}
	}			
	file->ReadAt(&(machine->mainMemory[k*PageSize]), PageSize, vpn*PageSize);	
	machine->rPageTable[k].valid = true;
	machine->rPageTable[k].virtualPage = vpn;	
	machine->rPageTable[k].use = false;
	machine->rPageTable[k].dirty = false;
	machine->rPageTable[k].readOnly = false;
	machine->rPageTable[k].threadID = currentThread->GetThreadID();
#else
	if (k == -1)
	{
		k = 0;
		for (int i = 0; i < machine->pageTableSize; i++)
		{
			if (machine->pageTable[i].physicalPage == 0) 	//substitute the first physical page
				if(machine->pageTable[i].dirty == true)	//for the missing page
				{
					file->WriteAt(&(machine->mainMemory[k*PageSize]), 
						PageSize, machine->pageTable[i].virtualPage*PageSize);
					machine->pageTable[i].valid = false;
					break;
				}		
	
		}
	}
	file->ReadAt(&(machine->mainMemory[k*PageSize]), PageSize, vpn*PageSize);	
	machine->pageTable[vpn].valid = true;
	machine->pageTable[vpn].physicalPage = k;
	machine->pageTable[vpn].use = false;
	machine->pageTable[vpn].dirty = false;
	machine->pageTable[vpn].readOnly = false;
#endif
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else {
	if (which == PageFaultException)
	{
		if (machine->tlb != NULL)
			tlbMiss();
		else
			pageMiss();
	}
	else
	{
		if ((which == SyscallException) && (type == SC_Exit))  //user program call Exit()
		{
			printf("User program exit.\n");
  			machine->ClearCurrentProgram();
			int NextPC = machine->ReadRegister(NextPCReg);
			machine->WriteRegister(PCReg, NextPC);
			currentThread->Finish();		
		}
		else		
			printf("Unexpected user mode exception %d %d\n", which, type);
	}
    }
}



		





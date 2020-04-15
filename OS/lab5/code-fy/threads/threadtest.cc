// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** ThreadName: %s threadID: %d UserID: %d looped %d times\n",currentThread->getName(), which, currentThread->GetUserID(), num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}


void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    Thread *t1 = new Thread("forked thread");
    Thread *t2 = new Thread("forked thread");
    Thread *t3 = new Thread("forked thread");


    t1->Fork(SimpleThread, t1->GetThreadID());
    t2->Fork(SimpleThread, t2->GetThreadID()); 
    t3->Fork(SimpleThread, t3->GetThreadID());
    SimpleThread(currentThread->GetThreadID());
}

void
ThreadNumberTest()
{
    DEBUG('t', "Entering ThreadNumberTest");
    for (int i = 1; i <=130; i++)
    {
	Thread* t = new Thread("forked thread");
	printf("*** ThreadName: %s threadID: %d\n", t-> getName(), t->GetThreadID());
    }
}


const char* const ThreadStatusChar[]={"JUST_CREATED", "RUNNING", "READY", "BLOCKED"};

void
ReadyListPrint(int arg) //print the info of the threads which are in the readylist
{
    Thread* t = (Thread*) arg;
    printf("*** ThreadName: %s; threadID: %d; ThreadStatus:%s; Priority: %d\n",t->getName(), t->GetThreadID(), ThreadStatusChar[t->getStatus()], t->GetPriority());
}

void
PrintThreadInfo(int useless)
{
    IntStatus oldLevel = interrupt-> SetLevel(IntOff);
    printf("*** ThreadName: %s; threadID: %d; ThreadStatus: %s; Priority: %d\n",currentThread->getName(), currentThread->GetThreadID(), ThreadStatusChar[currentThread->getStatus()], currentThread->GetPriority());
    List* ready = new List();
    ready = scheduler -> getReadyList();
    if (! ready->IsEmpty())
	ready -> Mapcar(ReadyListPrint);
    currentThread -> Yield();
    interrupt-> SetLevel(oldLevel);
}

void 
ThreadPrintTest()
{
    DEBUG('t',"Entering ThreadPrintTest");
    Thread *t1 = new Thread("forked thread");
    Thread *t2 = new Thread("forked thread");
    Thread *t3 = new Thread("forked thread");
    Thread *t4 = new Thread("forked thread");
    t1->Fork(PrintThreadInfo, 0);
    t2->Fork(PrintThreadInfo, 0);
    t3->Fork(PrintThreadInfo, 0);
    t4->Fork(PrintThreadInfo, 0);
}



void 
RunThread(int dummy)
{
    for (int i = 0; i < 100; i++)
    {
	printf("*** ThreadName: %s; times: %d\n",currentThread->getName(), i);
	interrupt->SetLevel(IntOff);
	interrupt->SetLevel(IntOn);
    }
}


void 
ThreadTimerTest()
{
    Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    t1->Fork(RunThread, 0);
    t2->Fork(RunThread, 0);
}




void
simplethread4(int which)
{
	for (int i=0; i<100; i++)
	{
		printf("I'm %s\n",currentThread->getName());
		interrupt->OneTick();
	}
}

void
thread_test_class()
{
	Thread *t = new Thread("hellokitty");
	t->Fork(simplethread4,1);
	Thread *t1 =new Thread("helloworld");
	t1->Fork(simplethread4,1);		
}


//producer-consumer problem

int buffer[10];

int number = 0; //the number of elements in the buffer

Lock* condition_lock = new Lock("condition_lock");
Condition* condition = new Condition("condition");

Semaphore* full = new Semaphore("full",0);
Semaphore* empty = new Semaphore("empty",10);
Semaphore* lock = new Semaphore("lock",1); 

#define semaphore

void 
producer(int dummy)
{
#ifdef condition	
	condition_lock -> Acquire();		
	for (int i = 0; i < 15;)
	{	
		if (number != 10) //the buffer is not full
		{	
			buffer[number] = i;
			printf("producer put item %d\n", i);
			number++;
			i++;
			condition -> Signal(condition_lock); 

		}
		else // the buffer is full
			condition -> Wait(condition_lock); 
	}
	condition_lock -> Release();
#endif
#ifdef semaphore
	for (int i = 0; i < 15; i++)	
	{
		empty->P(); //continue if buffer has empty space,else sleep
		lock->P(); //get the buffer lock
		buffer[number] = i;
		printf("producer put item %d\n", i);
		number++;
		full->V();
		lock->V();
	}
#endif
}



void
consumer(int dummy)
{
#ifdef condition	
	condition_lock->Acquire();
	for (int i = 0; i < 20;)
	{
		if (number != 0) //the buffer is not empty
		{
			number--;
			printf("consumer get item %d \n",buffer[number]);
			condition -> Signal(condition_lock); 
			i++; 
		}
		else 
			condition -> Wait(condition_lock);
	} 
	condition_lock -> Release();
#endif
#ifdef semaphore
	for (int i = 0; i < 15; i++)	
	{
		full->P(); //continue if buffer has element,else sleep
		lock->P(); //get the buffer lock
		number--;
		printf("consumer get item %d \n",buffer[number]);
		empty->V();
		lock->V();
	}
#endif
}

void
Condition_Test()
{
    Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    t1->Fork(producer, 0);
    t2->Fork(consumer, 0);
}

#undef semaphore 


Lock* mutex = new Lock("mutex");
int reader_count = 0;
Lock* reader_count_mutex = new Lock("reader_count_mutex");


int rw_buffer = 0; //initialize an int buffer to read or write


void
reader(int number) //reader thread number
{
	for (int i = 0; i < 5; i++)
	{	
		reader_count_mutex -> Acquire();
		reader_count++;
		if (reader_count == 1) //disable writer
			mutex -> Acquire();
		reader_count_mutex -> Release();
		printf("reader %d reads the buffer, the value is %d.\n",number, rw_buffer); //read the buffer
		reader_count_mutex -> Acquire();
		reader_count--;
		if (reader_count == 0) //enable writer
			mutex -> Release();
		reader_count_mutex -> Release();
	}
}
void 
writer(int number) //writer thread number这些都在reader_count锁中进行
{
	for (int i = 0; i < 5; i++)
	{	
		mutex -> Acquire();
		rw_buffer = i;
		printf("writer %d writes in the buffer, the value is %d.\n",number, i); //write the buffer
		mutex -> Release();
	}
}

void
rw_problem()
{
	Thread *t1 = new Thread("thread1");
	Thread *t2 = new Thread("thread2");
 	Thread *t3 = new Thread("thread1");
    	Thread *t4 = new Thread("thread2");
	t1->Fork(writer, 1);
    	t2->Fork(writer, 2);
	t3->Fork(reader, 3);
    	t4->Fork(reader, 4);
}






//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
        ThreadTest2();
	break;
    case 3:
	ThreadNumberTest();
	break;
    case 4:
	ThreadPrintTest();
	break;
    case 5:
	ThreadTimerTest();
	break;
    case 6:
	Condition_Test();
	break;
    case 7:
	rw_problem();
	break;
    case 8:
	thread_test_class();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}


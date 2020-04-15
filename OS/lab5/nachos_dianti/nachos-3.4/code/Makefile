# Copyright (c) 1992 The Regents of the University of California.
# All rights reserved.  See copyright.h for copyright notice and limitation 
# of liability and disclaimer of warranty provisions.

MAKE = make
LPR = lpr


#all是一个伪目标，当其放在第一的时候，默认其为目标
#进入文件之后，生成目标nachos
all: 
	cd threads; $(MAKE) depend
	cd threads; $(MAKE) nachos    
	cd userprog; $(MAKE) depend 
	cd userprog; $(MAKE) nachos 
	cd vm; $(MAKE) depend
	cd vm; $(MAKE) nachos 
	cd filesys; $(MAKE) depend
	cd filesys; $(MAKE) nachos 
	cd network; $(MAKE) depend
	cd network; $(MAKE) nachos 
	cd bin; make all
	cd test; make all

helloworld : hello.o
	cc -o helloworld hello.o

hello.o : hello.c
	cc -c hello.c

# don't delete executables in "test" in case there is no cross-compiler
clean:
	cd threads; rm *.o nachos
	cd network; rm *.o nachos
	cd filesys; rm *.o nachos
	cd test; rm *.o
	cd userprog; rm *.o nachos
	cd vm; rm *.o nachos
	rm -f *~ */{core,nachos,DISK,*.o,swtch.s,*~} test/{*.coff} bin/{coff2flat,coff2noff,disassemble,out}
	rm helloworld hello.o

print:
	/bin/csh -c "$(LPR) Makefile* */Makefile"
	/bin/csh -c "$(LPR) threads/*.h threads/*.cc threads/*.s"
	/bin/csh -c "$(LPR) userprog/*.h userprog/*.cc" 
	/bin/csh -c "$(LPR) filesys/*.h filesys/*.cc
	/bin/csh -c "$(LPR) network/*.h network/*.cc 
	/bin/csh -c "$(LPR) machine/*.h machine/*.cc
	/bin/csh -c "$(LPR) bin/noff.h bin/coff.h bin/coff2noff.c"
	/bin/csh -c "$(LPR) test/*.h test/*.c test/*.s"

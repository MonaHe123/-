// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include "time.h"
#include "string.h"
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
bool
FileHeader::Extend(BitMap *freeMap, int extendSize)
{
	int newNumBytes = numBytes + extendSize;
	int newNumSectors = divRoundUp(newNumBytes, SectorSize);
	if (newNumSectors == numSectors)
	{
		numBytes = newNumBytes;		
		return true;
	}
	else
	{
		int newSecondIndexSectors = divRoundUp(newNumSectors, 32);
		int newTotalSectors = newSecondIndexSectors + newNumSectors;
		int oldSecondIndexSectors = divRoundUp(numSectors, 32);	
		int oldTotalSectors = oldSecondIndexSectors + numSectors;
		if (freeMap->NumClear() < newTotalSectors - oldTotalSectors)
			return false;
		else
		{
			char* buffer = new char[SectorSize];
			synchDisk->ReadSector(dataSectors[oldSecondIndexSectors-1], buffer);
			int* sector = (int*) buffer;
			printf("Extend %d total sectors\n", newTotalSectors - oldTotalSectors);
			for (int i = oldSecondIndexSectors - 1; i < newSecondIndexSectors; i++)
			{
				if (i != oldSecondIndexSectors - 1)
					dataSectors[i] = freeMap->Find();
				int* new_sector = new int[32];
				for (int j = 0; (j < 32) && ((32 * i + j) < newNumSectors); j++)
					if (32 * i + j < numSectors)
						new_sector[j] = sector[j];
					else					
						new_sector[j] = freeMap->Find();
				synchDisk->WriteSector(dataSectors[i], (char*) new_sector);
				delete new_sector;
			}
			delete sector;	
			numBytes = newNumBytes;
			numSectors = newNumSectors;		
		}
	}	
}

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);

	int SecondIndexSectors = divRoundUp(numSectors, 32);	
	int totalSectors = numSectors + SecondIndexSectors;
	if (freeMap->NumClear() < totalSectors)
		return false;
	for (int i = 0; i < SecondIndexSectors; i++)
	{
		dataSectors[i] = freeMap->Find();
		int* sector = new int[32];
		for (int j = 0; (j < 32) && ((32 * i + j) < numSectors); j++)
			sector[j] = freeMap->Find();
		synchDisk->WriteSector(dataSectors[i], (char*) sector);
		delete sector;
	}

   //if (freeMap->NumClear() < numSectors)
	//return FALSE;		// not enough space

    //for (int i = 0; i < numSectors; i++)
	//dataSectors[i] = freeMap->Find();
    SetTime(0);
    //Print();
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
 	char *data = new char[SectorSize];
	int secondIndex = divRoundUp(numSectors, 32);
	for (int i = 0; i < secondIndex; i++) 
	{
		synchDisk->ReadSector(dataSectors[i], data);
		int* sector = (int*) data;
		for (int j = 0; (j < 32) && ((32 * i + j) < numSectors); j++)
		{
			ASSERT(freeMap->Test(sector[j]));  // ought to be marked!
			freeMap->Clear(sector[j]);
		}
		ASSERT(freeMap->Test(dataSectors[i]));  // ought to be marked!
		freeMap->Clear(dataSectors[i]);
	}
	//ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	//freeMap->Clear((int) dataSectors[i]);
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	int sector_num = offset / SectorSize;
	int secondIndex = sector_num / 32;
	
	char* buffer = new char[SectorSize];
	synchDisk->ReadSector(dataSectors[secondIndex], buffer);
	int* sector = (int*) buffer;
	int result = sector[sector_num - secondIndex * 32];
	delete sector;
	return result;
  //  return(dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
	//printf("filelength: %d\n", numBytes);
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.\n", numBytes);
	int secondIndex = divRoundUp(numSectors, 32);
	printf("Second index sectors:\n");
	for (i = 0; i < secondIndex; i++)
		printf("%d, ",dataSectors[i]);

    for (i = 0; i <secondIndex; i++)
    {
	printf("\nBlocks in second index sectors %d:\n", dataSectors[i]);
	synchDisk->ReadSector(dataSectors[i], data);
	int* sector = (int*) data;
	for (j = 0; (j < 32) && ((32 * i + j) < numSectors); j++)
		printf("%d, ",sector[j]);
    }
    printf("\nCreating time: %s", creatingTime);
    printf("\nLast reading time: %s", lastReadingTime);
    printf("\nLast writing time: %s", lastWritingTime);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	secondIndex = i / 32;
	synchDisk->ReadSector(dataSectors[secondIndex], data);
	int* sector = (int*) data;
	int temp = sector[i - secondIndex * 32];
	synchDisk->ReadSector(temp, data);
	printf("in sector %d:\n",temp);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

void
FileHeader::SetTime(int i)
{
	time_t time_sec;
	struct tm* localTime;
	time(&time_sec);
	localTime = localtime(&time_sec);
	char* timechar = asctime(localTime);
	switch(i)
	{
	case 0:
		strncpy(creatingTime, timechar, NumTime);
		break;
	case 1:
		strncpy(lastReadingTime, timechar, NumTime);
		break;
	case 2:
		strncpy(lastWritingTime, timechar, NumTime);
		break;
	default:
		break;
	}
}

char* 
FileHeader::GetTime(int i)
{
	switch(i)
	{
	case 0:
		return creatingTime;
	case 1:
		return lastReadingTime;
	case 2:
		return lastWritingTime;
	default:
		return NULL;
	}
}



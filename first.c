#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "first.h"
Cache curCache;
Simulation curSim;
long int curAge = 0;
int main(int argc, char **argv)
{
    FILE* traceFile = NULL;
    char* fileName = NULL;
    char accessType;
    long int curAddress = 0;
    char curLine[100];
    checkInput(argc, argv);
    fileName = argv[5];
    traceFile = fopen(fileName, "r");
    if(!traceFile)
    {
        printf("Couldn't open file\n");
        exit(0);
    }
    cacheSetup();
    for(int i = 0; i < 2; i++) //Read each file twice, one with prefetch and one without. Inefficient, no time to change currently
    {
        traceFile = fopen(fileName, "r");
        while(fgets(curLine, 100, traceFile) != NULL) //Get the entire line
        {
            if(strlen(curLine) == 5)
                break;
            sscanf(curLine, "%*x: %c %lx", &accessType, &curAddress); //Parse input
            simulate(accessType, curAddress);
        }
        printResults(); //Print the results for each simulation, no prefetch first, and then do prefetch
        freeCacheMem();
        cacheSetup();
        curCache.doPrefetch = true;
        fclose(traceFile);
    }
    freeCacheMem();
    return 0;
}
bool isPowerOfTwo(int x)
{
    return x && (!(x&(x-1)));
}
long int logTwo(long int x)
{
    long int i;
    for(i = 0; i < 47; i++)
    {
        if((x >> i) == 1)
        break;
    }
    return i;
}
long int getTagBits(long int address)
{
    long int numOffsetBits = logTwo(curCache.blockSize);
    long int numIndexBits = logTwo(curCache.numSets);
    long int tagBits = address >> (numIndexBits + numOffsetBits);
    return tagBits;
}
long int getIndexBits(long int address)
{
    if(strcmp(curCache.assocPolicy, "assoc") == 0)
        return 0;
    long int numOffsetBits = logTwo(curCache.blockSize);
    long int indexMask = (curCache.numSets - 1) << numOffsetBits;
    long int indexBits = (address&indexMask) >> numOffsetBits;
    return indexBits;
}
void checkInput(int argc, char **argv) //Makes sure all input is valid
{
    if(argc != 6)
    {
        printf("Not enough args\n");
        exit(0);
    }
    curCache.cacheSize = atoi(argv[1]);
    curCache.assocPolicy = argv[2];
    curCache.replacePolicy = argv[3];
    curCache.blockSize = atoi(argv[4]);
    if(!isPowerOfTwo(curCache.cacheSize))
    {
        printf("Cache size must be power of 2\n");
        exit(0);
    }
    if(!isPowerOfTwo(curCache.blockSize))
    {
        printf("Block size must be a power of 2\n");
        exit(0);
    }
    if(!(strcmp(curCache.replacePolicy,"LRU") == 0 || strcmp(curCache.replacePolicy,"lru") == 0 ||
         strcmp(curCache.replacePolicy, "fifo") == 0 || strcmp(curCache.replacePolicy, "FIFO") == 0))
    {
        printf("Invalid replacement policy\n");
        exit(0);
    }
    if(!(strcmp(curCache.assocPolicy, "direct") || strcmp(curCache.assocPolicy, "assoc") || strstr(curCache.assocPolicy, "assoc:")))
    {
        printf("Invalid Association Policy\n");
        exit(0);
    }
}
void cacheSetup()
{
    if(strcmp(curCache.assocPolicy , "assoc") == 0) //If fully associative, only 1 set, calulate blocks/set
    {
        curCache.numSets = 1;
        curCache.assoc = curCache.cacheSize/curCache.blockSize;
    }
    else if (strstr(curCache.assocPolicy, "assoc:") != NULL) //If N-way associative
    {
        int length = strlen(curCache.assocPolicy);
        int lengthOfNAssoc = length - 6;
        char *nAssocChar = &curCache.assocPolicy[length- lengthOfNAssoc];
        curCache.assoc = atoi(nAssocChar);
        if(!isPowerOfTwo(curCache.assoc))
        {
            printf("Associativity must be a power of two\n");
            exit(0);
        }
        curCache.numSets = curCache.cacheSize/(curCache.blockSize * curCache.assoc);
    }
    else if(strcmp(curCache.assocPolicy, "direct") == 0) //If direct, only one block/set
    {
        curCache.assoc = 1;
        curCache.numSets = curCache.cacheSize/curCache.blockSize;
    }
    allocateCacheMem();
    initializeValues();
}
void allocateCacheMem()
{
    curCache.baseSet = (Set*) malloc(curCache.numSets * sizeof(Set)); //Allocate memory for the sets
    for(int curSet = 0; curSet < curCache.numSets; curSet++)
    {
        curCache.baseSet[curSet].baseLine = (Line*) malloc(curCache.assoc * sizeof(Line)); //Allocate memory for line array in all sets
    }
}
void initializeValues()
{
    //Makes sure all valid bits are 0 to start, resets the simulator and the starting age for replacement
    for(int i = 0; i < curCache.numSets; i++)
    {
        Set* curSet = curCache.baseSet + i;
        for(int j = 0; j < curCache.assoc; j++)
        {
            Line* curLine = curSet->baseLine + j;
            curLine->valid = 0;
        }
    }
    curSim.reads = 0;
    curSim.writes = 0;
    curSim.hits = 0;
    curSim.misses = 0;
    curAge = 0;
}
void freeCacheMem()
{
    for(int curSet = 0; curSet < curCache.numSets; curSet++)
    {
        free(curCache.baseSet[curSet].baseLine);
    }
    free(curCache.baseSet);
}
void simulate(char accessType, long int curAddress)
{
    if(checkHit(curAddress)) //Check for a hit first
    {
        if(accessType == 'W') //Increment simulator based on access type
        {
            curSim.hits++;
            curSim.writes++;
        }
        else
        {
            curSim.hits++;
        }
    }
    else //Otherwise, we missed
    {
        if(!isSetFull(curAddress)) //If there is still room in the given set
        {
            writeNewData(curAddress); //Put the current address into the cache
            if(accessType == 'R') //Add to curSim based on access type
            {
                curSim.reads++;
                curSim.misses++;
            }
            else
            {
                curSim.reads++;
                curSim.misses++;
                curSim.writes++;
            }
        }
        else //Otherwise, the set is full, do eviction
        {
            doFifo(curAddress); //Perform FIFO on the current address
            if(accessType == 'R') //Add to curSim based on access type
            {
                curSim.reads++;
                curSim.misses++;
            }
            else
            {
                curSim.reads++;
                curSim.misses++;
                curSim.writes++;
            }
        }
        if(curCache.doPrefetch == true) //If the current cache is doing prefetching, go ahead and prefetch
        {
            prefetch(accessType, curAddress);
        }
    }
}
bool checkHit(long int curAddress)
{
    long int tagBits = getTagBits(curAddress);
    long int indexBits = getIndexBits(curAddress);
    Set* curSet = curCache.baseSet + indexBits; //Current set is the base set + the index bits
    for(int i = 0; i < curCache.assoc; i++)
    {
        Line* curLine = curSet->baseLine + i; //Current line is the baseLine + the current iteration for the loop
        if(curLine->tagBits == tagBits) //If a line's tag bits equal the addresses tag bits, we have a hit
            return true;
    }
    return false; //Otherwise, no hit
}
void writeNewData(long int curAddress) //Put an address into the cache
{
    long int tagBits = getTagBits(curAddress);
    long int indexBits = getIndexBits(curAddress);
    Set *curSet = curCache.baseSet + indexBits;
    for(int i = 0; i < curCache.assoc; i++)
    {
        Line* curLine = curSet->baseLine + i;
        if(curLine->valid == 0) //Find the first empty line in the appropriate set
        {
            curLine->valid = 1; //Validate the line
            curLine->tagBits = tagBits; //Set its tag bits and age, and then increment age
            curLine->age = curAge;
            curAge++;
        }
    }
}
void printResults()
{
    if(curCache.doPrefetch == false)
        printf("no-prefetch\n");
    else
        printf("with-prefetch\n");
    printf("Memory reads: %d\n", curSim.reads);
    printf("Memory writes: %d\n", curSim.writes);
    printf("Cache hits: %d\n", curSim.hits);
    printf("Cache misses: %d\n", curSim.misses);
}
bool isSetFull(long int curAddress)
{
    long int indexBits = getIndexBits(curAddress); //Check if a set is full
    Set* curSet = curCache.baseSet + indexBits;
    for(int i = 0; i < curCache.assoc; i++)
    {
        Line* curLine = curSet->baseLine + i; //Go through every line, and if we find a 0 valid bit, not full
        if(curLine->valid == 0)
        {
            return false;
        }
    }
    return true;
}
void prefetch(char accessType, long int curAddress)
{
    long int prefetchAddress = curAddress + curCache.blockSize; //Calculate the prefetch address
    if(!checkHit(prefetchAddress))  //If prefetched address is not in cache
    {
        if(!isSetFull(prefetchAddress)) //If it isnt full, just put the prefetch address into the cache
        {
            writeNewData(prefetchAddress);
            curSim.reads++;
        }
        else //Otherwise, set is full, and we need to do fifo using the prefetch address
        {
            doFifo(prefetchAddress);
            curSim.reads++;
        }
    }
}
void doFifo(long int curAddress) //Choppy implementation, will fix later time permitting
{
    long int indexBits = getIndexBits(curAddress);
    long int oldestAge = 999999999999999999; //Start our value high
    Line* oldestLine; //Pointer to the line that is the oldest. Older means a smaller age field
    Set* curSet = curCache.baseSet + indexBits;
    for(int i = 0; i < curCache.assoc; i++)
    {
        Line* curLine = curSet->baseLine + i;
        if(curLine->age < oldestAge) //If the age of the current line is less than the previous oldest line
        {
            oldestAge = curLine->age; //Update the oldest age
            oldestLine = curLine; //Update the oldest line
        }
    }
    oldestLine->tagBits = getTagBits(curAddress); //When done, change the oldest line's bits to the new address
    oldestLine->age = curAge; //Change the age of the oldest line to whatever cur age is
    curAge++;
}

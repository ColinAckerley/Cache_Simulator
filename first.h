#ifndef _first_h
#define _first_h
bool isPowerOfTwo(int);
long int logTwo(long int);
long int getTagBits(long int);
long int getIndexBits(long int);
void cacheSetup();
void checkInput(int, char**);
void allocateCacheMem();
typedef struct Line //A line has tag bits, a valid bit, and an age
{
    long int tagBits;
    int valid;
    int age;
} Line;

typedef struct Set //A set is an array of lines
{
    Line* baseLine;
} Set;

typedef struct Cache
{
    bool doPrefetch; //Should the given cache prefetch or not
    long int cacheSize; //Set by user input
    long int numSets; //Calculated
    long int blockSize; //Set by user input
    long int assoc; //Calculated or set by user input
    char* replacePolicy; //Set by user input
    char* assocPolicy; //Set by user input
    Set* baseSet; //A cache is an array of sets
} Cache;
typedef struct Simulation //Keep track of the current simulation stats easily
{
    int reads;
    int writes;
    int hits;
    int misses;
} Simulation;
void simulate(char, long int);
void printResults();
void freeCacheMem();
bool checkHit(long int);
bool isSetFull(long int);
void prefetch(char, long int);
void initializeValues();
void doFifo(long int);
void writeNewData(long int);
#endif

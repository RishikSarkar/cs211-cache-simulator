#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef struct Cache
{
    unsigned int valid;
    unsigned long int tag;
    unsigned long int replacementFIFO;
    //unsigned long int replacementLRU;
}Cache;

int hit = 0;
int miss = 0;
int readnum = 0;
int writenum = 0;
long int push = 0;

Cache** cache;

unsigned long int getSetIndex (unsigned long int address, int offsetbit, int setindexbit)
{
    return (address >> offsetbit) & ((1 << setindexbit) - 1);
}

unsigned long int getTagIndex (unsigned long int address, int move)
{
    return address >> move;
}

/*int getExtraCredit (char* ec)
{
    if (strcmp("fifo", ec) == 0)
    {
        return 0;
    }

    return 1;
}*/

char* getTraceFile (char** arg)
{
    return arg[5];
}

int getBlockSize (char** arg)
{
    return atoi(arg[4]);
}

int getCacheSize (char** arg)
{
    return atoi(arg[1]);
}

int getAssociativity (char** arg)
{
    char* associativity = arg[2];

    int c = getCacheSize(arg);
    int b = getBlockSize(arg);

    if (strcmp("direct", associativity) == 0)
    {
        return 1;
    }
    
    if (strcmp("assoc", associativity) == 0)
    {
        return c/b;
    }

    char num[1000];

    for (int i = 6, j = 0; i < strlen(associativity) && j < 1000; i++, j++)
    {
        num[j] = associativity[i];
    }

    return atoi(num);
}

int getSetNumber (char** arg)
{
    char* associativity = arg[2];
    int c = getCacheSize(arg);
    int b = getBlockSize(arg);

    if (strcmp("direct", associativity) == 0)
    {
        return c/b;
    }
    
    if (strcmp("assoc", associativity) == 0)
    {
        return 1;
    }

    char num[1000];

    for (int i = 6, j = 0; i < strlen(associativity) && j < 1000; i++, j++)
    {
        num[j] = associativity[i];
    }

    return c/b/atoi(num);
}

Cache** initialize (char** arg)
{
    int setNumber = getSetNumber(arg);
    int associativity = getAssociativity(arg);

    cache = (struct Cache**)malloc(sizeof(struct Cache) * setNumber);

    for (int i = 0; i < setNumber; i++)
    {
        cache[i] = (struct Cache*)malloc(sizeof(struct Cache) * associativity);

        for (int j = 0; j < associativity; j++)
        {
            cache[i][j].replacementFIFO = 0;
            cache[i][j].tag = 0;
            cache[i][j].valid = 0;
        }
    }

    hit = 0;
    push = 0;
    miss = 0;
    readnum = 0;
    writenum = 0;

    return cache;
}

void prefet (int associativity, unsigned long int secondSetIndex, unsigned long int secondTagIndex)
{
    for (int i = 0; i < associativity; i++)
    {
        if (cache[secondSetIndex][i].valid == 1 && cache[secondSetIndex][i].tag == secondTagIndex)
        {
            break;
        }
        else if (cache[secondSetIndex][i].valid == 1 && associativity == i + 1)
        {
            readnum++;

            long int maxnumber = LONG_MAX;

            for (int j = 0; j < associativity; j++)
            {
                if (maxnumber >= cache[secondSetIndex][j].replacementFIFO)
                {
                    maxnumber = cache[secondSetIndex][j].replacementFIFO;
                }
            }

            for (int j = 0; j < associativity; j++)
            {
                if (maxnumber == cache[secondSetIndex][j].replacementFIFO)
                {
                    cache[secondSetIndex][j].tag = secondTagIndex;
                    cache[secondSetIndex][j].replacementFIFO = push;
                    push++;
                    break;
                }
            }

            return;
        }
        else if (cache[secondSetIndex][i].valid == 0)
        {
            readnum++;

            cache[secondSetIndex][i].valid = 1;
            cache[secondSetIndex][i].tag = secondTagIndex;
            cache[secondSetIndex][i].replacementFIFO = push;

            push++;

            break;
        }
        else
        {
            continue;
        }
    }
}

/*void prefetchLRU(int associativity, unsigned long int secondSetIndex, unsigned long int secondTagIndex)
{
    for (int i = 0; i < associativity; i++)
    {
        if (cache[secondSetIndex][i].valid == 1 && cache[secondSetIndex][i].tag == secondTagIndex)
        {
            break;
        }
        else if (cache[secondSetIndex][i].valid == 1 && associativity == i + 1)
        {
            readnum++;

            long int maxnumber = LONG_MAX;

            for (int j = 0; j < associativity; j++)
            {
                if (maxnumber >= cache[secondSetIndex][j].replacementLRU)
                {
                    maxnumber = cache[secondSetIndex][j].replacementLRU;
                }
            }

            for (int j = 0; j < associativity; j++)
            {
                if (maxnumber == cache[secondSetIndex][j].replacementLRU)
                {
                    cache[secondSetIndex][j].tag = secondTagIndex;
                    cache[secondSetIndex][j].replacementLRU = 0;

                    for (int k = 0; k < associativity; k++)
                    {
                        if (j != k)
                        {
                            cache[secondSetIndex][k].replacementLRU--;
                        }
                    }

                    break;
                }
            }

            return;
        }
        else if (cache[secondSetIndex][i].valid == 0)
        {
            readnum++;

            cache[secondSetIndex][i].valid = 1;
            cache[secondSetIndex][i].tag = secondTagIndex;
            cache[secondSetIndex][i].replacementLRU = 0;

            for (int j = 0; j < associativity; j++)
            {
                if (i != j)
                {
                    cache[secondSetIndex][j].replacementLRU--;
                }
            }

            break;
        }
        else
        {
            continue;
        }
    }
}*/

void accesses (char status, int prefetch, /*int ec,*/ int associativity, unsigned long int tagIndex, unsigned long int setIndex,unsigned long int secondTagIndex,unsigned long int secondSetIndex)
{
    if (prefetch == 0) //no prefetch
    {
        for (int i = 0; i < associativity; i++)
        {
            if (cache[setIndex][i].valid == 1 && cache[setIndex][i].tag == tagIndex)
            {
                hit++;

                if (status == 'W')
                {
                    writenum++;
                }

                break;
            }
            else if (cache[setIndex][i].valid == 1 && associativity == i + 1)
            {
                miss++;
                readnum++;

                if (status == 'W')
                {
                    writenum++;
                }

                long int maxnumber = LONG_MAX;

                for (int j = 0; j < associativity; j++)
                {
                    if (maxnumber >= cache[setIndex][j].replacementFIFO)
                    {
                        maxnumber = cache[setIndex][j].replacementFIFO;
                    }
                }

                for (int j = 0; j < associativity; j++)
                {
                    if (maxnumber == cache[setIndex][j].replacementFIFO)
                    {
                        cache[setIndex][j].tag = tagIndex;
                        cache[setIndex][j].replacementFIFO = push;
                        push++;
                        break;
                    }
                }

                return;
            }
            else if (cache[setIndex][i].valid == 0)
            {
                miss++;
                readnum++;

                if (status == 'W')
                {
                    writenum++;
                }

                cache[setIndex][i].valid = 1;
                cache[setIndex][i].tag = tagIndex;
                cache[setIndex][i].replacementFIFO = push;
                push++;
                break;
            }
            else
            {
                continue;
            }
        }
    }
    else //prefetch
    {
        for (int i = 0; i < associativity; i++)
        {
            if (cache[setIndex][i].valid == 1 && cache[setIndex][i].tag == tagIndex)
            {
                hit++;

                if (status == 'W')
                {
                    writenum++;
                }

                break;
            }
            else if (cache[setIndex][i].valid == 1 && associativity == i + 1)
            {
                miss++;
                readnum++;

                if (status == 'W')
                {
                    writenum++;
                }

                long int maxnumber = LONG_MAX;

                for (int j = 0; j < associativity; j++)
                {
                    if (maxnumber >= cache[setIndex][j].replacementFIFO)
                    {
                        maxnumber = cache[setIndex][j].replacementFIFO;
                    }
                }

                for (int j = 0; j < associativity; j++)
                {
                    if (maxnumber == cache[setIndex][j].replacementFIFO)
                    {
                        cache[setIndex][j].tag = tagIndex;
                        cache[setIndex][j].replacementFIFO = push;
                        push++;
                        break;
                    }
                }

                prefet(associativity, secondSetIndex, secondTagIndex);

                return;
            }
            else if (cache[setIndex][i].valid == 0)
            {
                miss++;
                readnum++;

                if (status == 'W')
                {
                    writenum++;
                }

                cache[setIndex][i].valid = 1;
                cache[setIndex][i].tag = tagIndex;
                cache[setIndex][i].replacementFIFO = push;
                push++;

                prefet(associativity, secondSetIndex, secondTagIndex);

                break;
            }
            else
            {
                continue;
            }
        }
    }
}

void print()
{
    return;
}

unsigned long intlog2(unsigned long x)
{
    unsigned long l = 0;

    while (x > 1)
    {
        x >>= 1;
        l++;
    }

    return l;
}

/*double Log2(double n)
{
    return log(n) / log(2);
}*/

const char* __asan_default_options() 
{
    return "detect_leaks=0";
}

int main (int argc, char** argv)
{
    if (argc != 6)
    {
        return 0;
    }

    cache = initialize(argv);

    int associativity = getAssociativity(argv);
    int block = getBlockSize(argv);
    int set = getSetNumber(argv);

    //char* ec = argv[3];
    //int extraCredit = getExtraCredit(ec);

    char status;
    unsigned long int address;
    unsigned long int firstco;

    FILE* f = fopen(getTraceFile(argv), "r");
    int val = fscanf(f, "%lx: %c %lx", &firstco, &status, &address);
    int switcher = 0;

    while (val == 3)
    {
        int offsetBit = intlog2(block);
        int setIndexBit = intlog2(set);
        int sum = offsetBit + setIndexBit;

        unsigned long int tagIndex = getTagIndex(address, sum);
        unsigned long int setIndex = getSetIndex(address, offsetBit, setIndexBit);
        unsigned long int secondAddress = address + block;
        unsigned long int secondTagIndex = getTagIndex(secondAddress, sum);
        unsigned long int secondSetIndex = getSetIndex(secondAddress, offsetBit, setIndexBit);

        accesses(status, switcher, /*extraCredit,*/ associativity, tagIndex, setIndex, secondTagIndex, secondSetIndex);

        if (switcher == 1)
        {
            print();
        }

        val = fscanf(f, "%lx: %c %lx", &firstco, &status, &address);

        if (val != 3)
        {
            switcher++;

            if (switcher == 1)
            {
                printf("Prefetch 0\n");
                printf("Memory reads: %d\n", readnum);
                printf("Memory writes: %d\n", writenum);
                printf("Cache hits: %d\n", hit);
                printf("Cache misses: %d\n", miss);
            }
            else if (switcher == 2)
            {
                printf("Prefetch 1\n");
                printf("Memory reads: %d\n", readnum);
                printf("Memory writes: %d\n", writenum);
                printf("Cache hits: %d\n", hit);
                printf("Cache misses: %d\n", miss);
            }
            else
            {
                break;
            }

            f = fopen(getTraceFile(argv), "r");
            val = fscanf(f, "%lx: %c %lx", &firstco, &status, &address);
            cache = initialize(argv);
        }
    }

    for (int i = 0; i < set; i++)
    {
        free(cache[i]);
    }

    free(cache);

    return 0;
}

/*  if (ec == 0)
    {

    }
    else if (ec == 1)
    {
        if (prefetch == 0) //no prefetch
        {
            for (int i = 0; i < associativity; i++)
            {
                if (cache[setIndex][i].valid == 1 && cache[setIndex][i].tag == tagIndex)
                {
                    hit++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    cache[setIndex][i].replacementLRU = 0;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (i != j)
                        {
                            cache[setIndex][j].replacementLRU--;
                        }
                    }

                    break;
                }
                else if (cache[setIndex][i].valid == 1 && associativity == i + 1)
                {
                    miss++;
                    readnum++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    long int maxnumber = LONG_MAX;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (maxnumber >= cache[setIndex][j].replacementLRU)
                        {
                            maxnumber = cache[setIndex][j].replacementLRU;
                        }
                    }

                    for (int j = 0; j < associativity; j++)
                    {
                        if (maxnumber == cache[setIndex][j].replacementLRU)
                        {
                            cache[setIndex][j].tag = tagIndex;
                            cache[setIndex][j].replacementLRU = 0;

                            for (int k = 0; k < associativity; k++)
                            {
                                if (j != k)
                                {
                                    cache[setIndex][k].replacementLRU--;
                                }
                            }

                            break;
                        }
                    }

                    return;
                }
                else if (cache[setIndex][i].valid == 0)
                {
                    miss++;
                    readnum++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    cache[setIndex][i].valid = 1;
                    cache[setIndex][i].tag = tagIndex;
                    cache[setIndex][i].replacementLRU = 0;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (i != j)
                        {
                            cache[setIndex][j].replacementLRU--;
                        }
                    }

                    break;
                }
                else
                {
                    continue;
                }
            }
        }
        else //prefetch
        {
            for (int i = 0; i < associativity; i++)
            {
                if (cache[setIndex][i].valid == 1 && cache[setIndex][i].tag == tagIndex)
                {
                    hit++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    cache[setIndex][i].replacementLRU = 0;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (i != j)
                        {
                            cache[setIndex][j].replacementLRU--;
                        }
                    }

                    break;
                }
                else if (cache[setIndex][i].valid == 1 && associativity == i + 1)
                {
                    miss++;
                    readnum++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    long int maxnumber = LONG_MAX;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (maxnumber >= cache[setIndex][j].replacementLRU)
                        {
                            maxnumber = cache[setIndex][j].replacementLRU;
                        }
                    }

                    for (int j = 0; j < associativity; j++)
                    {
                        if (maxnumber == cache[setIndex][j].replacementLRU)
                        {
                            cache[setIndex][j].tag = tagIndex;
                            cache[setIndex][j].replacementLRU = 0;

                            for (int k = 0; k < associativity; k++)
                            {
                                if (j != k)
                                {
                                    cache[setIndex][k].replacementLRU--;
                                }
                            }

                            break;
                        }
                    }

                    prefetchLRU(associativity, secondSetIndex, secondTagIndex);

                    return;
                }
                else if (cache[setIndex][i].valid == 0)
                {
                    miss++;
                    readnum++;

                    if (status == 'W')
                    {
                        writenum++;
                    }

                    cache[setIndex][i].valid = 1;
                    cache[setIndex][i].tag = tagIndex;
                    cache[setIndex][i].replacementLRU = 0;

                    for (int j = 0; j < associativity; j++)
                    {
                        if (i != j)
                        {
                            cache[setIndex][j].replacementLRU--;
                        }
                    }

                    prefetchLRU(associativity, secondSetIndex, secondTagIndex);

                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    }*/
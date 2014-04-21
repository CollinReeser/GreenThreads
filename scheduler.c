#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define THREAD_DATA_ARR_START_LEN 4
#define THREAD_DATA_ARR_MUL_INCREASE 2

void hitASM()
{
    printf("Hit ASM\n");
}

void hit()
{
    printf("Hit\n");
}

extern void newProc(size_t argBytes, void* funcAddr, uint8_t* args);
extern void callFunc(size_t argBytes, void* funcAddr, uint8_t* args);

typedef struct
{
    // Address of function to exec
    void* funcAddr;
    // Bytes of arguments to function
    uint8_t* funcArgs;
    // Length of funcArgs array in bytes
    size_t funcArgsLen;
} ThreadData;

void callThreadFunc(ThreadData* thread)
{
    callFunc(thread->funcArgsLen, thread->funcAddr, thread->funcArgs);
}

void deallocThreadData(ThreadData* thread)
{
    // Dealloc memory for function arguments
    free(thread->funcArgs);
    // Dealloc memory for struct
    free(thread);
}

typedef struct
{
    // Array of managed green threads
    ThreadData** threadArr;
    // Length of managed green threads array
    uint32_t threadArrLen;
    // Index one past last valid ThreadData in threadArr
    uint32_t threadArrIndex;
} GlobalThreadMem;

GlobalThreadMem* g_threadManager = NULL;

void initThreadManager()
{
    // Alloc space for struct
    g_threadManager = (GlobalThreadMem*)malloc(sizeof(GlobalThreadMem));
    // Alloc initial space for ThreadData* array
    g_threadManager->threadArr =
        (ThreadData**)malloc(sizeof(ThreadData*) * THREAD_DATA_ARR_START_LEN);
    // Init ThreadData* array length tracker
    g_threadManager->threadArrLen = THREAD_DATA_ARR_START_LEN;
    // Init ThreadData* array index tracker
    g_threadManager->threadArrIndex = 0;
}

void takedownThreadManager()
{
    uint32_t i;
    // Loop over all valid ThreadData* and dealloc them
    for (i = 0; i < g_threadManager->threadArrIndex; i++)
    {
        deallocThreadData(g_threadManager->threadArr[i]);
    }
    // Dealloc memory for ThreadData*
    free(g_threadManager->threadArr);
    // Dealloc memory for struct
    free(g_threadManager);
}

void addThreadData(size_t argBytes, void* funcAddr, ...)
{
    // Alloc new ThreadData
    ThreadData* newThread = (ThreadData*)malloc(sizeof(ThreadData));
    // Init the address of the function this green thread manages
    newThread->funcAddr = funcAddr;
    // Init the length in bytes of the function arguments in total
    newThread->funcArgsLen = argBytes;
    // Get the address on the stack that the arguments start at
    uint8_t* vargArgStart = (uint8_t*)(&funcAddr) + sizeof(funcAddr);
    // Allocate space for copying the function arguments off the stack
    uint8_t* funcArgs = (uint8_t*)malloc(sizeof(uint8_t) * argBytes);
    // Copy the function arguments into the ThreadData-managed array
    memcpy(funcArgs, vargArgStart, argBytes);
    // Give the ThreadData object ownership of the arguments pointer
    newThread->funcArgs = funcArgs;
    // Put newThread into global thread manager, allocating space for the
    // pointer if necessary
    if (g_threadManager->threadArrIndex < g_threadManager->threadArrLen)
    {
        // Place pointer into ThreadData* array
        g_threadManager->threadArr[g_threadManager->threadArrIndex] = newThread;
        // Increment index
        g_threadManager->threadArrIndex++;
    }
    // We need to allocate more space for the threadArr array
    else
    {
        // Allocate more space for thread manager
        g_threadManager->threadArr = (ThreadData**)realloc(
            g_threadManager->threadArr,
            g_threadManager->threadArrLen * THREAD_DATA_ARR_MUL_INCREASE);
        g_threadManager->threadArrLen =
            g_threadManager->threadArrLen * THREAD_DATA_ARR_MUL_INCREASE;
        // Place pointer into ThreadData* array
        g_threadManager->threadArr[g_threadManager->threadArrIndex] = newThread;
        // Increment index
        g_threadManager->threadArrIndex++;
    }
}

void execAllManagedFuncs()
{
    uint32_t i = 0;
    for (i = 0; i < g_threadManager->threadArrIndex; i++)
    {
        ThreadData* curThread = g_threadManager->threadArr[i];
        callFunc(curThread->funcArgsLen, curThread->funcAddr,
            curThread->funcArgs);
    }
}

// Example function. Hailstone sequence
void example_func(uint8_t start)
{
    printf("Start is: [%d]\n", start);
    while (start > 1)
    {
        if (start % 2 == 0)
        {
            start = start / 2;
        }
        else
        {
            start = start * 3 + 1;
        }
        printf("  Val: %u\n", start);
    }
}

void average_novararg(uint8_t one, uint8_t two, uint8_t three)
{
    printf("One    : %u\n", one);
    printf("Two    : %u\n", two);
    printf("Three  : %u\n", three);
    printf("Average: %f\n", (one + two + three) / 3.0);
}

// newProc(size_t argBytes, void* funcAddr, uint8_t* args);

int main(int argc, char** argv)
{
    initThreadManager();

    uint8_t* args = (uint8_t*)malloc(sizeof(uint8_t) * 3);
    args[0] = 10;
    args[1] = 20;
    args[2] = 30;
    newProc(3, &average_novararg, args);
    args[0] = 20;
    args[1] = 30;
    args[2] = 40;
    newProc(3, &average_novararg, args);
    args[0] = 30;
    args[1] = 40;
    args[2] = 50;
    newProc(3, &average_novararg, args);
    free(args);

    execAllManagedFuncs();

    takedownThreadManager();
    return 0;
}

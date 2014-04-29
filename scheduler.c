#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include "channel.h"
#include "scheduler.h"

#define THREAD_DATA_ARR_START_LEN 4
#define THREAD_DATA_ARR_MUL_INCREASE 2
#define THREAD_STACK_SIZE (4096*64)

GlobalThreadMem* g_threadManager = NULL;

void hitASM()
{
    printf("Hit ASM\n");
}

void hit()
{
    printf("Hit\n");
}

void printThreadData(ThreadData* curThread)
{
    printf("Print Thread Data:\n");
    printf("    ThreadData* curThread: %X\n", curThread);
    printf("    funcAddr             : %X\n", curThread->funcAddr);
    printf("    curFuncAddr          : %X\n", curThread->curFuncAddr);
    printf("    funcArgsLen          : %u\n", curThread->funcArgsLen);
    printf("    t_StackBot           : %X\n", curThread->t_StackBot);
    printf("    t_StackCur           : %X\n", curThread->t_StackCur);
    printf("    t_StackRaw           : %X\n", curThread->t_StackRaw);
    printf("    t_ebp                : %X\n", curThread->t_ebp);
    printf("    stillValid           : %u\n", curThread->stillValid);
}

void callThreadFunc(ThreadData* thread)
{
    callFunc(thread->funcArgsLen, thread->funcAddr, thread->t_StackBot, thread);
}

void deallocThreadData(ThreadData* thread)
{
    // Unmap memory allocated for thread stack
    munmap(thread->t_StackRaw, THREAD_STACK_SIZE);
    // Dealloc memory for struct
    free(thread);
}

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

void addThreadData(uint32_t argBytes, void* funcAddr, ...)
{
    // Alloc new ThreadData
    ThreadData* newThread = (ThreadData*)malloc(sizeof(ThreadData));
    // Init the address of the function this green thread manages
    newThread->funcAddr = funcAddr;
    // Init the address of the function this green thread manages
    newThread->curFuncAddr = 0;
    // Thread starts off 0, meaning curFuncAddr should also be checked
    // to see if the thread simply hasn't started yet
    newThread->stillValid = 0;
    // Thread starts off with unitialized stack frame pointer
    newThread->t_ebp = 0;
    // Init the length in bytes of the function arguments in total
    newThread->funcArgsLen = argBytes;
    // mmap thread stack
    newThread->t_StackRaw = (uint8_t*)mmap(NULL, THREAD_STACK_SIZE,
                                           PROT_READ|PROT_WRITE,
                                           MAP_PRIVATE|MAP_ANONYMOUS,
                                           -1, 0);
    // StackCur starts as a meaningless pointer
    newThread->t_StackCur = 0;
    // Make t_StackBot point to "bottom" of stack (highest address)
    newThread->t_StackBot = newThread->t_StackRaw + THREAD_STACK_SIZE - 1;
    // Get the address on the stack that the arguments start at
    uint8_t* vargArgStart = (uint8_t*)(&funcAddr) + sizeof(funcAddr);
    // Copy function arguments into bottom of stack (highest addresses)
    memcpy(newThread->t_StackBot - argBytes, vargArgStart, argBytes);
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
            sizeof(ThreadData*) * g_threadManager->threadArrLen *
                THREAD_DATA_ARR_MUL_INCREASE);
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
    uint8_t stillValid = 0;
    for (i = 0; i < g_threadManager->threadArrIndex; i++)
    {
        ThreadData* curThread = g_threadManager->threadArr[i];
        if (curThread->stillValid != 0 || curThread->curFuncAddr == 0)
        {
            stillValid = 1;
            callThreadFunc(curThread);
        }
        if (i + 1 >= g_threadManager->threadArrIndex && stillValid != 0)
        {
            i = -1;
            stillValid = 0;
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>

#define THREAD_DATA_ARR_START_LEN 4
#define THREAD_DATA_ARR_MUL_INCREASE 2
#define THREAD_STACK_SIZE (4096*64)

void hitASM()
{
    printf("Hit ASM\n");
}

void hit()
{
    printf("Hit\n");
}

extern void newProc(size_t argBytes, void* funcAddr, uint8_t* args);
extern void callFunc(size_t argBytes, void* funcAddr, uint8_t* stackPtr);

typedef struct
{
    // Address of function to exec
    void* funcAddr;
    // Number of bytes that make up arguments. Probably multiple of 4
    size_t funcArgsLen;
    // Pointer to bottom of allocated stack (that grows DOWNWARD). That is,
    // this is a pointer to the highest address valid in the stack
    uint8_t* t_StackBot;
    // Pointer to the beginning of the memory area allocated for the stack.
    // This is what was originally returned by mmap, and what should be used
    // with munmap
    uint8_t* t_StackRaw;
} ThreadData;

void callThreadFunc(ThreadData* thread)
{
    callFunc(thread->funcArgsLen, thread->funcAddr, thread->t_StackBot);
}

void deallocThreadData(ThreadData* thread)
{
    // Unmap memory allocated for thread stack
    munmap(thread->t_StackRaw, THREAD_STACK_SIZE);
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
    // mmap thread stack
    newThread->t_StackRaw = (uint8_t*)mmap(NULL, THREAD_STACK_SIZE,
                                           PROT_READ|PROT_WRITE,
                                           MAP_PRIVATE|MAP_ANONYMOUS,
                                           -1, 0);
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
            curThread->t_StackBot);
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

    uint32_t* args = (uint32_t*)malloc(sizeof(uint32_t) * 3);
    args[0] = 10;
    args[1] = 20;
    args[2] = 30;
    newProc(sizeof(uint32_t) * 3, &average_novararg, (uint8_t*)args);
    args[0] = 20;
    args[1] = 30;
    args[2] = 40;
    newProc(sizeof(uint32_t) * 3, &average_novararg, (uint8_t*)args);
    args[0] = 30;
    args[1] = 40;
    args[2] = 50;
    newProc(sizeof(uint32_t) * 3, &average_novararg, (uint8_t*)args);
    free(args);

    execAllManagedFuncs();

    takedownThreadManager();
    return 0;
}

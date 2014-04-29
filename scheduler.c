#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include "channel.h"

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

typedef struct
{
    // Address of function to exec
    void* funcAddr;
    // Current position in function (0 if start of function)
    // Will be the value that eip needs to be to continue execution
    void* curFuncAddr;
    // Number of bytes that make up arguments. Probably multiple of 4
    uint32_t funcArgsLen;
    // Pointer to bottom of allocated stack (that grows DOWNWARD). That is,
    // this is a pointer to the highest address valid in the stack
    uint8_t* t_StackBot;
    // This is a pointer to the current value of the stack as should be used
    // for execution. That is, after returning to this thread from being
    // yielded away from it, set esp to this value
    uint8_t* t_StackCur;
    // Pointer to the beginning of the memory area allocated for the stack.
    // This is what was originally returned by mmap, and what should be used
    // with munmap
    uint8_t* t_StackRaw;
    // ebp of thread, other portion of saving stack information. 0 if un-init
    uint8_t* t_ebp;
    // Whether this thread has finished execution or not. Non-zero if still
    // valid, 0 if the thread is finished or the thread has not started yet.
    // That is to say, the thread is finished if curFuncAddr is non-zero and
    // stillValid is 0, and the thread is still valid if stillValid != 0 OR
    // curFuncAddr == 0
    uint8_t stillValid;
} ThreadData;

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

extern void newProc(uint32_t argBytes, void* funcAddr, uint8_t* args);
extern void callFunc(uint32_t argBytes, void* funcAddr, uint8_t* stackPtr, ThreadData* curThread);
extern void yield(uint32_t status);

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
    printf("One    : %3u, address: %X\n", one, &one);
    printf("Two    : %3u, address: %X\n", two, &two);
    printf("Three  : %3u, address: %X\n", three, &three);
    yield(1);
    printf("Average: %f\n", (one + two + three) / 3.0);
}

void hailstone(uint32_t start)
{
    uint32_t iter = start;
    uint32_t count = 1;
    printf("Start at %u\n", start);
    while (iter > 1)
    {
        if (iter % 2 == 0)
        {
            iter /= 2;
            count++;
        }
        else
        {
            iter = iter * 3 + 1;
            count++;
        }
        yield(1);
    }
    printf("%5u takes %5u (%X) steps.\n", start, count, &count);
}

int fib(int x)
{
    if (x == 0)
    {
        return 0;
    }
    if (x == 1)
    {
        return 1;
    }
    yield(1);
    return fib(x-1)+fib(x-2);
}

void printFib(int x)
{
    int result = fib(x);
    printf("The %uth fibonacci number is %u\n", x, result);
}

void numProducer(uint32_t num, uint32_t willProduce, Channel* chan)
{
    uint32_t multiplier = 1;
    uint32_t i = 0;
    while (i < willProduce)
    {
        uint32_t written = write(chan, 1, num * multiplier);
        if (written == 1)
        {
            printf("Produced value %5u on channel %X\n", num * multiplier, chan);
            multiplier++;
            i++;
        }
        else
        {
            printf("Value not yet read on channel %X\n", chan);
        }
        yield(1);
    }
}

void numConsumer(uint32_t expects, Channel* chan)
{
    uint32_t numRead = 0;
    uint32_t value;
    uint32_t success = 0;
    while (numRead < expects)
    {
        yield(1);
        success = read_1(chan, &value);
        if (success)
        {
            numRead++;
            printf("Got value %5u from channel %X\n", value, chan);
        }
        else
        {
            printf("No value available on channel %X\n", chan);
        }
    }
}

// newProc(uint32_t argBytes, void* funcAddr, uint8_t* args);

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

    args = (uint32_t*)malloc(sizeof(uint32_t));
    args[0] = 10;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);
    args[0] = 15;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);
    args[0] = 20;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);
    args[0] = 25;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);
    args[0] = 30;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);
    args[0] = 35;
    newProc(sizeof(uint32_t) * 1, &hailstone, (uint8_t*)args);

    args[0] = 10;
    newProc(sizeof(uint32_t) * 1, &printFib, (uint8_t*)args);
    args[0] = 20;
    newProc(sizeof(uint32_t) * 1, &printFib, (uint8_t*)args);
    args[0] = 30;
    newProc(sizeof(uint32_t) * 1, &printFib, (uint8_t*)args);
    free(args);

    Channel* chan_1 = createChannel(1);
    Channel* chan_2 = createChannel(1);
    Channel* chan_3 = createChannel(1);
    args = (uint32_t*)malloc(sizeof(uint32_t) * 3);
    args[0] = 10;
    args[1] = 5;
    args[2] = (uint32_t)chan_1;
    newProc(sizeof(uint32_t) * 3, &numProducer, (uint8_t*)args);
    args[0] = 11;
    args[1] = 5;
    args[2] = (uint32_t)chan_2;
    newProc(sizeof(uint32_t) * 3, &numProducer, (uint8_t*)args);
    args[0] = 13;
    args[1] = 5;
    args[2] = (uint32_t)chan_3;
    newProc(sizeof(uint32_t) * 3, &numProducer, (uint8_t*)args);
    free(args);

    args = (uint32_t*)malloc(sizeof(uint32_t) * 2);
    args[0] = 5;
    args[1] = (uint32_t)chan_1;
    newProc(sizeof(uint32_t) * 2, &numConsumer, (uint8_t*)args);
    args[0] = 5;
    args[1] = (uint32_t)chan_2;
    newProc(sizeof(uint32_t) * 2, &numConsumer, (uint8_t*)args);
    args[0] = 5;
    args[1] = (uint32_t)chan_3;
    newProc(sizeof(uint32_t) * 2, &numConsumer, (uint8_t*)args);
    free(args);

    execAllManagedFuncs();

    takedownThreadManager();
    destroyChannel(chan_1);
    destroyChannel(chan_2);
    destroyChannel(chan_3);

    return 0;
}

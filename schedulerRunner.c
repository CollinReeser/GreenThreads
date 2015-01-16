#include <stdio.h>
#include <stdlib.h>
#include "channel.h"
#include "scheduler.h"

void average_novararg(uint8_t one, uint8_t two, uint8_t three)
{
    printf("One    : %3u, address: %X\n", one, &one);
    printf("Two    : %3u, address: %X\n", two, &two);
    printf("Three  : %3u, address: %X\n", three, &three);
    yield();
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
        yield();
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
    yield();
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
        uint32_t written = writeChannel(chan, 1, num * multiplier);
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
        yield();
    }
    closeChannel(chan);
}

void numConsumer(Channel* chan)
{
    uint32_t value;
    uint32_t success = 0;
    while (isChannelOpen(chan))
    {
        success = readChannel_1(chan, &value);
        if (success)
        {
            printf("Got value %5u from channel %X\n", value, chan);
        }
        else
        {
            printf("No value available on channel %X\n", chan);
        }
        yield();
    }
    destroyChannel(chan);
}

void spawnInner(int count, int endVal)
{
    printf("Entered spawnInner with count: %u :: endVal: %u\n", count, endVal);
    if (count < endVal)
    {
        int i;
        for (i = 0; i <= count; i++)
        {
            int8_t* argLens = malloc(2);
            void* args = malloc(16);
            argLens[0] = 4;
            argLens[1] = 4;
            ((uint64_t*)args)[0] = count + 1;
            ((uint64_t*)args)[1] = endVal;
            newProc(2, &spawnInner, argLens, args);
            free(argLens);
            free(args);
            printf("  Created new thread. count: %u :: endVal: %u\n", count + 1, endVal);
            printf("    Yielding...\n");
            yield();
        }
    }
}

void spawnMany(int endVal)
{
    printf("Exec-ing spawnInner with endVal: %u\n", endVal);
    spawnInner(0, endVal);
}

void threadMain()
{
    int8_t* argLens;
    void* args;
    printf("Instantiating average_novararg\n");
    argLens = malloc(3);
    args = malloc(24);
    argLens[0] = 1;
    argLens[1] = 1;
    argLens[2] = 1;
    ((uint64_t*)args)[0] = 10;
    ((uint64_t*)args)[1] = 20;
    ((uint64_t*)args)[2] = 30;
    newProc(3, &average_novararg, argLens, args);
    printf("Instantiating average_novararg\n");
    ((uint64_t*)args)[0] = 20;
    ((uint64_t*)args)[1] = 30;
    ((uint64_t*)args)[2] = 40;
    newProc(3, &average_novararg, argLens, args);
    printf("Instantiating average_novararg\n");
    ((uint64_t*)args)[0] = 30;
    ((uint64_t*)args)[1] = 40;
    ((uint64_t*)args)[2] = 50;
    newProc(3, &average_novararg, argLens, args);

    printf("Instantiating hailstone\n");
    argLens[0] = 4;
    ((uint64_t*)args)[0] = 10;
    newProc(1, &hailstone, argLens, args);
    printf("Instantiating hailstone\n");
    ((uint64_t*)args)[0] = 15;
    newProc(1, &hailstone, argLens, args);
    printf("Instantiating hailstone\n");
    ((uint64_t*)args)[0] = 20;
    newProc(1, &hailstone, argLens, args);
    printf("Instantiating hailstone\n");
    ((uint64_t*)args)[0] = 25;
    newProc(1, &hailstone, argLens, args);
    printf("Instantiating hailstone\n");
    ((uint64_t*)args)[0] = 30;
    newProc(1, &hailstone, argLens, args);
    printf("Instantiating hailstone\n");
    ((uint64_t*)args)[0] = 35;
    newProc(1, &hailstone, argLens, args);

    printf("Instantiating printFib\n");
    argLens[0] = 4;
    ((uint64_t*)args)[0] = 10;
    newProc(1, &printFib, argLens, args);
    printf("Instantiating printFib\n");
    ((uint64_t*)args)[0] = 20;
    newProc(1, &printFib, argLens, args);
    printf("Instantiating printFib\n");
    ((uint64_t*)args)[0] = 30;
    newProc(1, &printFib, argLens, args);

    Channel* chan_1 = createChannel(1);
    Channel* chan_2 = createChannel(1);
    Channel* chan_3 = createChannel(1);
    printf("Instantiating numProducer\n");
    argLens[0] = 4;
    argLens[1] = 4;
    argLens[2] = 8;
    ((uint64_t*)args)[0] = 10;
    ((uint64_t*)args)[1] = 5;
    ((uint64_t*)args)[2] = (uint64_t)chan_1;
    newProc(3, &numProducer, argLens, args);
    printf("Instantiating numProducer\n");
    ((uint64_t*)args)[0] = 11;
    ((uint64_t*)args)[1] = 5;
    ((uint64_t*)args)[2] = (uint64_t)chan_2;
    newProc(3, &numProducer, argLens, args);
    printf("Instantiating numProducer\n");
    ((uint64_t*)args)[0] = 13;
    ((uint64_t*)args)[1] = 5;
    ((uint64_t*)args)[2] = (uint64_t)chan_3;
    newProc(3, &numProducer, argLens, args);
    printf("Instantiating numConsumer\n");
    argLens[0] = 8;
    ((uint64_t*)args)[0] = (uint64_t)chan_1;
    newProc(1, &numConsumer, argLens, args);
    printf("Instantiating numConsumer\n");
    ((uint64_t*)args)[0] = (uint64_t)chan_2;
    newProc(1, &numConsumer, argLens, args);
    printf("Instantiating numConsumer\n");
    ((uint64_t*)args)[0] = (uint64_t)chan_3;
    newProc(1, &numConsumer, argLens, args);
    printf("Instantiating spawnMany\n");
    argLens[0] = 4;
    ((uint64_t*)args)[0] = 3;
    newProc(1, &spawnMany, argLens, args);

    free(args);
    free(argLens);
}

int main(int argc, char** argv)
{
    initThreadManager();

    newProc(0, &threadMain, NULL, NULL);

    execScheduler();

    takedownThreadManager();

    return 0;
}

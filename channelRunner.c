#include <stdio.h>
#include "channel.h"

void main()
{
    Channel* chan = createChannel(10);
    uint32_t value = 0;
    printChannel(chan);

    printf("\n");

    uint32_t numWritten = write(chan, 3, 10, 20, 30);
    printf("Wrote %u uints to channel\n", numWritten);
    printChannel(chan);

    readChannel_1(chan, &value);
    printf("Read value: %u\n", value);
    printChannel(chan);

    printf("\n");

    numWritten = write(chan, 3, 40, 50, 60);
    printf("Wrote %u uints to channel\n", numWritten);
    printChannel(chan);

    readChannel_1(chan, &value);
    printf("Read value: %u\n", value);
    printChannel(chan);

    printf("\n");

    numWritten = write(chan, 3, 70, 80, 90);
    printf("Wrote %u uints to channel\n", numWritten);
    printChannel(chan);

    readChannel_1(chan, &value);
    printf("Read value: %u\n", value);
    printChannel(chan);

    printf("\n");

    numWritten = write(chan, 3, 100, 110, 120);
    printf("Wrote %u uints to channel\n", numWritten);
    printChannel(chan);

    readChannel_1(chan, &value);
    printf("Read value: %u\n", value);
    printChannel(chan);

    printf("\n");

    numWritten = write(chan, 3, 130, 140, 150);
    printf("Wrote %u uints to channel\n", numWritten);
    printChannel(chan);

    readChannel_1(chan, &value);
    printf("Read value: %u\n", value);
    printChannel(chan);

    uint32_t success = 0;
    while (success = readChannel_1(chan, &value))
    {
        printf("Read value: %u\n", value);
        printChannel(chan);
    }

    printf("No more values in channel!\n");

    destroyChannel(chan);
}

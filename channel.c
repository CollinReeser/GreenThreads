#include <stdio.h>
#include "channel.h"

Channel* createChannel(size_t length)
{
    Channel* chan = (Channel*)malloc(sizeof(Channel));
    chan->buffer = (uint32_t*)malloc(sizeof(uint32_t) * length);
    chan->length = length;
    chan->index = 0;
    chan->numValid = 0;
    return chan;
}

// Returns number of elements (not bytes) that were successfully written to the
// buffer
uint32_t write(Channel* chan, uint32_t numElems, ...)
{
    uint32_t* buffer = chan->buffer;
    uint32_t length = chan->length;
    uint32_t index = chan->index;
    uint32_t numValid = chan->numValid;
    uint32_t spotsLeft = length - numValid;
    uint32_t elemsWritten = 0;
    // Check if there is even room in the buffer
    if (spotsLeft == 0)
    {
        // There is no room in the buffer, fail to write and return
        return 0;
    }
    uint32_t curIndex = index + numValid;
    if (curIndex >= length)
    {
        curIndex = index + numValid - length;
    }
    uint32_t i;
    va_list ap;
    va_start(ap, numElems);
    for(i = 0; i < numElems; i++)
    {
        buffer[curIndex] = va_arg(ap, uint32_t);
        curIndex++;
        if (curIndex == length)
        {
            curIndex = 0;
        }
        spotsLeft--;
        elemsWritten++;
        if (spotsLeft == 0)
        {
            break;
        }
    }
    chan->numValid = numValid + elemsWritten;
    va_end(ap);
    return elemsWritten;
}

// Places read value into location value, returns non-zero for success,
// zero for failure
uint32_t read_1(Channel* chan, uint32_t* value)
{
    uint32_t* buffer = chan->buffer;
    uint32_t length = chan->length;
    uint32_t index = chan->index;
    uint32_t numValid = chan->numValid;
    // Check if there are any values to read
    if (numValid == 0)
    {
        value = 0;
        return 0;
    }
    *value = buffer[index];
    numValid--;
    index++;
    if (index >= length)
    {
        index = 0;
    }
    chan->index = index;
    chan->numValid = numValid;
    return 1;
}

void destroyChannel(Channel* chan)
{
    free(chan->buffer);
    free(chan);
}

void printChannel(Channel* chan)
{
    printf("Printing channel: %X\n", chan);
    printf("    buffer  : %X\n", chan->buffer);
    uint32_t i = 0;
    for (i = 0; i < chan->length; i++)
    {
        printf("        buffer[%u]: %6u\n", i, chan->buffer[i]);
    }
    printf("    length  : %u\n", chan->length);
    printf("    index   : %u\n", chan->index);
    printf("    numValid: %u\n", chan->numValid);
}

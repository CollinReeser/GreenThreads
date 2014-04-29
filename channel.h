#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

typedef struct
{
    // Actual allocated memory
    uint32_t* buffer;
    // Length of buffer in number of elements
    size_t length;
    // Index into buffer that valid entries start
    uint32_t index;
    // Number of valid elements that can be read (have been written and have
    // not been read yet)
    uint32_t numValid;
} Channel;

Channel* createChannel(size_t length);

// Returns number of elements (not bytes) that were successfully written to the
// buffer
uint32_t write(Channel* chan, uint32_t numElems, ...);

// Places read value into location value, returns non-zero for success,
// zero for failure
uint32_t read_1(Channel* chan, uint32_t* value);

void destroyChannel(Channel* chan);

void printChannel(Channel* chan);

#endif

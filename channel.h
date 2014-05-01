#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdint.h>

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
    // Whether or not the channel itself is valid. Writes to a closed channel
    // will fail, and reads from a closed channel will work until no more
    // elements are available. 0 for false, anything else for true
    uint32_t isOpen;
} Channel;

Channel* createChannel(size_t length);

// Returns number of elements (not bytes) that were successfully written to the
// buffer
uint32_t writeChannel(Channel* chan, uint32_t numElems, ...);

// Places read value into location value, returns non-zero for success,
// zero for failure
uint32_t readChannel_1(Channel* chan, uint32_t* value);

// Indicate that write operations are no longer permitted
void closeChannel(Channel* chan);

// Check whether the channel is open
uint32_t isChannelOpen(Channel* chan);

void destroyChannel(Channel* chan);

void printChannel(Channel* chan);

#endif

#pragma once
#include "libtwl/rtos/rtosMutex.h"

class DSPicoUsbInEndpoint
{
public:
    explicit DSPicoUsbInEndpoint(u8 endpointAddress)
        : _endpointAddress(endpointAddress), _currentDSPicoBuffer(0), _buffer(nullptr)
        , _bufferLength(0), _remaining(0), _bufferOffset(0), _transferActive(false)
    {
        rtos_createMutex(&_mutex);
    }

    void BeginTransfer(const u8* buffer, u32 length);
    void ProcessTransferCompleteEvent(u32 transferredBytes);

private:
    u8 _endpointAddress;
    u8 _currentDSPicoBuffer;
    const u8* _buffer;
    u32 _bufferLength;
    u32 _remaining;
    u32 _bufferOffset;
    rtos_mutex_t _mutex;
    bool _transferActive;

    void SendUsbBlock(bool startTransfer);
};

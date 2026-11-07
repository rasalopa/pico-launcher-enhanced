#pragma once

class DSPicoUsbOutEndpoint
{
public:
    explicit DSPicoUsbOutEndpoint(u8 endpointAddress)
        : _endpointAddress(endpointAddress), _currentDSPicoBuffer(0), _buffer(nullptr)
        , _bufferLength(0), _bufferOffset(0), _totalReceived(0), _transferActive(false)
    {
        rtos_createMutex(&_mutex);
    }

    void BeginTransfer(u8* buffer, u32 length);
    void ProcessTransferCompleteEvent(u32 transferredBytes);
private:
    u8 _endpointAddress;
    u8 _currentDSPicoBuffer;
    u8* _buffer;
    u32 _bufferLength;
    u32 _bufferOffset;
    u32 _totalReceived;
    bool _transferActive;
    rtos_mutex_t _mutex;

    void ReceiveUsbBlock();
};

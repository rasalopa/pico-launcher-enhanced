#pragma once
#include "ipc/ThreadIpcService.h"
#include "usbMscIpcCommand.h"
#include "ipcChannels.h"

/// @brief Answers the ARM9's mount request: tries to put the microSD on the
///        USB bus and reports whether a host was there to take it.
///
/// It runs on its own thread because the mount waits for the host, and the
/// cart bus (and with it the rest of the launcher) must not wait with it.
class UsbMscIpcService : public ThreadIpcService
{
    u32 _threadStack[256];

public:
    UsbMscIpcService()
        : ThreadIpcService(IPC_CHANNEL_USB_MSC, 6, _threadStack, sizeof(_threadStack)) { }

    void HandleMessage(u32 data) override;
};

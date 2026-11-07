#include "common.h"
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/rtos/rtosEvent.h>
#include "ipcChannels.h"
#include "usbMscIpcCommand.h"
#include "UsbMscIpc.h"

static rtos_event_t sEvent;
static bool sMounted = false;
/// The ARM7's answer to the one host question the launcher asks at startup.
static bool sHostPresent = false;
static bool sInitialized = false;

static void ipcMessageHandler(u32 channel, u32 data, void* arg)
{
    (void)channel;
    (void)arg;

    if (USB_MSC_IPC_IS_REPLY_HOST(data))
    {
        sHostPresent = USB_MSC_IPC_HOST_BIT(data) != 0;
        rtos_signalEvent(&sEvent);
        return;
    }
    sMounted = data != 0;
    rtos_signalEvent(&sEvent);
}

/// Registered on first use: the channel only exists for this one question, so
/// nothing is set up at boot for a feature most sessions never touch.
static void ensureInitialized()
{
    if (sInitialized)
        return;
    rtos_createEvent(&sEvent);
    ipc_setChannelHandler(IPC_CHANNEL_USB_MSC, ipcMessageHandler, nullptr);
    sInitialized = true;
}

bool UsbMscMountMicroSd()
{
    ensureInitialized();

    rtos_clearEvent(&sEvent);
    ipc_sendFifoMessage(IPC_CHANNEL_USB_MSC, USB_MSC_IPC_CMD_MOUNT);
    rtos_waitEvent(&sEvent, false, true);

    return sMounted;
}

bool UsbMscQueryHostPresent()
{
    ensureInitialized();

    rtos_clearEvent(&sEvent);
    ipc_sendFifoMessage(IPC_CHANNEL_USB_MSC, USB_MSC_IPC_CMD_QUERY_HOST);
    rtos_waitEvent(&sEvent, false, true);

    return sHostPresent;
}

bool UsbMscHostPresent()
{
    return sHostPresent;
}

bool UsbMscIsMounted()
{
    return sMounted;
}

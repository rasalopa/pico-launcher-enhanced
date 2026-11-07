#include "common.h"
#include "usbMscIpcCommand.h"
#include "UsbMscIpcService.h"
#include "UsbMscMode.h"

void UsbMscIpcService::HandleMessage(u32 data)
{
    switch (data)
    {
        case USB_MSC_IPC_CMD_MOUNT:
            SendResponseMessage(UsbMscMount() == UsbMscMountResult::Mounted ? 1 : 0);
            break;
        case USB_MSC_IPC_CMD_QUERY_HOST:
            // Answered from the device stack's own view of the bus, so this
            // returns at once: nothing has to be put on the bus to ask.
            SendResponseMessage(USB_MSC_IPC_REPLY_HOST(UsbMscIsHostPresent()));
            break;
        default:
            break;
    }
}

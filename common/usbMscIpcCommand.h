#pragma once

/// @brief Commands the ARM9 sends on IPC_CHANNEL_USB_MSC.
///
/// The ARM7 answers on the same channel: the mount command with 1 when a host
/// was found and the microSD is exposed to it, 0 when there was no host; the
/// host query with USB_MSC_IPC_REPLY_HOST. The host query is asked once, at
/// launcher startup, and the answer stands for the rest of the session.
typedef enum
{
    USB_MSC_IPC_CMD_MOUNT = 0,
    USB_MSC_IPC_CMD_QUERY_HOST = 1
} UsbMscIpcCommand;

/// A reply to USB_MSC_IPC_CMD_QUERY_HOST: bit 8 marks it, bit 0 is the answer.
#define USB_MSC_IPC_REPLY_HOST(present) (0x100u | (unsigned int)((present) ? 1 : 0))
#define USB_MSC_IPC_IS_REPLY_HOST(data) (((data) & 0x100u) != 0)
#define USB_MSC_IPC_HOST_BIT(data)      ((data) & 1u)

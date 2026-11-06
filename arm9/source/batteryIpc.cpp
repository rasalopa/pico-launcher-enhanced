#include "common.h"
#include <libtwl/ipc/ipcFifoSystem.h>
#include "ipcChannels.h"
#include "batteryIpc.h"

/// @brief -1 until the ARM7 answers, then the raw state it sent.
static volatile s16 sBattery = -1;

static void ipcMessageHandler(u32 channel, u32 data, void* arg)
{
    sBattery = data & 0xFF;
}

void battery_init()
{
    ipc_setChannelHandler(IPC_CHANNEL_BATTERY, ipcMessageHandler, nullptr);
    ipc_sendFifoMessage(IPC_CHANNEL_BATTERY, 0);
}

bool battery_getState(unsigned int& level, bool& charging, bool& coarse)
{
    s16 battery = sBattery;
    if (battery < 0)
        return false;
    level = battery & IPC_BATTERY_LEVEL_MASK;
    charging = (battery & IPC_BATTERY_CHARGING) != 0;
    coarse = (battery & IPC_BATTERY_DS_MODE) != 0;
    return true;
}

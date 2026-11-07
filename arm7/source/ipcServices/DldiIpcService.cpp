#include "common.h"
#include <string.h>
#include <nds/disc_io.h>
#include "DldiIpcService.h"

extern FN_MEDIUM_STARTUP _DLDI_startup_ptr;
extern FN_MEDIUM_READSECTORS _DLDI_readSectors_ptr;
extern FN_MEDIUM_WRITESECTORS _DLDI_writeSectors_ptr;

void DldiIpcService::HandleMessage(u32 data)
{
    auto cmd = reinterpret_cast<const dldi_ipc_cmd_t*>(data << 2);
    switch (cmd->cmd)
    {
        case DLDI_IPC_CMD_SETUP:
            SetupDldi(cmd);
            break;

        case DLDI_IPC_CMD_READ_SECTORS:
            ReadSectors(cmd);
            break;

        case DLDI_IPC_CMD_WRITE_SECTORS:
            WriteSectors(cmd);
            break;
    }
}

void DldiIpcService::SetupDldi(const dldi_ipc_cmd_t* cmd) const
{
    memcpy((void*)0x037F8000, cmd->buffer, 16 * 1024);
    bool result = _DLDI_startup_ptr();
    SendResponseMessage(result);
}

void DldiIpcService::ReadSectors(const dldi_ipc_cmd_t* cmd) const
{
    rtos_lockMutex(&gCardMutex);
    _DLDI_readSectors_ptr(cmd->sector, cmd->count, cmd->buffer);
    rtos_unlockMutex(&gCardMutex);
    SendResponseMessage(0);
}

void DldiIpcService::WriteSectors(const dldi_ipc_cmd_t* cmd) const
{
    rtos_lockMutex(&gCardMutex);
    _DLDI_writeSectors_ptr(cmd->sector, cmd->count, cmd->buffer);
    rtos_unlockMutex(&gCardMutex);
    SendResponseMessage(0);
}

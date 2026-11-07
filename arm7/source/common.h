#pragma once
#include <nds/ndstypes.h>
#include <libtwl/rtos/rtosMutex.h>

/// @brief Serialises every access to the slot-1 cart bus.
///
/// The DLDI service serves the ARM9's sector reads and writes from one thread,
/// and the USB mass-storage transfer thread serves the PC's from another; both
/// put commands on the same bus. The DSpico platform layer takes this mutex for
/// every USB command it sends, so a transfer can never interleave with a DLDI
/// access. Defined in cardMutex.cpp.
extern rtos_mutex_t gCardMutex;

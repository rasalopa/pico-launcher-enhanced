#pragma once

#define IPC_CHANNEL_DSI_SD  16
#define IPC_CHANNEL_DLDI    17
#define IPC_CHANNEL_LOADER  18
#define IPC_CHANNEL_SOUND   19
#define IPC_CHANNEL_RTC     20
#define IPC_CHANNEL_PMIC    21
#define IPC_CHANNEL_BATTERY 22

// On IPC_CHANNEL_BATTERY the ARM7 pushes the battery state whenever it changes:
// a level 0..4 in the low bits, plus flags. The original DS and DS Lite only
// tell "low" from "not low", so they report level 1 or 4 with the DS_MODE flag
// set; the DSi's MCU gives all five levels and whether it is charging.
#define IPC_BATTERY_LEVEL_MASK  0x7
#define IPC_BATTERY_CHARGING    (1 << 3)
#define IPC_BATTERY_DS_MODE     (1 << 4)

// On IPC_CHANNEL_PMIC the ARM9 sends a backlight level (0..3), or this bit
// alone to ask whether the console has levels at all; the ARM7 answers 1 or 0
// on the same channel.
#define IPC_PMIC_ASK_LEVELS (1 << 7)

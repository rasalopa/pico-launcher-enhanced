#pragma once

/// @brief Registers the handler for the battery state the ARM7 pushes whenever
///        it changes, and asks the ARM7 to send it once now. Call once after
///        the ARM7 is up.
void battery_init();

/// @brief The latest battery state. Returns false until the ARM7 has sent one.
/// @param level 0 (empty) .. 4 (full). The original DS and DS Lite only tell
///        a low battery from a good one, so they report 1 or 4.
/// @param charging Whether the charger is connected; always false on a DS.
/// @param coarse Whether the console only reports low or not low.
bool battery_getState(unsigned int& level, bool& charging, bool& coarse);

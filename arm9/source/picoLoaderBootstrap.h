#pragma once
#include "picoLoader7.h"

pload_params_t* pload_getLoadParams();
void pload_setBootDrive(PicoLoaderBootDrive bootDrive);
void pload_setLauncherPath(const char* launcherPath);
void pload_setCheatData(const pload_cheats_t* cheatData);

/// @brief Asks Pico Loader Enhanced to count the frames the next game runs.
void pload_setCountPlayTime(bool countPlayTime);

/// @brief Takes the frame count the loader passes after an in-game exit.
void pload_setPlayFramesArgument(const char* argument);

/// @brief How long the last game ran, or 0 when the loader passed no frames.
u32 pload_getPlaySeconds();

void pload_start();

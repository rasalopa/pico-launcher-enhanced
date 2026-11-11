#pragma once
#include "common.h"

/// @brief Builds the title shown for a game that has none in its header, from
///        its file name: the extension and the trailing groups of a no-intro,
///        GoodTools or romhack name are taken off, and an article that no-intro
///        appends to the end is put back in front.
/// @param title Buffer the title is copied to.
/// @param titleLength The size of the buffer, in characters.
/// @param fileName The game's file name.
void BuildRomTitle(char16_t* title, u32 titleLength, const char* fileName);

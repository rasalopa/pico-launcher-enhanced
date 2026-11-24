#pragma once
#include "common.h"

/// @brief Looks up the company a two character publisher code stands for. The GBA maker
///        code, the Game Boy new licensee code and the SNES expanded maker code use this
///        format. Case is ignored.
/// @param code The two characters as they appear in the rom header.
/// @return The company name, or \c nullptr when the code is not known.
const char* GetRomPublisherName(const char* code);

/// @brief Looks up the company a Game Boy new licensee code stands for. A few codes were
///        reassigned between the monochrome and color eras.
/// @param code The two characters as they appear in the rom header.
/// @param colorGameBoy Whether the cartridge is a Game Boy Color cartridge.
/// @return The company name, or \c nullptr when the code is not known.
const char* GetGameBoyPublisherName(const char* code, bool colorGameBoy);

/// @brief Looks up the company a SNES expanded maker code stands for. A few codes were
///        reassigned after the SNES era, so they are named as they were then.
/// @param code The two characters as they appear in the rom header.
/// @return The company name, or \c nullptr when the code is not known.
const char* GetSnesPublisherName(const char* code);

/// @brief Looks up the company an old one byte licensee code stands for. The Game Boy old
///        licensee code, the NES Nintendo header and the SNES developer ID use it.
/// @param code The byte from the header.
/// @return The company name, or \c nullptr when the code is not known.
const char* GetLegacyPublisherName(u8 code);

/// @brief Copies a publisher name into a UTF-16 buffer, truncating it if needed.
/// @param dest The destination buffer.
/// @param destSize The size of the destination buffer, in characters.
/// @param name The name to copy, or \c nullptr to write an empty string.
void CopyPublisherName(char16_t* dest, u32 destSize, const char* name);

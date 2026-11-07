#pragma once

/// @brief Asks the ARM7 to put the microSD on the USB bus, and waits for its
///        answer.
///
/// This blocks while the ARM7 waits for a host to enumerate the device, which
/// is a fraction of a second when the cable is plugged in and at most half a
/// second when it is not.
///
/// @return true when a host was found and the card is now served to it.
bool UsbMscMountMicroSd();

/// @brief Asks the ARM7 whether a USB host is on the other end of the cable
///        and caches the answer.
///
/// Called once, from the launcher's startup path: the cable belongs in the
/// flashcart before the console boots, so one question is all this needs. One
/// IPC round trip, not a wait for enumeration - the ARM7 armed the device
/// stack at boot, so by now it already knows.
///
/// @return true when a host is plugged in.
bool UsbMscQueryHostPresent();

/// @brief The cached answer to UsbMscQueryHostPresent(), without asking. The
///        menu reads this; nothing updates it after startup.
bool UsbMscHostPresent();

/// @brief Whether the microSD is currently served to a USB host. The launcher
///        refuses to launch anything while it is, since the PC owns the
///        filesystem until the console is powered off or leaves the launcher.
bool UsbMscIsMounted();

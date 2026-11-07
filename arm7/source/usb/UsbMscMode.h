#pragma once

enum class UsbMscMountResult
{
    Mounted,
    NoHost,
};

/// @brief Puts the USB device stack on the bus, if it is not on it yet.
///
/// Called from the boot path. The DSpico has no way to report a cable on its
/// own, so the only way to know whether a host is plugged in is to be a device
/// on the bus and watch for the host to enumerate it. The stack stays armed
/// for the rest of the session and serves no media until UsbMscMount() is
/// asked for it, so an idle console shows no drive on the PC.
void UsbMscStart();

/// @brief Whether a USB host is on the other end of the cable.
///
/// False until UsbMscStart() ran. The launcher asks this once, at startup, and
/// keeps that answer: the cable is meant to be plugged in before the cart is
/// booted, so there is nothing to watch for afterwards.
bool UsbMscIsHostPresent();

/// @brief Starts the USB device stack, if it is not running yet, and waits for
///        a host to enumerate it.
///
/// Once mounted, the microSD stays exposed for the rest of the session: the
/// launcher does not unmount it, by design (see docs/Enhanced.md).
///
/// @return Mounted when a host is there and the card can be served, NoHost
///         otherwise.
UsbMscMountResult UsbMscMount();

/// @brief Whether the microSD is currently exposed to a host.
bool UsbMscIsMounted();

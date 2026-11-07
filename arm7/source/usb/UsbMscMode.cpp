#include "common.h"
#include <string.h>
#include <nds/disc_io.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb.h"
#include "usb_descriptors.h"
#include "UsbMscMode.h"

extern FN_MEDIUM_READSECTORS _DLDI_readSectors_ptr;
extern FN_MEDIUM_WRITESECTORS _DLDI_writeSectors_ptr;

/// @brief Set once tusb_init() ran. The device stack and its thread outlive
///        every mount attempt, and outlive the launcher's browsing too: the
///        stack is armed at boot so the menu can tell whether a cable is
///        plugged in, which is the one question it asks about the cable.
static bool sUsbStarted = false;
/// @brief Whether the card is served to the host. Only true after a mount
///        succeeded, which is what keeps the launcher's own browsing safe
///        until the user asks for the card reader.
static volatile bool sMediaPresent = false;
static u32 sBlockCount = 0;

static rtos_thread_t sUsbThread;
static u32 sUsbThreadStack[512];

static u8 sSector0Buffer[512] alignas(4);

/// @brief Reads the SD's capacity the way mass-storage.nds does.
///
/// Based on https://github.com/asiekierka/nrio-usb-disk/blob/main/source/msc.c
/// msc_find_block_count by Asie. The DSpico firmware does not report the real
/// SD capacity, so the partition table (or the FAT boot sector) is the only
/// source. It can be wrong for exotic layouts, which is what the example says
/// too; the host copes with a size that is merely different from the card's.
static u32 findSdCardBlockCount()
{
    rtos_lockMutex(&gCardMutex);
    _DLDI_readSectors_ptr(0, 1, sSector0Buffer);
    rtos_unlockMutex(&gCardMutex);

    u16 footer = *(u16*)(sSector0Buffer + 510);
    if (footer == 0xAA55)
    {
        u8 bootOpcode = sSector0Buffer[0];
        if (bootOpcode == 0xEB || bootOpcode == 0xE9 || bootOpcode == 0xE8)
        {
            if (!memcmp(sSector0Buffer + 54, "FAT", 3) || !memcmp(sSector0Buffer + 82, "FAT32   ", 8))
            {
                u32 totalSectors = *(u32*)(sSector0Buffer + 32);
                if (totalSectors < 0x10000)
                {
                    totalSectors = sSector0Buffer[19] | (sSector0Buffer[20] << 8);
                }
                return totalSectors;
            }
        }

        u32 blockCount = 0;
        for (u32 tableEntry = 0x1BE; tableEntry < 0x1FE; tableEntry += 16)
        {
            u32 pStart = *(u16*)(sSector0Buffer + tableEntry + 8) | (*(u16*)(sSector0Buffer + tableEntry + 10) << 16);
            u32 pCount = *(u16*)(sSector0Buffer + tableEntry + 12) | (*(u16*)(sSector0Buffer + tableEntry + 14) << 16);
            u32 pEnd = pStart + pCount;
            if (pEnd > blockCount)
            {
                blockCount = pEnd;
            }
        }

        return blockCount;
    }

    return 0;
}

static void usbThreadMain(void* arg)
{
    (void)arg;
    // A timeout, not tud_task(): the blocking form parks on the device stack's
    // queue instead of spinning, so the ARM7 keeps its cycles for sound and
    // DLDI while the launcher sits in the card reader.
    while (true)
    {
        tud_task_ext(10, false);
    }
}

void UsbMscStart()
{
    if (sUsbStarted)
        return;

    tusb_rhport_init_t devInit =
    {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    if (!tusb_init(0, &devInit))
        return;

    rtos_createThread(&sUsbThread, 3, usbThreadMain, nullptr, sUsbThreadStack, sizeof(sUsbThreadStack));
    rtos_wakeupThread(&sUsbThread);
    sUsbStarted = true;
}

bool UsbMscIsHostPresent()
{
    return sUsbStarted && tud_mounted();
}

UsbMscMountResult UsbMscMount()
{
    UsbMscStart();
    if (!sUsbStarted)
        return UsbMscMountResult::NoHost;

    // A host that is already on the other end of the cable resets the bus
    // within milliseconds of the pull-up going on. Half a second is far more
    // than enumeration needs and keeps the freeze of the sheets that ask for
    // this short when the cable is not there. When the stack was armed at boot
    // and the host was there all along, this loop does not run at all.
    for (int i = 0; i < 100 && !tud_mounted(); i++)
    {
        tud_task_ext(5, false);
    }

    if (!tud_mounted())
        return UsbMscMountResult::NoHost;

    sBlockCount = findSdCardBlockCount();
    sMediaPresent = sBlockCount != 0;
    return UsbMscMountResult::Mounted;
}

bool UsbMscIsMounted()
{
    return sMediaPresent;
}

//--------------------------------------------------------------------+
// TinyUSB mass storage callbacks
//--------------------------------------------------------------------+

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void)lun;

    static const char kVendor[] = "LNH";
    static const char kProduct[] = "DSpico microSD";
    static const char kRevision[] = "1.0";

    memcpy(vendor_id, kVendor, sizeof(kVendor) - 1);
    memcpy(product_id, kProduct, sizeof(kProduct) - 1);
    memcpy(product_rev, kRevision, sizeof(kRevision) - 1);
}

// The card only appears once the user asked for the reader: until then the
// device enumerates with no medium, so Windows assigns no drive letter.
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void)lun;
    return sMediaPresent;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    (void)lun;

    *block_count = sBlockCount;
    *block_size = 512;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void)lun;
    (void)power_condition;
    (void)start;
    (void)load_eject;
    // Eject and load are left alone on purpose: the launcher does not unmount
    // the card mid-session, so the host's eject is acknowledged and nothing
    // else. The safe way out is the one the sheet spells out.
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    (void)lun;

    if (!sMediaPresent || offset != 0 || (bufsize % 512) != 0)
        return -1;

    rtos_lockMutex(&gCardMutex);
    _DLDI_readSectors_ptr(lba, bufsize / 512, buffer);
    rtos_unlockMutex(&gCardMutex);

    return (int32_t)bufsize;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void)lun;
    return true;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    (void)lun;

    if (!sMediaPresent || offset != 0 || (bufsize % 512) != 0)
        return -1;

    rtos_lockMutex(&gCardMutex);
    _DLDI_writeSectors_ptr(lba, bufsize / 512, buffer);
    rtos_unlockMutex(&gCardMutex);

    return (int32_t)bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    (void)lun;
    (void)buffer;
    (void)bufsize;

    // Everything this reader supports has its own callback above; the rest is
    // refused as an invalid operation, the way the example does.
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}

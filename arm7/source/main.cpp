#include "common.h"
#include <nds/system.h>
#include <nds/bios.h>
#include <libtwl/sound/sound.h>
#include <libtwl/sound/soundChannel.h>
#include <libtwl/sound/soundCapture.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/timer/timer.h>
#include <libtwl/sound/sound.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/sys/sysPower.h>
#include <libtwl/sio/sioRtc.h>
#include <libtwl/sio/sio.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/mem/memSwap.h>
#include <libtwl/i2c/i2cMcu.h>
#include <libtwl/spi/spiPmic.h>
#include "logger/PlainLogger.h"
#include "logger/NocashOutputStream.h"
#include "logger/NullLogger.h"
#include "logger/ThreadSafeLogger.h"
#include "picoLoaderBootstrap.h"
#include "sharedMemory.h"
#include "ipcChannels.h"
#include "ipcServices/DsiSdIpcService.h"
#include "ipcServices/DldiIpcService.h"
#include "ipcServices/SoundIpcService.h"
#include "ipcServices/RtcIpcService.h"
#include "ExitMode.h"
#include "Arm7State.h"
#include "mmc/tmio.h"
#include "touchScreen.h"

static NocashOutputStream sNocashOutputStream;
static PlainLogger sPlainLogger = PlainLogger(LogLevel::All, std::unique_ptr<IOutputStream>(&sNocashOutputStream));
static ThreadSafeLogger sThreadSafeLogger = ThreadSafeLogger(std::unique_ptr<ILogger>(&sPlainLogger));

static DsiSdIpcService sDsiSdIpcService;
static DldiIpcService sDldiIpcService;
static SoundIpcService sSoundIpcService;
static RtcIpcService sRtcIpcService;

ILogger* gLogger = &sThreadSafeLogger;

static rtos_event_t sVCountEvent;
static ExitMode sExitMode;
static Arm7State sState;
static volatile u8 sMcuIrqFlag = false;
/// @brief Requested backlight level + 1, 0 when nothing is pending. Written
///        from the IPC handler, consumed on the main thread: the PMIC shares
///        the SPI bus with the touch screen, so all SPI stays on one thread.
static volatile u8 sPendingBacklight = 0;

// The custom ARM7 binary does not use libnds' default PM loop, so closing
// the DS/DSi lid must be handled here. RCNT0_H bit 7 is the hinge sensor:
// 0 = open, 1 = closed. We debounce it for a few VCount frames before
// entering BIOS sleep. swiSleep() wakes when the lid is opened again.
static u8 sLidClosedFrames = 0;
static bool sLidWasClosed = false;

// BIOS sleep turns the DS Lite backlight off. Keep the level that was active
// before sleeping so it can be restored immediately after the lid wakes the
// console. On the original DS this register mirrors the control register, so
// it is only written when the DS Lite signature is present.
static u8 sSleepBacklightLevel = 0;
static bool sSleepBacklightValid = false;

static void restoreBacklightAfterSleep()
{
    // BIOS sleep does not restore the DS Lite PMIC display state for us.
    // Explicitly restore the LED and both backlight enable bits after wake.
    pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_NONE);
    pmic_setTopBacklightEnable(true);
    pmic_setBottomBacklightEnable(true);

    if (sSleepBacklightValid)
    {
        const u8 backlight = pmic_readRegister(PMIC_REG_BACKLIGHT);
        if ((backlight & 0xF0) == 0x40)
        {
            pmic_writeRegister(
                PMIC_REG_BACKLIGHT,
                (backlight & ~PMIC_BACKLIGHT_MASK) | (sSleepBacklightLevel & PMIC_BACKLIGHT_MASK)
            );
        }
    }

    sSleepBacklightValid = false;
}

// The RTOS IRQ table uses bit 22 for the ARM7 hinge/PMIC interrupt.
// On this project the default IRQ mask has it disabled, so BIOS sleep
// would have no enabled wake source when the lid is opened. Keep a
// dedicated handler installed and enable the interrupt explicitly.
static void lidIrq(u32 irqMask)
{
    (void)irqMask;
}

static void checkLidSleep()
{
    const bool lidClosed = (REG_RCNT0_H & RCNT0_H_DATA_LID) != 0;

    if (!lidClosed)
    {
        sLidClosedFrames = 0;

        // If swiSleep() returned because the lid was opened, restore the
        // backlight before the next frame is processed.
        if (sLidWasClosed)
        {
            sLidWasClosed = false;
            restoreBacklightAfterSleep();
        }

        return;
    }

    if (sLidClosedFrames < 8)
        sLidClosedFrames++;

    if (!sLidWasClosed && sLidClosedFrames >= 4)
    {
        sLidWasClosed = true;

        // Capture the current DS Lite backlight level before BIOS sleep
        // powers the display down.
        const u8 backlight = pmic_readRegister(PMIC_REG_BACKLIGHT);
        if ((backlight & 0xF0) == 0x40)
        {
            sSleepBacklightLevel = backlight & PMIC_BACKLIGHT_MASK;
            sSleepBacklightValid = true;
        }

        // Enter the same low-power display state used by normal DS Lite
        // lid sleep: disable both LCD backlights and make the power LED
        // blink slowly. The hinge IRQ must remain enabled because BIOS
        // sleep wakes the ARM7 from the lid-open interrupt.
        pmic_setTopBacklightEnable(false);
        pmic_setBottomBacklightEnable(false);
        pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_SLOW);

        rtos_disableIrqMask(RTOS_IRQ_VCOUNT);
        swiSleep();
        rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
    }
}

static void vcountIrq(u32 irqMask)
{
    rtos_signalEvent(&sVCountEvent);
}

static void backlightIpcHandler(u32 channel, u32 data, void* arg)
{
    sPendingBacklight = (data & PMIC_BACKLIGHT_MASK) + 1;
}

static void applyPendingBacklight()
{
    u8 pending = mem_swapByte(0, &sPendingBacklight);
    if (pending != 0)
    {
        u8 backlight = pmic_readRegister(PMIC_REG_BACKLIGHT);
        // DS Lite only, where bits 4-7 of the backlight register read back
        // as 4. On the original DS registers 4..7F are MIRRORS of 0..3, so
        // this read actually hit the control register — writing it back
        // with modified low bits would clobber the sound amplifier there.
        if ((backlight & 0xF0) == 0x40)
        {
            pmic_writeRegister(PMIC_REG_BACKLIGHT,
                (backlight & ~PMIC_BACKLIGHT_MASK) | (pending - 1));
        }
    }
}

static void mcuIrq(u32 irq2Mask)
{
    sMcuIrqFlag = true;
}

static void checkMcuIrq(void)
{
    // mcu only exists in DSi mode
    if (isDSiMode())
    {
        // check and ack the flag atomically
        if (mem_swapByte(false, &sMcuIrqFlag))
        {
            // check the irq mask
            u32 irqMask = mcu_getIrqMask();
            if (irqMask & MCU_IRQ_RESET)
            {
                // power button was released
                sExitMode = ExitMode::Reset;
                sState = Arm7State::ExitRequested;
            }
            else if (irqMask & MCU_IRQ_POWER_OFF)
            {
                // power button was held long to trigger a power off
                sExitMode = ExitMode::PowerOff;
                sState = Arm7State::ExitRequested;
            }
        }
    }
}

static void initializeVCountIrq()
{
    rtos_createEvent(&sVCountEvent);
    gfx_setVCountMatchLine(96);
    rtos_setIrqFunc(RTOS_IRQ_VCOUNT, vcountIrq);
    rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
    gfx_setVCountMatchIrqEnabled(true);
}

static void clearSoundRegisters()
{
    REG_SOUNDCNT = 0;
    REG_SNDCAP0CNT = 0;
    REG_SNDCAP1CNT = 0;

    for (int i = 0; i < 16; i++)
    {
        REG_SOUNDxCNT(i) = 0;
        REG_SOUNDxSAD(i) = 0;
        REG_SOUNDxTMR(i) = 0;
        REG_SOUNDxPNT(i) = 0;
        REG_SOUNDxLEN(i) = 0;
    }
}

static void initializeArm7()
{
    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();

    clearSoundRegisters();

    pmic_setAmplifierEnable(true);
    sys_setSoundPower(true);

    readUserSettings();
    pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_NONE);

    sio_setGpioSiIrq(false);
    sio_setGpioMode(RCNT0_L_MODE_GPIO);

    rtc_init();

    if (isDSiMode())
    {
        TMIO_init();
        sDsiSdIpcService.Start();
    }

    sDldiIpcService.Start();
    pload_init();

    snd_setMasterVolume(127);
    snd_setMasterEnable(true);
    sSoundIpcService.Start();
    sRtcIpcService.Start();
    ipc_setChannelHandler(IPC_CHANNEL_PMIC, backlightIpcHandler, nullptr);

    initializeVCountIrq();

    // RTOS_IRQ_PMIC (bit 22) is the ARM7 hinge interrupt on NDS.
    // Enable it explicitly so BIOS swiSleep() can wake when the lid opens.
    rtos_setIrqFunc(RTOS_IRQ_PMIC, lidIrq);
    rtos_enableIrqMask(RTOS_IRQ_PMIC);

    if (isDSiMode())
    {
        rtos_setIrq2Func(RTOS_IRQ2_MCU, mcuIrq);
        rtos_enableIrq2Mask(RTOS_IRQ2_MCU);
    }

    touch_init();

    ipc_setArm7SyncBits(7);
}

static void updateArm7IdleState()
{
    if (pload_shouldStart())
    {
        sExitMode = ExitMode::PicoLoader;
        sState = Arm7State::ExitRequested;
    }
    else
    {
        checkMcuIrq();
    }

    if (sState == Arm7State::ExitRequested)
    {
        snd_setMasterVolume(0); // mute sound
    }
}

static bool performExit(ExitMode exitMode)
{
    switch (exitMode)
    {
        case ExitMode::Reset:
        {
            mcu_setWarmBootFlag(true);
            mcu_hardReset();
            break;
        }
        case ExitMode::PowerOff:
        {
            pmic_shutdown();
            break;
        }
        case ExitMode::PicoLoader:
        {
            pload_start();
            break;
        }
    }

    while (true); // wait infinitely for exit
}

static void updateArm7ExitRequestedState()
{
    performExit(sExitMode);
}

static void updateArm7()
{
    switch (sState)
    {
        case Arm7State::Idle:
        {
            updateArm7IdleState();
            break;
        }
        case Arm7State::ExitRequested:
        {
            updateArm7ExitRequestedState();
            break;
        }
    }
}

int main()
{
    sState = Arm7State::Idle;
    initializeArm7();

    while (true)
    {
        rtos_waitEvent(&sVCountEvent, true, true);
        u16 keys = REG_RCNT0_H | RCNT0_H_DATA_PEN;
        touchPosition touchPos;
        if (touch_update(touchPos))
        {
            keys &= ~RCNT0_H_DATA_PEN; // pen down
            SHARED_TOUCH_X = touchPos.px;
            SHARED_TOUCH_Y = touchPos.py;
        }
        SHARED_KEY_XY = keys;
        applyPendingBacklight();
        checkLidSleep();
        updateArm7();
    }

    return 0;
}

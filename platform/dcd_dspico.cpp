#include "common.h"
#include <libtwl/card/card.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb_option.h"
#include "tusb.h"

#if CFG_TUD_ENABLED && CFG_TUSB_MCU == OPT_MCU_DSPICO

#include "device/dcd.h"
#include "DSPicoUsb.h"
#include "DSPicoUsbInEndpoint.h"
#include "DSPicoUsbOutEndpoint.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

static rtos_event_t sCardIrqEvent;
static rtos_thread_t sUsbThread;
static u32 sUsbThreadStack[512];

static DSPicoUsbInEndpoint sInEndpoints[16] =
{
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 0),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 1),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 2),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 3),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 4),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 5),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 6),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 7),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 8),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 9),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 10),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 11),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 12),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 13),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 14),
    DSPicoUsbInEndpoint(TUSB_DIR_IN_MASK | 15)
};

static DSPicoUsbOutEndpoint sOutEndpoints[16] =
{
    DSPicoUsbOutEndpoint(0),
    DSPicoUsbOutEndpoint(1),
    DSPicoUsbOutEndpoint(2),
    DSPicoUsbOutEndpoint(3),
    DSPicoUsbOutEndpoint(4),
    DSPicoUsbOutEndpoint(5),
    DSPicoUsbOutEndpoint(6),
    DSPicoUsbOutEndpoint(7),
    DSPicoUsbOutEndpoint(8),
    DSPicoUsbOutEndpoint(9),
    DSPicoUsbOutEndpoint(10),
    DSPicoUsbOutEndpoint(11),
    DSPicoUsbOutEndpoint(12),
    DSPicoUsbOutEndpoint(13),
    DSPicoUsbOutEndpoint(14),
    DSPicoUsbOutEndpoint(15)
};

static void sendCommand(u64 command)
{
    rtos_lockMutex(&gCardMutex);
    card_romSetCmd(command);
    card_romStartXfer(DSPICO_USB_DEFAULT_COMMAND_SETTINGS, false);
    card_romWaitBusy();
    rtos_unlockMutex(&gCardMutex);
}

static void cardIrq(u32 irqMask)
{
    rtos_signalEvent(&sCardIrqEvent);
}

static void usbThreadMain(void* arg)
{
    while (true)
    {
        rtos_waitEvent(&sCardIrqEvent, false, true);

        u32 event;
        bool lastEvent;
        do
        {
            rtos_lockMutex(&gCardMutex);
            card_romSetCmd(DSPICO_CMD_USB_GET_EVENT);
            card_romStartXfer(MCCNT1_DIR_READ | MCCNT1_RESET_OFF | MCCNT1_CLK_6_7_MHZ | MCCNT1_LEN_4 | MCCNT1_CMD_SCRAMBLE |
                MCCNT1_LATENCY2(4) | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY1(0), false);
            card_romCpuRead(&event, 1);
            rtos_unlockMutex(&gCardMutex);
            lastEvent = (event >> 31) != 0;
            event &= ~(1 << 31);
            if (event == 0)
            {
                // USB_EVENT_NONE
                break;
            }
            else if ((event >> 30) == 1)
            {
                // USB_EVENT_SETUP_RECEIVED
                tusb_control_request_t setup;
                setup.wLength = (event >> 16) & 0x1FFF;
                u32 direction = (event >> 29) & 1;
                setup.wIndex = event & 0xFFFF;

                rtos_lockMutex(&gCardMutex);
                card_romSetCmd(DSPICO_CMD_USB_GET_EVENT);
                card_romStartXfer(MCCNT1_DIR_READ | MCCNT1_RESET_OFF | MCCNT1_CLK_6_7_MHZ | MCCNT1_LEN_4 | MCCNT1_CMD_SCRAMBLE |
                    MCCNT1_LATENCY2(4) | MCCNT1_CLOCK_SCRAMBLER | MCCNT1_READ_DATA_DESCRAMBLE | MCCNT1_LATENCY1(0), false);
                card_romCpuRead(&event, 1);
                rtos_unlockMutex(&gCardMutex);
                lastEvent = (event >> 31) != 0;
                event &= ~(1 << 31);
                setup.wValue = event & 0xFFFF;
                setup.bRequest = (event >> 16) & 0xFF;
                setup.bmRequestType = (event >> 24) & 0x7F;
                setup.bmRequestType_bit.direction = direction;
                dcd_event_setup_received(0, (const u8*)&setup, true);
            }
            else if ((event >> 28) == 2)
            {
                // USB_EVENT_SOF
                dcd_event_sof(0, event & 0x7FF, true);
            }
            else if ((event >> 28) == 3)
            {
                // USB_EVENT_XFER_COMPLETE
                u32 endpoint = event & 0xFF;
                u32 transferredBytes = (event >> 8) & 0x1FFF;
                if (tu_edpt_dir(endpoint) == TUSB_DIR_IN)
                {
                    sInEndpoints[endpoint & ~0x80].ProcessTransferCompleteEvent(transferredBytes);
                }
                else
                {
                    sOutEndpoints[endpoint & ~0x80].ProcessTransferCompleteEvent(transferredBytes);
                }
            }
            else if (event == 1)
            {
                // USB_EVENT_BUS_RESET
                dcd_event_bus_reset(0, TUSB_SPEED_FULL, true);
            }
            else if (event == 2)
            {
                // USB_EVENT_UNPLUGGED
                dcd_event_bus_signal(0, DCD_EVENT_UNPLUGGED, true);
            }
            else if (event == 3)
            {
                // USB_EVENT_SUSPEND
                dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
            }
            else if (event == 4)
            {
                // USB_EVENT_RESUME
                dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
            }
            else
            {
                // invalid
            }
        } while (!lastEvent);
    }
}

/*------------------------------------------------------------------*/
/* Device API
 *------------------------------------------------------------------*/

// Initialize controller to device mode
bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init)
{
    (void) rhport;
    (void) rh_init;
    rtos_createEvent(&sCardIrqEvent);
    rtos_disableIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
    rtos_setIrqFunc(RTOS_IRQ_DS_SLOTA_IREQ, cardIrq);
    rtos_ackIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
    rtos_createThread(&sUsbThread, 2, usbThreadMain, NULL, sUsbThreadStack, sizeof(sUsbThreadStack));
    rtos_wakeupThread(&sUsbThread);
    sendCommand(DSPICO_CMD_USB_COMMAND_INIT);
    return true;
}

bool dcd_deinit(uint8_t rhport)
{
    (void) rhport;
    rtos_disableIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
    rtos_setIrqFunc(RTOS_IRQ_DS_SLOTA_IREQ, NULL);
    rtos_ackIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
    sendCommand(DSPICO_CMD_USB_COMMAND_DEINIT);
    return true;
}

// Enable device interrupt
void dcd_int_enable(uint8_t rhport)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_INTERRUPT_ENABLE);
    rtos_enableIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
}

// Disable device interrupt
void dcd_int_disable(uint8_t rhport)
{
    (void) rhport;
    rtos_disableIrqMask(RTOS_IRQ_DS_SLOTA_IREQ);
    sendCommand(DSPICO_CMD_USB_COMMAND_INTERRUPT_DISABLE);
}

// Receive Set Address request, mcu port must also include status IN response
void dcd_set_address(uint8_t rhport, uint8_t dev_addr)
{
    (void) rhport;
    (void) dev_addr;
    sendCommand(DSPICO_CMD_USB_COMMAND_BEGIN_SET_ADDRESS);
}

// Wake up host
void dcd_remote_wakeup(uint8_t rhport)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_REMOTE_WAKEUP);
}

// Connect by enabling internal pull-up resistor on D+/D-
void dcd_connect(uint8_t rhport)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_CONNECT);
}

// Disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_DISCONNECT);
}

void dcd_sof_enable(uint8_t rhport, bool en)
{
    (void) rhport;
    sendCommand(en ? DSPICO_CMD_USB_COMMAND_SOF_ENABLE : DSPICO_CMD_USB_COMMAND_SOF_DISABLE);
}

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const* request)
{
    (void) rhport;
    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
        request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD &&
        request->bRequest == TUSB_REQ_SET_ADDRESS)
    {
        sendCommand(DSPICO_CMD_USB_COMMAND_FINISH_SET_ADDRESS((u8)request->wValue));
    }
}

// Configure endpoint's registers according to descriptor
bool dcd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const * ep_desc)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_EP_OPEN(
        ep_desc->bEndpointAddress, ep_desc->wMaxPacketSize, ep_desc->bmAttributes.xfer));
    return true;
}

void dcd_edpt_close_all(uint8_t rhport)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_EP_CLOSE_ALL);
}

// Submit a transfer, When complete dcd_event_xfer_complete() is invoked to notify the stack
bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes)
{
    (void) rhport;
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
        sInEndpoints[ep_addr & ~0x80].BeginTransfer(buffer, total_bytes);
    }
    else
    {
        sOutEndpoints[ep_addr & ~0x80].BeginTransfer(buffer, total_bytes);
    }
    return true;
}

// Stall endpoint
void dcd_edpt_stall(uint8_t rhport, uint8_t ep_addr)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_EP_STALL(ep_addr));
}

// clear stall, data toggle is also reset to DATA0
void dcd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_EP_CLEAR_STALL(ep_addr));
}

void dcd_edpt_close(uint8_t rhport, uint8_t ep_addr)
{
    (void) rhport;
    sendCommand(DSPICO_CMD_USB_COMMAND_EP_CLOSE(ep_addr));
}

#endif

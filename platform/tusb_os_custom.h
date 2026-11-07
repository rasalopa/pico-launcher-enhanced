#pragma once
#include "common.h"
#include "libtwl/rtos/rtosMutex.h"
#include "libtwl/rtos/rtosEvent.h"

typedef rtos_event_t osal_semaphore_def_t;
typedef rtos_event_t* osal_semaphore_t;

static inline osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef)
{
    rtos_createEvent(semdef);
    return semdef;
}

static inline bool osal_semaphore_delete(osal_semaphore_t semd_hdl)
{
    (void) semd_hdl;
    return true;
}

static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr)
{
    rtos_signalEvent(sem_hdl);
    return true;
}

static inline bool osal_semaphore_wait(osal_semaphore_t sem_hdl, uint32_t msec)
{
    (void) msec;
    rtos_waitEvent(sem_hdl, false, true);
    return true;
}

static inline void osal_semaphore_reset(osal_semaphore_t sem_hdl)
{
    rtos_clearEvent(sem_hdl);
}

typedef rtos_mutex_t osal_mutex_def_t;
typedef rtos_mutex_t* osal_mutex_t;

static inline osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef)
{
    rtos_createMutex(mdef);
    return mdef;
}

static inline bool osal_mutex_delete(osal_mutex_t mutex_hdl)
{
    (void) mutex_hdl;
    return true;
}

static inline bool osal_mutex_lock(osal_mutex_t mutex_hdl, uint32_t msec)
{
    if (msec == OSAL_TIMEOUT_NOTIMEOUT)
    {
        return rtos_tryLockMutex(mutex_hdl);
    }
    else
    {
        rtos_lockMutex(mutex_hdl);
        return true;
    }
}

static inline bool osal_mutex_unlock(osal_mutex_t mutex_hdl)
{
    rtos_unlockMutex(mutex_hdl);
    return true;
}

typedef struct
{
    rtos_event_t event;
    volatile uint32_t readPtr;
    volatile uint32_t writePtr;
    uint32_t elementSize;
    uint32_t bufferSize;
    uint8_t* buffer;
} osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

#define OSAL_QUEUE_DEF(_int_set, _name, _depth, _type)    \
    uint8_t _name##_buf[(_depth + 1)*sizeof(_type)];      \
    osal_queue_def_t _name = {                            \
        .readPtr = 0,                                     \
        .writePtr = 0,                                    \
        .elementSize = sizeof(_type),                     \
        .bufferSize = (_depth + 1)*sizeof(_type),         \
        .buffer = _name##_buf                             \
    }

static inline osal_queue_t osal_queue_create(osal_queue_def_t* qdef)
{
    rtos_createEvent(&qdef->event);
    qdef->readPtr = 0;
    qdef->writePtr = 0;
    return qdef;
}

static inline bool osal_queue_delete(osal_queue_t qhdl)
{
    (void) qhdl;
    return true;
}

static inline bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec)
{
    do
    {
        uint32_t readPtr = qhdl->readPtr;
        uint32_t writePtr = qhdl->writePtr;
        if (readPtr != writePtr)
        {
            memcpy(data, &qhdl->buffer[readPtr], qhdl->elementSize);
            readPtr += qhdl->elementSize;
            if (readPtr == qhdl->bufferSize)
            {
                readPtr = 0;
            }
            qhdl->readPtr = readPtr;
            return true;
        }

        if (msec == OSAL_TIMEOUT_NOTIMEOUT)
        {
            return false;
        }

        rtos_waitEvent(&qhdl->event, false, true);
    } while (true);
}

static inline bool osal_queue_send(osal_queue_t qhdl, const void* data, bool in_isr)
{
    uint32_t readPtr = qhdl->readPtr;
    uint32_t writePtr = qhdl->writePtr;
    uint32_t newWritePtr = writePtr + qhdl->elementSize;
    if (newWritePtr == qhdl->bufferSize)
    {
        newWritePtr = 0;
    }
    if (newWritePtr == readPtr)
    {
        return false;
    }

    memcpy(&qhdl->buffer[writePtr], data, qhdl->elementSize);
    qhdl->writePtr = newWritePtr;
    rtos_signalEvent(&qhdl->event);
    return true;
}

static inline bool osal_queue_empty(osal_queue_t qhdl)
{
    qhdl->readPtr = 0;
    qhdl->writePtr = 0;
    return true;
}

static inline void osal_task_delay(uint32_t msec)
{
    // Not implemented
}

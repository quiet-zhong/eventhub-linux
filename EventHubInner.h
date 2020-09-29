#ifndef EVENTHUBINNER_H
#define EVENTHUBINNER_H

#include "EventHub.h"
#include "libunixsocket.h"
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define strBindFileName "/tmp/.eventhub.sock"

enum TRANS_TYPE
{
    TRANS_SUB_CREAT,
    TRANS_SUB_EVENT,
    TRANS_UNSUB_EVENT,
};

typedef struct _TransPack
{
    HL_S32          trans_type;
    HL_EVENT_S      event;
}TransPack;

typedef struct _ThreadFuncArg
{
    HL_EVENT_S              event;
    HL_SUBSCRIBER_ATTR_S   param;
}ThreadFuncArg;

HL_VOID LOCK();

HL_VOID UNLOCK();

HL_S32 init();

HL_S32 deinit();

HL_S32 event_id_reg(HL_EVENT_ID EventID);

HL_S32 event_id_unreg(HL_EVENT_ID EventID);

HL_S32 event_pub(HL_EVENT_S *pEvent);

HL_BOOL enable();

HL_VOID setEnable(const HL_BOOL enable);

HL_S32 send_data(HL_S32 sock, TransPack pack);

HL_BOOL isInit();

HL_S32 get_event_history(HL_EVENT_ID EventID, HL_EVENT_S *pEvent);

#ifdef __cplusplus
}
#endif

#endif // EVENTHUBINNER_H

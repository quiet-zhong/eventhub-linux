#include "EventHubInner.h"
#include "EventHub.h"
#include "debug.h"

#include <string.h>
#include <unistd.h>

//检测空指针
#define CHECK_NULL_PTR(p) \
    do{         \
    if(p == NULL) \
{ return HL_ERR_EVTHUB_NULL_PTR; } \
    }while(0)

//检测是否初始化
#define CHECK_EVENTHUB_INIT() \
    do{         \
    if(isInit() == HL_FALSE) \
{ return HL_ERR_EVTHUB_NOT_INIT; } \
    }while(0)

HL_S32 HL_EVTHUB_Init()
{
    return init();
}

HL_S32 HL_EVTHUB_Deinit()
{
    CHECK_EVENTHUB_INIT();
    deinit();
    return 0;
}

HL_S32 HL_EVTHUB_Register(HL_EVENT_ID EventID)
{
    CHECK_EVENTHUB_INIT();
    return event_id_reg(EventID);
}

HL_S32 HL_EVTHUB_UnRegister(HL_EVENT_ID EventID)
{
    CHECK_EVENTHUB_INIT();
    return event_id_unreg(EventID);
}


HL_S32 HL_EVTHUB_Publish(HL_EVENT_S *pEvent)
{
    CHECK_EVENTHUB_INIT();
    CHECK_NULL_PTR(pEvent);

    static int count = 0;
    if(count++ == 0)
    {
        sleep(1); //第一次发布增加延时确保订阅完成
    }
    pEvent->u64CreateTime = (HL_U64)time(NULL);
    return event_pub(pEvent);
}

HL_S32 HL_EVTHUB_CreateSubscriber(HL_SUBSCRIBER *pSubscriber, HL_SUBSCRIBER_ATTR_S *pstSuberAttr)
{
    CHECK_EVENTHUB_INIT();
    CHECK_NULL_PTR(pSubscriber);
    CHECK_NULL_PTR(pstSuberAttr);
    CHECK_NULL_PTR(pstSuberAttr->funcProc);
    CHECK_NULL_PTR(pstSuberAttr->azName);

    HL_S32 ret = -1;
    HL_S32 sfd = create_unix_stream_socket(strBindFileName, 0);
    if ( sfd < 0 )
    {
        LOG("ERROR\n");
        return -1;
    }
    *pSubscriber = sfd;

    TransPack pack;
    memset(&pack, 0, sizeof(pack));
    pack.trans_type = TRANS_SUB_CREAT;
    memcpy(pack.event.aszPayload, pstSuberAttr, sizeof(HL_SUBSCRIBER_ATTR_S));

    ret = send_data(*pSubscriber, pack);
    if(ret > 0)
    {
        ret = 0;
    }
    return ret;
}

HL_S32 HL_EVTHUB_DestroySubscriber(HL_SUBSCRIBER subscriber)
{
    CHECK_EVENTHUB_INIT();

    HL_S32 sfd = subscriber;
    return destroy_unix_socket(sfd);
}

HL_S32 HL_EVTHUB_Subscribe(HL_SUBSCRIBER subscriber, HL_EVENT_ID EventID)
{
    CHECK_EVENTHUB_INIT();

    HL_S32 ret = -1;
    TransPack pack;
    memset(&pack, 0, sizeof(pack));
    pack.trans_type = TRANS_SUB_EVENT;
    pack.event.EventID = EventID;

    ret = send_data(subscriber, pack);
    if(ret < 0)
        return ret;

    return 0;
}

HL_S32 HL_EVTHUB_UnSubscribe(HL_SUBSCRIBER subscriber, HL_EVENT_ID EventID)
{
    CHECK_EVENTHUB_INIT();

    HL_S32 ret = -1;
    TransPack pack;
    memset(&pack, 0, sizeof(pack));
    pack.trans_type = TRANS_UNSUB_EVENT;
    pack.event.EventID = EventID;

    ret = send_data(subscriber, pack);
    if(ret < 0)
    {
        return ret;
    }
    return 0;
}

HL_S32 HL_EVTHUB_GetEventHistory(HL_EVENT_ID EventID, HL_EVENT_S *pEvent)
{
    CHECK_EVENTHUB_INIT();
    CHECK_NULL_PTR(pEvent);

    return get_event_history(EventID, pEvent);
}

HL_S32 HL_EVTHUB_SetEnabled(HL_BOOL bFlag)
{
    CHECK_EVENTHUB_INIT();
    setEnable(bFlag);
    return 0;
}

HL_S32 HL_EVTHUB_GetEnabled(HL_BOOL *pFlag)
{
    CHECK_EVENTHUB_INIT();
    CHECK_NULL_PTR(pFlag);
    *pFlag = enable();
    return 0;
}


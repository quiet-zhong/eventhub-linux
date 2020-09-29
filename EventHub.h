#ifndef EVENTHUB_H
#define EVENTHUB_H

#include "hl_type.h"

#ifdef __cplusplus
extern "C" {
#endif


#define HL_ERR_EVTHUB_NULL_PTR              -0x1001
#define HL_ERR_EVTHUB_NOT_INIT              -0x1002
#define HL_ERR_EVTHUB_HANDLE_INVALID        -0x1003


#define EVENT_PAYLOAD_LEN               (512)
#define HL_EVTHUB_SUBSCRIBE_NAME_LEN    (16)

typedef HL_U32 HL_EVENT_ID;

typedef struct _EVENT_S
{
    HL_EVENT_ID EventID;
    HL_S32      arg1;
    HL_S32      arg2;
    HL_S32      s32Result;
    HL_U64      u64CreateTime;
    HL_CHAR     aszPayload[EVENT_PAYLOAD_LEN];
} HL_EVENT_S;

typedef struct _SUBSCRIBER_PARAM_S
{
    HL_CHAR     azName[HL_EVTHUB_SUBSCRIBE_NAME_LEN]; //订阅者名
    HL_S32      (*funcProc)(HL_EVENT_S* pEvent, HL_VOID* argv); //回调函数
    HL_VOID*    argv;       //回调参数
    HL_BOOL     bSync;      //是否同步
    HL_U16      u16Pri;     //优先级
}HL_SUBSCRIBER_ATTR_S;

typedef HL_S32 HL_SUBSCRIBER;

/**
 * @brief module init
 *
 * @return  0 success, non-zero error code
 */
HL_S32 HL_EVTHUB_Init();

/**
 * @brief module deinit
 *
 * @return  0 success, non-zero error code
 */
HL_S32 HL_EVTHUB_Deinit();

/**
  * @brief register event id
  *
  * @param[in]  EventID: event ID
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_Register(HL_EVENT_ID EventID);

/**
  * @brief unregister event id
  *
  * @param[in]  EventID: event ID
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_UnRegister(HL_EVENT_ID EventID);

/**
  * @brief publish event
  *
  * @param[in]  pEvent: event point
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_Publish(HL_EVENT_S *pEvent);

/**
 * @brief create subscriber
 *
 * @param pSubscriber[out]: the handle of subscriber
 * @param pstSuberAttr[in]: point of subscriber attribute
 * @return
 */
HL_S32 HL_EVTHUB_CreateSubscriber(HL_SUBSCRIBER *pSubscriber, HL_SUBSCRIBER_ATTR_S *pstSuberAttr);

/**
  * @brief destroy subscriber
  *
  * @param[in]  subscriber: the handle of subscriber
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_DestroySubscriber(HL_SUBSCRIBER subscriber);

/**
  * @brief subscribe event
  *
  * @param[in]  pSubscriber: subscribe handle
  * @param[in]  EventID: event ID
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_Subscribe(HL_SUBSCRIBER subscriber, HL_EVENT_ID EventID);

/**
  * @brief unsubscribe event
  *
  * @param[in]  pSubscriber: subscribe handle
  * @param[in]  EventID: event ID
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_UnSubscribe(HL_SUBSCRIBER subscriber, HL_EVENT_ID EventID);

/**
  * @brief get event history
  *
  * @param[in]  EventID: event id
  * @param[in]  pEvent: point of event
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_GetEventHistory(HL_EVENT_ID EventID, HL_EVENT_S *pEvent);

/**
  * @brief set publish enable or disable
  *
  * @param[in]  bFlag: flag
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_SetEnabled(HL_BOOL bFlag);

/**
  * @brief get publish enable or disable
  *
  * @param[in]  pFlag: point of flag
  * @return     0 success, non-zero error code
  */
HL_S32 HL_EVTHUB_GetEnabled(HL_BOOL *pFlag);


#ifdef __cplusplus
}
#endif

#endif // EVENTHUB_H

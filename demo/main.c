#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "EventHub.h"
#include "debug.h"
#include "dump.h"

#define SD_IN      0x00100201
#define SD_OUT     0x00100202
#define WIFI_OUT   0x00100203

typedef struct EventProcParam
{
    HL_SUBSCRIBER suber; //订阅器
    HL_S32  times;       //触发次数
}EventProcParam;

HL_S32 EVTHUB_EventProc(HL_EVENT_S* pEvent, HL_VOID* argv)
{
    EventProcParam param = *(EventProcParam*)argv;

    HL_S32 times = param.times;
    HL_SUBSCRIBER suber = param.suber;

    switch (pEvent->EventID)
    {
    case SD_IN:
        LOG("##############################################################################, %d\n", times);
        break;
    case SD_OUT:
        LOG("############################################\n");
        break;
    case WIFI_OUT:
        LOG("################n");
        break;
    default:
        break;
    }

    LOG("suber=%04d, ID:%08X, arg1:%d\t arg2:%d\t time:%lld, aszPayload:%s\n", suber,
        pEvent->EventID, pEvent->arg1, pEvent->arg2, pEvent->u64CreateTime, pEvent->aszPayload);
    return HL_SUCCESS;
}

void signal_handler(int signo)
{
    printf("\n=========>>>catch signal %d <<<=========\n", signo);

    printf("Dump stack start...\n");

    dump("./eventhub", "./crash.log");

    printf("Dump stack end...\n");

    signal(signo, SIG_DFL); /* 恢复信号默认处理 */
    raise(signo);           /* 重新发送信号 */
}

HL_S32 main(HL_S32 argc, char* argv[])
{
    (HL_VOID)argc;
    (HL_VOID)argv;

    signal(SIGSEGV,     signal_handler );
    signal(SIGFPE,      signal_handler );
    signal(SIGILL,      signal_handler );
    signal(SIGBUS,      signal_handler );
    signal(SIGABRT,     signal_handler );
    signal(SIGSYS,      signal_handler );

    HL_S32 s32Ret = HL_SUCCESS;
    HL_S32 i = 0;

    HL_SUBSCRIBER suber1, suber2, suber3, suber4;
    EventProcParam param1, param2, param3, param4;
    HL_SUBSCRIBER_ATTR_S suberParam;
    memset(&suberParam, 0, sizeof(HL_SUBSCRIBER_ATTR_S));

    HL_EVENT_S stEvent1 = {SD_IN,    111, 111, 0, 0, "SD_IN"};
    HL_EVENT_S stEvent2 = {SD_OUT,   222, 222, 0, 0, "SD_OUT"};
    HL_EVENT_S stEvent3 = {WIFI_OUT, 333, 333, 0, 0, "WIFI_OUT"};

    //! ############################################################
    //! 模块初始化
    HL_EVTHUB_Init();

      //! ############################################################
    //! 创建订阅器1
    strcpy(suberParam.azName, "sub1");
    suberParam.funcProc = EVTHUB_EventProc;
    suberParam.argv     = &param1;
    suberParam.bSync    = HL_TRUE;
    suberParam.u16Pri   = 40;

    HL_EVTHUB_CreateSubscriber(&suber1, &suberParam);
    LOG("create suber1 ==> %d\n", suber1);

    //! ############################################################
    //! 创建订阅器2
    strcpy(suberParam.azName, "sub2");
    suberParam.funcProc = EVTHUB_EventProc;
    suberParam.argv     = &param2;
    suberParam.bSync    = HL_TRUE;
    suberParam.u16Pri   = 20;

    HL_EVTHUB_CreateSubscriber(&suber2, &suberParam);
    LOG("create suber2 ==> %d\n", suber2);

    //! ############################################################
    //! 创建订阅器3, 异步
    strcpy(suberParam.azName, "sub3");
    suberParam.funcProc = EVTHUB_EventProc;
    suberParam.argv     = &param3;
    suberParam.bSync    = HL_FALSE;
    suberParam.u16Pri   = 20;

    HL_EVTHUB_CreateSubscriber(&suber3, &suberParam);
    LOG("create suber3 ==> %d\n", suber3);

    //! ############################################################
    //! 创建订阅器4
    strcpy(suberParam.azName, "sub4");
    suberParam.funcProc = EVTHUB_EventProc;
    suberParam.argv     = &param4;
    suberParam.bSync    = HL_TRUE;
    suberParam.u16Pri   = 30;

    HL_EVTHUB_CreateSubscriber(&suber4, &suberParam);
    LOG("create suber4 ==> %d\n", suber4);

    param1.suber = suber1;
    param2.suber = suber2;
    param3.suber = suber3;
    param4.suber = suber4;

    //! 循环压力测试
    //! ############################################################
//    while(1)
    {
        //! 向eventhub注册事件ID
        HL_EVTHUB_Register(SD_OUT);
        HL_EVTHUB_Register(SD_IN);
        HL_EVTHUB_Register(WIFI_OUT);

        //! ############################################################
        //! 订阅事件

        HL_EVTHUB_Subscribe(suber1, SD_IN);
        HL_EVTHUB_Subscribe(suber1, SD_OUT);
        HL_EVTHUB_Subscribe(suber1, WIFI_OUT);

        HL_EVTHUB_Subscribe(suber2, SD_OUT);
        HL_EVTHUB_Subscribe(suber2, WIFI_OUT);

        HL_EVTHUB_Subscribe(suber3, SD_OUT);
        HL_EVTHUB_Subscribe(suber3, WIFI_OUT);

        HL_EVTHUB_Subscribe(suber4, SD_OUT);

        //! ############################################################
        //! 发布事件
        do{
            param1.times = i;
            param2.times = i;
            param3.times = i;
            param4.times = i;

            usleep(1 * 1000);
            s32Ret = HL_EVTHUB_Publish(&stEvent1); //SD_IN

            s32Ret = HL_EVTHUB_Publish(&stEvent2); //SD_OUT

            s32Ret = HL_EVTHUB_Publish(&stEvent3); //WIFI_OUT
        }while( (i++%1000) != 300);

        //! ############################################################
        //取消几个订阅再发布
        HL_EVTHUB_UnSubscribe(suber2, SD_OUT);
        HL_EVTHUB_UnSubscribe(suber3, WIFI_OUT);

        do{
            param1.times = i;
            param2.times = i;
            param3.times = i;
            param4.times = i;

            usleep(1 * 1000);
            s32Ret = HL_EVTHUB_Publish(&stEvent1); //SD_IN

            s32Ret = HL_EVTHUB_Publish(&stEvent2); //SD_OUT

            s32Ret = HL_EVTHUB_Publish(&stEvent3); //WIFI_OUT
        }while( (i++%1000) != 600);


        //! ############################################################
        //注销几个事件再发布
        HL_EVTHUB_UnRegister(WIFI_OUT);
        HL_EVTHUB_UnRegister(SD_OUT);
        do{
            param1.times = i;
            param2.times = i;
            param3.times = i;
            param4.times = i;

            usleep(1 * 1000);
            s32Ret = HL_EVTHUB_Publish(&stEvent1); //SD_IN

            s32Ret = HL_EVTHUB_Publish(&stEvent2); //SD_OUT

            s32Ret = HL_EVTHUB_Publish(&stEvent3); //WIFI_OUT
        }while( (i++%1000) != 999);

    }


    //! ################################################################
    //! 结束
//    while(1)
    {
        sleep(10);
    }
    HL_EVTHUB_DestroySubscriber(suber1);
    HL_EVTHUB_DestroySubscriber(suber2);
    HL_EVTHUB_DestroySubscriber(suber3);
    HL_EVTHUB_DestroySubscriber(suber4);

    HL_EVTHUB_Deinit();

    return s32Ret;
}

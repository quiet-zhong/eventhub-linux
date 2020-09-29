#include "EventHubInner.h"
#include "debug.h"
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/prctl.h>

#define EPOLL_MAX_EVENTS        512
#define THREAD_POOL_COUNT       8
#define THREAD_POOL_FUNC_QUEUE  256

typedef struct _PrivData
{
    HL_BOOL                     m_bInit;        //是否已经初始化
    HL_S32                      m_epollFd;
    HL_S32                      m_sockServer;   //服务端socket

    sem_t                       m_semParse;     //事件解析线程信号量
    sem_t                       m_semSyncRun;   //事件同步处理线程信号量
    HL_BOOL                     m_bEnable;      //事件发布是否有效

    threadpool_t                *m_threadPool;  //线程池
}PrivData;

static  PrivData     g_priv_data;

//! ###################################################
#include <iostream>
#include <map>
#include <list>
#include <queue>
#include <set>
using namespace std;

//! ###################################################
static queue<HL_EVENT_S>    g_queueEvents;   // 事件队列
static pthread_mutex_t      g_queueEventsLock = PTHREAD_MUTEX_INITIALIZER;

//! ###################################################
static queue<ThreadFuncArg> g_queueSyncFunc; // 同步回调队列
static pthread_mutex_t      g_queueSyncFuncLock = PTHREAD_MUTEX_INITIALIZER;

//! ###################################################
static map< HL_SUBSCRIBER, HL_SUBSCRIBER_ATTR_S>   g_mapSuberParam; // 订阅者 -> 订阅者参数
static pthread_mutex_t      g_mapSuberParamLock = PTHREAD_MUTEX_INITIALIZER;

//! ###################################################
struct cmp_set_suberpri //set 排序
{
    bool operator() (const HL_SUBSCRIBER &sub1, const HL_SUBSCRIBER &sub2) const {
        return (HL_BOOL)(g_mapSuberParam.at(sub1).u16Pri < g_mapSuberParam.at(sub2).u16Pri);
    }
};
typedef set<HL_SUBSCRIBER, cmp_set_suberpri> SuberSet;
static map< HL_EVENT_ID, SuberSet >     g_mapSetEventSuber; // 事件ID -> 订阅者集合
static pthread_mutex_t                  g_mutexEventSuberLock = PTHREAD_MUTEX_INITIALIZER;

//! ###################################################
static map< HL_EVENT_ID, list<HL_EVENT_S> >         g_mapListEventHistory; // 事件ID历史记录

//! ###################################################
static HL_VOID LOCK(pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
}

static HL_VOID UNLOCK(pthread_mutex_t *lock)
{
    pthread_mutex_unlock(lock);
}

static HL_VOID var_init()
{
    g_priv_data.m_sockServer = 0;
    g_priv_data.m_epollFd = 0;
    g_priv_data.m_bEnable = HL_TRUE;
    g_priv_data.m_bInit = HL_FALSE;

    LOCK(&g_mutexEventSuberLock);
    g_mapSetEventSuber.clear();
    UNLOCK(&g_mutexEventSuberLock);

    LOCK(&g_mapSuberParamLock);
    g_mapSuberParam.clear();
    UNLOCK(&g_mapSuberParamLock);

    g_mapListEventHistory.clear();
}

static HL_VOID var_deinit()
{
    var_init();
}

static HL_VOID signal_handler(HL_S32 sig)
{
    unlink( strBindFileName );
    signal(sig, SIG_DFL);
    raise(sig);
}

static HL_S32 epoll_add(HL_S32 fd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(g_priv_data.m_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        LOG("ERROR\n");
        perror("epoll_ctl add failed");
        return -1;
    }
    return 0;
}

static HL_S32 event_sub_create(HL_EVENT_S evt, HL_SUBSCRIBER subscriber)
{
    HL_SUBSCRIBER_ATTR_S stSubscriber;
    memcpy(&stSubscriber, evt.aszPayload, sizeof(stSubscriber));
    LOCK(&g_mapSuberParamLock);
    g_mapSuberParam.insert(pair<HL_SUBSCRIBER, HL_SUBSCRIBER_ATTR_S>(subscriber, stSubscriber));
    UNLOCK(&g_mapSuberParamLock);
    return 0;
}

static HL_S32 event_sub(HL_EVENT_ID EventID, HL_SUBSCRIBER subscriber)
{
    LOCK(&g_mutexEventSuberLock);
    if(0 != g_mapSetEventSuber.count(EventID))
    {
        g_mapSetEventSuber.at(EventID).insert(subscriber);
        UNLOCK(&g_mutexEventSuberLock);
        return 0;
    }
    UNLOCK(&g_mutexEventSuberLock);
    return -1;
}

static HL_S32 event_unsub(HL_EVENT_ID EventID, HL_SUBSCRIBER subscriber)
{
    LOCK(&g_mutexEventSuberLock);
    if(0 != g_mapSetEventSuber.count(EventID))
    {
        g_mapSetEventSuber.at(EventID).erase(subscriber);
        UNLOCK(&g_mutexEventSuberLock);
        return 0;
    }
    UNLOCK(&g_mutexEventSuberLock);
    return -1;
}

static HL_VOID *thread_sync_proc(HL_VOID *arg)
{
    (void) arg;
    prctl(PR_SET_NAME, "evthub_sync");

    while(1)
    {
        sem_wait(&g_priv_data.m_semSyncRun);

        LOCK(&g_queueSyncFuncLock);
        if(g_queueSyncFunc.empty())
        {
            UNLOCK(&g_queueSyncFuncLock);
            continue;
        }
        ThreadFuncArg threadArg = g_queueSyncFunc.front();
        g_queueSyncFunc.pop();
        UNLOCK(&g_queueSyncFuncLock);

        threadArg.event.s32Result = threadArg.param.funcProc(&threadArg.event, threadArg.param.argv);
    }
}

static HL_VOID *thread_async_proc(HL_VOID *arg)
{
    prctl(PR_SET_NAME, "evthub_tmp");
    ThreadFuncArg funcArg = *(ThreadFuncArg*)(arg);
    funcArg.event.s32Result = funcArg.param.funcProc(&funcArg.event, funcArg.param.argv);
    return NULL;
}

static HL_VOID *thread_subscriber_proc(HL_VOID *arg)
{
    (void) arg;
    prctl(PR_SET_NAME, "evthub_sub");

    HL_S32 ret = 0;

    HL_S32 sockeServer = g_priv_data.m_sockServer;
    HL_S32 epollfd = g_priv_data.m_epollFd;

    HL_S32 subfd;              // 订阅者
    HL_S32 nfds;
    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (1)
    {
        nfds = epoll_wait(epollfd, events, EPOLL_MAX_EVENTS, -1);
        if (nfds <= 0)
            continue;

        for (HL_S32 i = 0; i < nfds; i++)
        {
            subfd = events[i].data.fd;

            if (subfd == sockeServer)
            {
                // 客户端请求连接
                subfd = accept_unix_stream_socket(sockeServer, 0);
                if (subfd < 0)
                {
                    LOG("ERROR\n");
                    continue;
                }

                if (epoll_add(subfd) == -1)
                {
                    LOG("ERROR\n");
                    continue;
                }
                LOG("[%d] Connected\n", subfd);
                continue;
            }

            // 客户端有数据发送过来
            TransPack pack;
            HL_S32 len = recv(subfd, &pack, sizeof(pack), 0);
            if(len == 0)
            {
                LOG("[%d] DisConnect\n", subfd);

                LOCK(&g_mapSuberParamLock);
                g_mapSuberParam.erase(subfd);
                UNLOCK(&g_mapSuberParamLock);

                epoll_ctl(epollfd, EPOLL_CTL_DEL, subfd, NULL);
                destroy_unix_socket(subfd);
                continue;
            }
            if(len < 0)
            {
                LOG("Recv Error\n");
                continue;
            }

            HL_EVENT_S evt = pack.event;
            HL_EVENT_ID eid = evt.EventID;

            switch (pack.trans_type) {
            case TRANS_SUB_CREAT:
                //设置订阅参数
                LOG("[%d] Create Subscriber\n", subfd);
                ret = event_sub_create(evt, subfd);
                if(ret < 0)
                {
                    LOG("ERROR\n");
                }
                break;
            case TRANS_SUB_EVENT:
                //订阅
                LOG("[%d] Subscribe event=%08X\n", subfd, eid);
                ret = event_sub(eid, subfd);
                if(ret < 0)
                {
                    LOG("ERROR\n");
                }
                break;
            case TRANS_UNSUB_EVENT:
                //取消订阅
                LOG("[%d] UnSubscribe event=%08X\n", subfd, eid);
                ret = event_unsub(eid, subfd);
                if(ret < 0)
                {
                    LOG("ERROR\n");
                }
                break;
            default:
                break;
            }

        }//for end

    }

}

static HL_VOID *thread_publisher_proc(HL_VOID *arg)
{
    (void) arg;
    prctl(PR_SET_NAME, "evthub_pub");

    while(1)
    {
        sem_wait(&g_priv_data.m_semParse);

        LOCK(&g_queueEventsLock);
        if(g_queueEvents.empty())
        {
            UNLOCK(&g_queueEventsLock);
            continue;
        }
        HL_EVENT_S event = g_queueEvents.front();
        g_queueEvents.pop();
        UNLOCK(&g_queueEventsLock);

        HL_EVENT_ID eid = event.EventID;

        //添加历史记录
        if(0 != g_mapListEventHistory.count(eid))
        {
            g_mapListEventHistory.at(eid).push_back(event);
        }

        LOCK(&g_mutexEventSuberLock);
        if(0 == g_mapSetEventSuber.count(eid))
        {
            UNLOCK(&g_mutexEventSuberLock);
            continue;
        }
        SuberSet suberSet = g_mapSetEventSuber.at(eid);
        UNLOCK(&g_mutexEventSuberLock);

        for(SuberSet::iterator iter = suberSet.begin(); iter != suberSet.end(); iter++)
        {
            HL_SUBSCRIBER suber = *iter;

            LOCK(&g_mapSuberParamLock);
            if(0 != g_mapSuberParam.count(suber))
            {
                HL_SUBSCRIBER_ATTR_S param = g_mapSuberParam.at(suber);
                UNLOCK(&g_mapSuberParamLock);

                // LOG("Event[%08x] call subscriber[%d] name=[%s] sync=[%d]\n",
                //  eid, suber, param.azName, param.bSync);

                ThreadFuncArg funcArg;
                funcArg.event = event;
                funcArg.param = param;

                //! 区别是否同步
                if(param.bSync)
                {
                    //! 添加到同步运行队列
                    LOCK(&g_queueSyncFuncLock);
                    g_queueSyncFunc.push(funcArg);
                    UNLOCK(&g_queueSyncFuncLock);
                    sem_post(&g_priv_data.m_semSyncRun);
                }
                else
                {
                    threadpool_add(g_priv_data.m_threadPool, thread_async_proc, (void *)&funcArg, 0);
                }
            }
            else
            {
                UNLOCK(&g_mapSuberParamLock);
            }
        }


    }
}

HL_S32 init()
{
    var_init();

    signal(SIGINT,  signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    HL_S32 ret = 0;

    sem_init(&g_priv_data.m_semParse, 0, 0);
    sem_init(&g_priv_data.m_semSyncRun, 0, 0);

    g_priv_data.m_epollFd = epoll_create(EPOLL_MAX_EVENTS);
    if (g_priv_data.m_epollFd == -1)
    {
        ret = -1;
        goto ERR;
    }

    g_priv_data.m_sockServer = create_unix_server_socket(strBindFileName, LIBSOCKET_STREAM, 0);
    if ( g_priv_data.m_sockServer < 0 )
    {
        ret = -2;
        goto ERR1;
    }

    if (epoll_add(g_priv_data.m_sockServer) == -1)
    {
        ret = -3;
        goto ERR2;
    }

    g_priv_data.m_threadPool = threadpool_create(THREAD_POOL_COUNT, THREAD_POOL_FUNC_QUEUE, 0);
    if(g_priv_data.m_threadPool == NULL)
    {
        ret = -4;
        goto ERR2;
    }

    //创建订阅者服务线程,接受订阅，取消订阅等事件
    ret = threadpool_add(g_priv_data.m_threadPool, thread_subscriber_proc, NULL, 0);
    if(ret != 0)
    {
        ret = -5;
        goto ERR3;
    }

    //创建发布者服务线程,解析发布事件对应的订阅者
    ret = threadpool_add(g_priv_data.m_threadPool, thread_publisher_proc, NULL, 0);
    if(ret != 0)
    {
        ret = -6;
        goto ERR3;
    }

    //创建同步执行线程
    ret = threadpool_add(g_priv_data.m_threadPool, thread_sync_proc, NULL, 0);
    if(ret != 0)
    {
        ret = -7;
        goto ERR3;
    }

    g_priv_data.m_bInit = HL_TRUE;
    return 0;

ERR3:
    threadpool_destroy(g_priv_data.m_threadPool, 0);
ERR2:
    destroy_unix_socket(g_priv_data.m_sockServer);
ERR1:
    close(g_priv_data.m_epollFd);
ERR:
    LOG("ERROR ret:%d\n", ret);
    return ret;
}

HL_S32 deinit()
{
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    sem_destroy(&g_priv_data.m_semParse);
    sem_destroy(&g_priv_data.m_semSyncRun);

    destroy_unix_socket(g_priv_data.m_sockServer);

    close(g_priv_data.m_epollFd);

    unlink(strBindFileName);

    threadpool_destroy(g_priv_data.m_threadPool, 0);

    g_priv_data.m_bInit = HL_FALSE;
    var_deinit();
    return 0;
}

HL_BOOL isInit()
{
    return g_priv_data.m_bInit;
}

HL_BOOL enable()
{
    return g_priv_data.m_bEnable;
}

HL_VOID setEnable(const HL_BOOL bEnable)
{
    g_priv_data.m_bEnable = bEnable;
}

HL_S32 event_id_reg(HL_EVENT_ID id)
{
    set<HL_SUBSCRIBER,cmp_set_suberpri> s;
    LOCK(&g_mutexEventSuberLock);
    g_mapSetEventSuber.insert(pair<HL_EVENT_ID, set<HL_SUBSCRIBER,cmp_set_suberpri> >(id, s) );
    UNLOCK(&g_mutexEventSuberLock);
    list<HL_EVENT_S> lst;
    g_mapListEventHistory.insert(pair<HL_EVENT_ID, list<HL_EVENT_S> >(id, lst) );
    return 0;
}

HL_S32 event_id_unreg(HL_EVENT_ID id)
{
    LOCK(&g_mutexEventSuberLock);
    g_mapSetEventSuber.erase(id);
    UNLOCK(&g_mutexEventSuberLock);
    return 0;
}

HL_S32 event_pub(HL_EVENT_S *pEvent)
{
    if(g_priv_data.m_bEnable == HL_TRUE)
    {
        LOCK(&g_queueEventsLock);
        g_queueEvents.push(*pEvent);
        UNLOCK(&g_queueEventsLock);

        int val = 0;
        sem_getvalue(&g_priv_data.m_semParse, &val);
        if(val < 10)
            sem_post(&g_priv_data.m_semParse);
    }
    return 0;
}

HL_S32 send_data(HL_S32 sock, TransPack pack)
{
    HL_S32 ret = send(sock, (HL_VOID *)&pack, sizeof(pack), 0);
    if ( ret < 0 )
    {
        LOG("ERROR\n");
        return -2;
    }
    return ret;
}

HL_S32 get_event_history(HL_EVENT_ID EventID, HL_EVENT_S *pEvent)
{
    if( 0 != g_mapListEventHistory.count(EventID)  && NULL != pEvent )
    {
        list<HL_EVENT_S> lst = g_mapListEventHistory.at(EventID);
        int i = 0;
        for(list<HL_EVENT_S>::iterator iter = lst.begin(); iter != lst.end(); iter++)
        {
            HL_EVENT_S evt = *iter;
            memcpy(pEvent+i, &evt, sizeof(HL_EVENT_S));
            i++;
        }
        return i;
    }
    return 0;
}

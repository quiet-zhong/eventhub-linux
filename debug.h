#ifndef __DEBUG_H_
#define __DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#include <stdio.h>
#include <pthread.h>
static pthread_mutex_t _log_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOGLOCK()     pthread_mutex_lock(&_log_mutex)
#define LOGUNLOCK()   pthread_mutex_unlock(&_log_mutex)

#define LOG(fmt,...)                            \
    do                                          \
{                                               \
    LOGLOCK();                                  \
    printf("[%s][%04d]: ", __func__, __LINE__); \
    printf(fmt, ##__VA_ARGS__);                 \
    fflush(stdout);                             \
    LOGUNLOCK();                                \
    }while(0)
#else
#define LOG(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif

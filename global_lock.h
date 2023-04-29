#ifndef  __GLOBAL_LOCK_H__
#define  __GLOBAL_LOCK_H__

#include <pthread.h>

// 声明全局互斥量
extern pthread_mutex_t mutex;
extern pthread_cond_t usb_signal;

#endif // ! __GLOBAL_LOCK_H__
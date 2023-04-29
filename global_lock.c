#include "global_lock.h"

// 定义全局互斥量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t usb_signal = PTHREAD_COND_INITIALIZER;
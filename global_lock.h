#ifndef  __GLOBAL_LOCK_H__
#define  __GLOBAL_LOCK_H__

#include <pthread.h>
#include "v4l2_camera.h"

// 声明全局互斥量
extern pthread_mutex_t mutex;
extern pthread_cond_t  usb_cond;


extern camera_data* global_camera_data;

#endif // ! __GLOBAL_LOCK_H__
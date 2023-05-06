#ifndef __CAMERA_MANAGER_THREAD_H__
#define __CAMERA_MANAGER_THREAD_H__
#include <pthread.h>

extern pthread_t g_thread_usb_camera[10];

void* thread_manager_cameras(void* arg);

#endif // !__CAMERA_MANAGER_THREAD_H__
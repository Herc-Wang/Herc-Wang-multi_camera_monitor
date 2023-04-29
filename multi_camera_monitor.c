#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include "v4l2_camera.h"
#include "usb_monitor.h"
#include "global_lock.h"
#include "push_steam.h"
#include "camera_manager_thread.h"

#define NUM_CAMERA_THREADS 4                            //打开摄像头的最大线程数量
#define PUSH_STREAM_ADDR    "udp://192.168.124.10:1234" //推流地址

camera_data* global_camera_data;        //全局摄像头结构体指针



/***********************************************************************************************
主线程：启动usb监控线程，推流线程，摄像头捕获线程
usb监控线程：监测是否有usb设备热插拔。如有则发出信号
摄像头捕获线程：根据信号来重新打开所有摄像头
推流线程：将lcd数据推流
***********************************************************************************************/
int main(int argc, char *argv[])
{
    long i;
    int ret;

    pthread_t thread_push_video;                        //推流线程
    pthread_t thread_usb_camera_monitor;                //监测usb线程
    pthread_t thread_camera_mangaer;                    //管理摄像头线程

    global_camera_data = init_global_camera_struct(NUM_CAMERA_THREADS); //初始化全局摄像头结构体

    //枚举有效的摄像头
    enmu_valid_camera(global_camera_data->pCamera_private_param, &global_camera_data->vaild_camera_num);
    // for(i=0; i<global_camera_data->vaild_camera_num; i++)      //打印有效的摄像头路径
    //     printf("%s\r\n", global_camera_data->pCamera_private_param[i].path);

    //创建线程，用于监测usb
    pthread_create(&thread_usb_camera_monitor, NULL, thread_usb_monitor, NULL);
    
    //创建线程，用于管理摄像头的子线程
    pthread_create(&thread_camera_mangaer, NULL, thread_manager_cameras, NULL);

    //设置摄像头采集数据显示到lcd的位置索引
    relocation_camera_display(global_camera_data->pCamera_private_param, global_camera_data->vaild_camera_num);

    //创建线程，用于启动ffmpeg推流（udp）
    pthread_create(&thread_push_video, NULL, thread_push_stream, PUSH_STREAM_ADDR);

    
    pthread_join(thread_usb_camera_monitor, NULL);
    pthread_join(thread_camera_mangaer, NULL);
    pthread_join(thread_push_video, NULL);
    
    deinit_global_camera_struct(global_camera_data);    //回收申请的资源
    return 0;
}
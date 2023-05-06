#include <bits/pthreadtypes.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "camera_manager_thread.h"
#include "v4l2_camera.h"
#include "global_lock.h"

pthread_t g_thread_usb_camera[10];

/**
 * @brief thread_manager_cameras - 该线程用于管理摄像头的子线程
 */
void* thread_manager_cameras(void* arg){
    int i, ret;
    pthread_attr_t attr;
    size_t stack_size = 1024*2;    //2MB

    //初始化线程属性
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);   //设置栈大小

    while(1){
        //枚举有效的摄像头
        enmu_valid_camera(global_camera_data->pCamera_private_param, &global_camera_data->vaild_camera_num);
        // for(i=0; i<global_camera_data->vaild_camera_num; i++)      //打印有效的摄像头路径
        //     printf("%s\r\n", global_camera_data->pCamera_private_param[i].path);
        printf("now vaild camera num = %d\r\n", global_camera_data->vaild_camera_num);

        //设置摄像头采集数据显示到lcd的位置索引
        relocation_camera_display(global_camera_data->pCamera_private_param, global_camera_data->vaild_camera_num);

        for(i=0; i<global_camera_data->vaild_camera_num; i++){
            printf("Creating thread %d\n", i);
            ret = pthread_create(&g_thread_usb_camera[i], &attr, v4l2_camera, \
                                    (void *)&(global_camera_data->pCamera_private_param[i]));
            if(ret){
                printf("Error creating thread , err code = %d\n", ret);
                exit(-1); 
            }
            // sleep(1);// 使用互斥锁比较好
        }
        //等待usb监测信号，每次接收到信号则重新获取摄像头数据
        pthread_mutex_lock(&mutex);
        printf("debug:  >>>>>>pthread_cond_wait\r\n");
        pthread_cond_wait(&usb_cond, &mutex);
        printf("debug:  <<<<<<pthread_cond_wait satisfy\r\n");
        pthread_mutex_unlock(&mutex);
        
        printf("vaild_camera_num = %d\r\n", global_camera_data->vaild_camera_num);
        for(i=0; i<global_camera_data->vaild_camera_num; i++){
            printf("pthread_cancel thread[%d]\r\n", i);
            pthread_cancel(g_thread_usb_camera[i]);       //父线程主动发起取消请求
        }
        
        for(i=0; i<global_camera_data->vaild_camera_num; i++){
            printf("pthread_join thread[%d]\r\n", i);
            pthread_join(g_thread_usb_camera[i], NULL); 
        }
        global_camera_data->vaild_camera_num = 0;
    }
    return NULL;
}
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
#include <pthread.h>
#include "v4l2_camera.h"

#define NUM_THREADS 4

int main(int argc, char *argv[])
{
    long i;
    int ret;
    size_t stack_size = 1024*4;    //4MB
    camera_parameter camera_param[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;

    //设置摄像头显示的参数：路径，显示到lcd上的位置
    for(i=0; i<argc-1; i++){
        SCREEN_INDEX screen_index;
        strcpy(camera_param[i].path, argv[i+1]);
        switch (i) {
            case 0:screen_index = SCREEN_INDEX_LeftTop;break;
            case 1:screen_index = SCREEN_INDEX_RightTop;break;
            case 2:screen_index = SCREEN_INDEX_LeftBottom;break;
            case 3:screen_index = SCREEN_INDEX_RightBottom;break;
        }
        camera_param[i].screen_index = screen_index;
    }

    //初始化线程属性
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);   //设置栈大小

    for(i=0; i<argc-1; i++){
        printf("Creating thread %ld\n", i);
        ret = pthread_create(&threads[i], &attr, v4l2_camera, (void *)(&camera_param[i]));
        if(ret){
            printf("Error creating thread , err code = %d\n", ret);
            exit(-1); 
        }
        sleep(1);// 使用互斥锁比较好
    }

    pthread_exit(NULL);
    return 0;
}
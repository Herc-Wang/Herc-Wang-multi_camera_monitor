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
#include <linux/videodev2.h>
#include "v4l2_camera.h"

#define NUM_CAMERA_THREADS 4                            //打开摄像头的最大线程数量
#define PUSH_STREAM_ADDR    "udp://192.168.124.10:1234" //推流地址

/**
 * @brief 推流线程，通过system指令来调用ffmpeg进行推流
 *
 * @param arg 推流udp地址
 * @note 
 */
void *thread_push_stream(void *arg){
    char cmd[200] = "ffmpeg -f fbdev -framerate 15 -i /dev/fb0 -vf 'format=yuv420p' \
                     -c:v libx264 -preset ultrafast -tune zerolatency \
                     -f mpegts ";
    strcat(cmd, (char*)arg);
    system(cmd);        //启动命令
    return NULL;
}

/**
 * @brief enmu_valid_camera - 遍历可用摄像头，将信息存储到结构体pCameraParam中
 * 
 * @param pCameraParam 存储有效摄像头的信息
 * @param vaildCameraCnt 返回有效的摄像头个数
 * @return 0
*/
int enmu_valid_camera(camera_parameter* pCameraParam, int* vaildCameraCnt){
    int fd;
    int i = 0;
    struct v4l2_format fmt = {0};
    struct v4l2_capability caps;
    *vaildCameraCnt = 0;    //清0

    char camera_path[16];  
    for(i=0; i<16; i++){
        sprintf(camera_path, "/dev/video%d", i);
        fd = open(camera_path, O_RDONLY);
        if (fd == -1) {
            continue;
        }

        /* 查询设备功能 */
        if (ioctl(fd, VIDIOC_QUERYCAP, &caps) == -1) {
            #ifdef _DEBUG_
                perror("VIDIOC_QUERYCAP");
            #endif
            close(fd);
            continue;
        }

        /* 判断是否是视频采集设备 */
        if (!(V4L2_CAP_VIDEO_CAPTURE & caps.capabilities)) {
            #ifdef _DEBUG_
                fprintf(stderr, "Error: %s: No capture video device!\n", camera_path);
            #endif
            close(fd);
            continue;
        }

        /* 设置帧格式 ,只有可用摄像头才可以设置帧格式*/
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//type类型
        fmt.fmt.pix.width = 1;  //期待的视频帧宽度
        fmt.fmt.pix.height = 1;//期待的视频帧高度
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  //像素格式
        // printf("set before 视频帧大小<%d * %d>\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
        if (0 > ioctl(fd, VIDIOC_S_FMT, &fmt)) {
            #ifdef _DEBUG_
                fprintf(stderr, "ioctl error: VIDIOC_S_FMT: %s\n", strerror(errno));
            #endif
            continue;
        }

        camera_parameter *pCamera = &pCameraParam[*vaildCameraCnt];
        strcpy(pCamera->path, camera_path);
        (*vaildCameraCnt)++;

        close(fd);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    long i;
    int ret;
    int vaildCameraNum = 0; //有效的摄像头数量
    size_t stack_size = 1024*2;    //2MB
    camera_parameter *pCamera_param = (camera_parameter*)malloc(sizeof(camera_parameter)*NUM_CAMERA_THREADS);
    pthread_t threads[NUM_CAMERA_THREADS];
    pthread_t threadPushVideo;  //推流线程
    pthread_attr_t attr;

    //枚举有效的摄像头
    enmu_valid_camera(pCamera_param, &vaildCameraNum);
    // for(i=0; i<vaildCameraNum; i++)      //打印有效的摄像头路径
    //     printf("%s\r\n", pCamera_param[i].path);
    

    //设置摄像头显示的参数：路径，显示到lcd上的位置
    for(i=0; i<vaildCameraNum; i++){
        SCREEN_INDEX screen_index;
        switch (i) {
            case 0:screen_index = SCREEN_INDEX_LeftTop;break;
            case 1:screen_index = SCREEN_INDEX_RightTop;break;
            case 2:screen_index = SCREEN_INDEX_LeftBottom;break;
            case 3:screen_index = SCREEN_INDEX_RightBottom;break;
        }
        pCamera_param[i].screen_index = screen_index;
    }

    //初始化线程属性
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);   //设置栈大小

    for(i=0; i<vaildCameraNum; i++){
        printf("Creating thread %ld\n", i);
        ret = pthread_create(&threads[i], &attr, v4l2_camera, (void *)&(pCamera_param[i]));
        if(ret){
            printf("Error creating thread , err code = %d\n", ret);
            exit(-1); 
        }
        sleep(1);// 使用互斥锁比较好
    }

    //创建线程用于启动ffmpeg推流（udp）
    pthread_create(&threadPushVideo, NULL, thread_push_stream, PUSH_STREAM_ADDR);

    //等待线程结束，回收资源
    for(i=0; i<argc-1; i++){
        pthread_join(threads[i], NULL);
    }
    pthread_join(threadPushVideo, NULL);

    return 0;
}
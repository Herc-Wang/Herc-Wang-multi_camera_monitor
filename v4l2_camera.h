#ifndef __V4L2_CAMERA_H__
#define __V4L2_CAMERA_H__

// #define _DEBUG_

#define FRAMEBUFFER_COUNT   3               //帧缓冲数量

/*** 摄像头像素格式及其描述信息 ***/
typedef struct camera_format {
    unsigned char description[32];  //字符串描述信息
    unsigned int pixelformat;       //像素格式
} cam_fmt;

/*** 描述一个帧缓冲的信息 ***/
typedef struct cam_buf_info {
    unsigned short *start;      //帧缓冲起始地址
    unsigned long length;       //帧缓冲长度
} cam_buf_info;

typedef struct {
    char    path[20];
    int     screen_index;
    int     v4l2_fd;
    cam_buf_info buf_infos[FRAMEBUFFER_COUNT];
    cam_fmt cam_fmts[10];
    int frm_width, frm_height;   //视频帧宽度和高度
}camera_parameter;

typedef enum {
    SCREEN_INDEX_LeftTop,
    SCREEN_INDEX_RightTop,
    SCREEN_INDEX_LeftBottom,
    SCREEN_INDEX_RightBottom,
}SCREEN_INDEX;

void* v4l2_camera(void* arg);

#endif // !__V4L2_CAMERA_H__
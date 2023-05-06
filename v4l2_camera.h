#ifndef __V4L2_CAMERA_H__
#define __V4L2_CAMERA_H__

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

//摄像头私有参数结构体
typedef struct {
    char    path[20];
    int     screen_index;
    int     v4l2_fd;
    cam_buf_info buf_infos[FRAMEBUFFER_COUNT];
    cam_fmt cam_fmts[10];
    int frm_width, frm_height;   //视频帧宽度和高度
}camera_parameter;

//全部摄像头结构体链表
typedef struct {
    camera_parameter *pCamera_private_param;    //存储摄像头私有参数的数组
    int vaild_camera_num;                       //有效的摄像头数量
    int max_camera_num;                         //最大摄像头数量
}camera_data;

void* v4l2_camera(void* arg);
int enmu_valid_camera(camera_parameter* pCameraParam, int* vaildCameraCnt);
camera_data* init_global_camera_struct(int max_camera);
void deinit_global_camera_struct(camera_data* pgdata);
int relocation_camera_display(camera_parameter* pCameraParam, int vaildCameraCnt);
#endif // !__V4L2_CAMERA_H__
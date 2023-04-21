/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2021. All rights reserved.
 文件名 : v4l2_camera.c
 作者 : 邓涛
 版本 : V1.0
 描述 : V4L2摄像头应用编程实战
 其他 : 无
 论坛 : www.openedv.com
 日志 : 初版 V1.0 2021/7/09 邓涛创建
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>

#define argb8888_to_rgb565(color) ({ \
 unsigned int temp = (color); \
 ((temp & 0xF80000UL) >> 8) | \
 ((temp & 0xFC00UL) >> 5) | \
 ((temp & 0xF8UL) >> 3); \
 })

#define rgbIII_to_rgb565(r,g,b) ({ \
 (((unsigned int)(r) & 0xF8) << 8) | \
 (((unsigned int)(g) & 0xFC) << 5) | \
 (((unsigned int)(b) & 0xF8) << 0) ; \
 })

#define rgbIII_to_bgr565(r,g,b) ({ \
 (((unsigned int)(b) & 0xF8) << 8) | \
 (((unsigned int)(g) & 0xFC) << 5) | \
 (((unsigned int)(r) & 0xF8) << 0) ; \
 })

#define FB_DEV              "/dev/fb0"      //LCD设备节点
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

static int width;                       //LCD宽度
static int height;                      //LCD高度
static unsigned short *screen_base = NULL;//LCD显存基地址
static int fb_fd = -1;                  //LCD设备文件描述符
static int v4l2_fd = -1;                //摄像头设备文件描述符
static cam_buf_info buf_infos[FRAMEBUFFER_COUNT];
static cam_fmt cam_fmts[10];
static int frm_width, frm_height;   //视频帧宽度和高度

static int fb_dev_init()
{
    struct fb_var_screeninfo fb_var = {0};
    struct fb_fix_screeninfo fb_fix = {0};
    unsigned long screen_size;

    /* 打开framebuffer设备 */
    fb_fd = open(FB_DEV, O_RDWR);
    if (0 > fb_fd) {
        fprintf(stderr, "open error: %s: %s\n", FB_DEV, strerror(errno));
        return -1;
    }

    /* 获取framebuffer设备信息 */
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_fix);

    screen_size = fb_fix.line_length * fb_var.yres;
    width = fb_var.xres;
    height = fb_var.yres;
    // screen_size = fb_fix.line_length * height;
    //line_length:1600   表示一行占用的空间，已知一行像素点数为800，即每个点占2字节
    printf("screen_size:%lu   fb_var  width=%d,    height=%d   line_length=%d \r\n", screen_size, width, height, fb_fix.line_length);

    /* 内存映射 */
    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (MAP_FAILED == (void *)screen_base) {
        perror("mmap error 1");
        close(fb_fd);
        return -1;
    }

    /* LCD背景刷白 */
    memset(screen_base, 0xFE, screen_size);
    return 0;
}

static int v4l2_dev_init(const char *device)
{
    struct v4l2_capability cap = {0};

    /* 打开摄像头 */
    v4l2_fd = open(device, O_RDWR);
    if (0 > v4l2_fd) {
        fprintf(stderr, "open error: %s: %s\n", device, strerror(errno));
        return -1;
    }

    /* 查询设备功能 */
    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);

    /* 判断是否是视频采集设备 */
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        fprintf(stderr, "Error: %s: No capture video device!\n", device);
        close(v4l2_fd);
        return -1;
    }

    return 0;
}

static void v4l2_enum_formats(void)
{
    struct v4l2_fmtdesc fmtdesc = {0};

    /* 枚举摄像头所支持的所有像素格式以及描述信息 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc)) {

        // 将枚举出来的格式以及描述信息存放在数组中
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        strcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description);
        fmtdesc.index++;
    }
}

static void v4l2_print_formats(void)
{
    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_frmivalenum frmival = {0};
    int i;

    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (i = 0; cam_fmts[i].pixelformat; i++) {

        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat,
                    cam_fmts[i].description);

        /* 枚举出摄像头所支持的所有视频采集分辨率 */
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {

            printf("size<%d*%d> ",
                    frmsize.discrete.width,
                    frmsize.discrete.height);
            frmsize.index++;

            /* 获取摄像头视频采集帧率 */
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {

                printf("<%dfps>", frmival.discrete.denominator /
                        frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
}


static void yuyv_to_rgb565(unsigned char *yuyvdata, unsigned char *rgbdata, int w, int h)
{
    int i;
    int r1, g1, b1;
    
    // int r2, g2, b2;
    // printf("yuyv_to_rgb565:   loop time = %d \n", w * h / 2);
    for (i = 0; i < w * h; i++)
    {
        // printf("*************%s %d \r\n", __FUNCTION__, __LINE__);
        unsigned char data[4];
        memcpy((unsigned char*)data, (unsigned char*)(yuyvdata + i * 4), 4);
        // printf("data:%x, yuyvdata: %x\r\n", *(short*)data, yuyvdata[i * 4]);
        // Y0 U0 Y1 V1-->[Y0 U0 V1] [Y1 U0 v1]
        unsigned char Y0 = data[0];
        unsigned char U0 = data[1];
        unsigned char Y1 = data[2];
        unsigned char V1 = data[3];
        // printf("Y0=%x, U0=%x, Y1==%x, V1=%x\r\n", Y0, U0, Y1, V1);

        r1 = Y0 + 1.4075 * (V1-128); if(r1>255)r1=255; if(r1<0)r1=0;
	    g1 =Y0 - 0.3455 * (U0-128) - 0.7169 * (V1-128); if(g1>255)g1=255; if(g1<0)g1=0;
        b1 = Y0 + 1.779 *(U0-128); if(b1>255)b1=255; if(b1<0)b1=0;

	    // r2 = Y1+1.4075* (V1-128) ;if(r2>255)r2=255; if(r2<0)r2=0;
	    // g2 = Y1- 0.3455 *(U0-128) - 0.7169*(V1-128); if(g2>255)g2=255;if(g2<0)g2=0;
        // b2 = Y1+ 1.779 * (U0-128);if(b2>255)b2=255;if(b2<0)b2=0;

        // rgbdata[i * 2 + 0] = ((r1&0x1F) << 3) | ((g1&0xE0) >> 5);
        // rgbdata[i * 2 + 1] = ((g1&0x07) << 5) | (b1&0x1F);
        // rgbdata[i] = rgbIII_to_bgr565(r1,g1,b1);
        
        *(rgbdata++) = (((g1 & 0x1c) << 3) | (b1 >> 3));/* g低5位，b高5位 */
        *(rgbdata++) = ((r1 & 0xf8) | (g1 >> 5));	    /* r高5位，g高3位 */

    }
}

/*
* YUV422打包数据,UYVY,转换为RGB565,
* inBuf -- YUV data
* outBuf -- RGB565 data
* imgWidth,imgHeight -- image width and height
*/
int convert_uyvy_to_rgb565(unsigned char *inBuf, unsigned char *outBuf, int imgWidth, int imgHeight)
{
	int rows ,cols;	/* 行列标志 */
	int y, u, v, r, g, b;	/* yuv rgb 相关分量 */
	unsigned char *YUVdata, *RGBdata;	/* YUV和RGB数据指针 */
	int Ypos, Upos, Vpos;	/* Y U V在数据缓存中的偏移 */
	unsigned int i = 0;

	YUVdata = inBuf;
	RGBdata = outBuf;
#if 0
	/* YVYU */
	Ypos = 0;
	Vpos = Ypos + 1;
	Upos = Vpos + 2;

    /* UYVY */
	Ypos = 1;
	Upos = Ypos - 1;
	Vpos = Ypos + 1;
#endif

#if 1   
    /*  YUYV */
	Ypos = 0;
	Upos = Ypos + 1;
	Vpos = Upos + 2;
#endif

	/* 每个像素两个字节 */
	for(rows = 0; rows < imgHeight; rows++)
	{
		for(cols = 0; cols < imgWidth; cols++)
		{
			/* 矩阵推到，百度 */
			y = YUVdata[Ypos];
			u = YUVdata[Upos] - 128;
			v = YUVdata[Vpos] - 128;

			r = y + v + ((v * 103) >> 8);
			g = y - ((u * 88) >> 8) - ((v * 183) >> 8);
			b = y + u + ((u * 198) >> 8);

			r = r > 255?255:(r < 0?0:r);
			g = g > 255?255:(g < 0?0:g);
			b = b > 255?255:(b < 0?0:b);

			/* 从低到高r g b */
			// *(RGBdata ++) = (((g & 0x1c) << 3) | (b >> 3));	/* g低5位，b高5位 */
			// *(RGBdata ++) = ((r & 0xf8) | (g >> 5));	/* r高5位，g高3位 */
            // 从低到高 b g r
            *(RGBdata ++) = (((g & 0x1c) << 3) | (r >> 3));	/* g低5位，r高5位 */
			*(RGBdata ++) = ((b & 0xf8) | (g >> 5));	    /* b高5位，g高3位 */

			/* 两个字节数据中包含一个Y */
			Ypos += 2;
			//Ypos++;
			i++;
			/* 每两个Y更新一次UV */
			if(!(i & 0x01))	
			{
				Upos = Ypos - 1;
				Vpos = Ypos + 1;
			}

		}
	}

	return 0;
}



static int v4l2_set_format(void)
{
    int err = 0;
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm streamparm = {0};

    /* 设置帧格式 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//type类型
    fmt.fmt.pix.width = width;  //视频帧宽度
    fmt.fmt.pix.height = height;//视频帧高度
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  //像素格式
    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt)) {
        fprintf(stderr, "ioctl error: VIDIOC_S_FMT: %s\n", strerror(errno));
        return -1;
    }

    /*** 判断是否已经设置为我们要求的RGB565像素格式
    如果没有设置成功表示该设备不支持RGB565像素格式 */
    if (V4L2_PIX_FMT_YUYV != fmt.fmt.pix.pixelformat) {
        fprintf(stderr, "Error: the device does not support YUV422 format!    now format=0x%x\n", fmt.fmt.pix.pixelformat);
        return -1;
    }

    frm_width = fmt.fmt.pix.width;  //获取实际的帧宽度
    frm_height = fmt.fmt.pix.height;//获取实际的帧高度
    printf("视频帧大小<%d * %d>\n", frm_width, frm_height);

    /* 获取streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    err = ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm);

    /** 判断是否支持帧率设置 **/
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 15;//30fps
        if (0 > ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm)) {
            fprintf(stderr, "ioctl error: VIDIOC_S_PARM: %s\n", strerror(errno));
            return -1;
        }
    }

    err = ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm);
    if(!err){
        printf("get VIDIOC_G_PARM success\n");
        printf("      numerator:%d , denominator:%d\r\n", streamparm.parm.capture.timeperframe.numerator, \
                                                        streamparm.parm.capture.timeperframe.denominator);
    }

    return err;
}

static int v4l2_init_buffer(void)
{
    struct v4l2_requestbuffers reqbuf = {0};
    struct v4l2_buffer buf = {0};

    /* 申请帧缓冲 */
    reqbuf.count = FRAMEBUFFER_COUNT;       //帧缓冲的数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (0 > ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf)) {
        fprintf(stderr, "ioctl error: VIDIOC_REQBUFS: %s\n", strerror(errno));
        return -1;
    }

    /* 建立内存映射 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {

        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);
        buf_infos[buf.index].length = buf.length;
        printf("v4l2_init_buffer:  buf_infos[%d].length = %d\r\n", buf.index, buf_infos[buf.index].length);
        buf_infos[buf.index].start = mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                v4l2_fd, buf.m.offset);
        if (MAP_FAILED == buf_infos[buf.index].start) {
            perror("mmap error 2");
            return -1;
        }
    }

    /* 入队 */
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {

        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf)) {
            fprintf(stderr, "ioctl error: VIDIOC_QBUF: %s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

static int v4l2_stream_on(void)
{
    /* 打开摄像头、摄像头开始采集数据 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 > ioctl(v4l2_fd, VIDIOC_STREAMON, &type)) {
        fprintf(stderr, "ioctl error: VIDIOC_STREAMON: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/*
 * @func    :scale_image
 * @desp    :对图像进行缩放
*/
static void scale_image(unsigned short* Buf, int src_width, int src_height, int scale_width, int scale_height)
{
    int i, j;
    double scale_n = (scale_width * scale_height)/(src_width * scale_height);
    for(i=0; i<src_height*scale_n; i++){
        for(j=0; j<src_width*scale_n; j++){
            if(i<scale_width && j<scale_height){
                Buf[i*scale_height+j] = Buf[src_width*(src_height-1-(int)(i/scale_n))+(int)(j/scale_n)];
            }
        }
    }
}

static void v4l2_read_data(void)
{
    struct v4l2_buffer buf = {0};
    unsigned short rgbdata[480 * 272];
    unsigned short *base;
    unsigned short *start;
    int min_w, min_h;
    int j;
    int rgbdataIdx = 0;

    if (width > frm_width)      //如果LCD的宽度大于视频帧宽度
        min_w = frm_width;      //取最小的
    else
        min_w = width;
    if (height > frm_height)
        min_h = frm_height;
    else
        min_h = height;

    printf("width=%d,   frm_width=%d\r\n", width, frm_width);
    printf("height=%d,   frm_height=%d\r\n", height, frm_height);
    printf("min_w=%d,   min_h=%d\r\n", min_w, min_h);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    // printf("*************%s %d \r\n", __FUNCTION__, __LINE__);
    for ( ; ; ) {

        for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {

            ioctl(v4l2_fd, VIDIOC_DQBUF, &buf);     //出队
            //视频格式转换
            // yuyv_to_rgb565((unsigned char *)(buf_infos[buf.index].start), \
                             (unsigned char *)(rgbdata), frm_width, frm_height); 
            convert_uyvy_to_rgb565((unsigned char *)(buf_infos[buf.index].start), \
                                    (unsigned char *)(rgbdata), frm_width, frm_height);
            
            //循环min_h次有问题，如果只是要显示摄像头的数据，只刷新摄像头数据高度即可。
            for (j = 0, base=screen_base, start=buf_infos[buf.index].start, rgbdataIdx = 0;
                        j < min_h; j++) {
            
                memcpy((unsigned char*)base, (unsigned char*)(&rgbdata[rgbdataIdx]), frm_width*2);
                base += width;  //LCD显示指向下一行
                rgbdataIdx += frm_width;//指向下一行数据

            }
            // 数据处理完之后、再入队、往复
            ioctl(v4l2_fd, VIDIOC_QBUF, &buf);
        }
    }
}

int main(int argc, char *argv[])
{
    if (2 != argc) {
        fprintf(stderr, "Usage: %s <video_dev>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 初始化LCD */
    if (fb_dev_init())
        exit(EXIT_FAILURE);

    /* 初始化摄像头 */
    if (v4l2_dev_init(argv[1]))
        exit(EXIT_FAILURE);

    /* 枚举所有格式并打印摄像头支持的分辨率及帧率 */
    v4l2_enum_formats();
    v4l2_print_formats();

    /* 设置格式 */
    if (v4l2_set_format())
        exit(EXIT_FAILURE);

    /* 初始化帧缓冲：申请、内存映射、入队 */
    if (v4l2_init_buffer())
        exit(EXIT_FAILURE);

    /* 开启视频采集 */
    if (v4l2_stream_on())
        exit(EXIT_FAILURE);

    /* 读取数据：出队 */
    v4l2_read_data();       //在函数内循环采集数据、将其显示到LCD屏

    exit(EXIT_SUCCESS);
}
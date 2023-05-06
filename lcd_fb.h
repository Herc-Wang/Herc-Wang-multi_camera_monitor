#ifndef __LCD_FB_H__
#define __LCD_FB_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "v4l2_camera.h"


#define FB_DEV              "/dev/fb0"      //LCD设备节点

//LCD设备参数的结构体
typedef struct {
    int lcd_width;                      //LCD宽度
    int lcd_height;                     //LCD高度
    unsigned short *lcd_screen_base;    //LCD显存基地址
    int fb_fd;                          //LCD设备文件描述符
    unsigned long lcd_screen_size;      //LCD屏幕大小
}LCD_fb_DEV;

//屏幕显示位置索引
typedef enum {
    SCREEN_INDEX_LeftTop,
    SCREEN_INDEX_RightTop,
    SCREEN_INDEX_LeftBottom,
    SCREEN_INDEX_RightBottom,
}SCREEN_INDEX;

extern LCD_fb_DEV g_LCD_fb_dev;

void lcd_fb_dev_init(void);
void lcd_clear_to_white(LCD_fb_DEV LCD_fb_dev);
void updateLCD(LCD_fb_DEV* LCD_fb_dev, int lcdWidth, int lcdHeight, int startX, int startY, \
                unsigned short *baseAddrBuf,int updateWidth, int updateHeight);
#endif // !__LCD_FB_H__
#include "lcd_fb.h"

/************全局变量区*************/
//全局LCD参数结构体
LCD_fb_DEV g_LCD_fb_dev;


/**
 * @brief fb_dev_init - 初始化framebuffer设备，包括获取设备信息，内存映射等操作。
 *
 * @param LCD_fb_dev   - 指向全局LCD设备的结构体的指针。
 * @return int - 返回0表示成功，返回其他值表示失败。
 */
static int fb_dev_init(LCD_fb_DEV* LCD_fb_dev)
{
    int fb_fd;
    #ifdef _DEBUG_ 
        printf("\tdebug: %s\n", __FUNCTION__);
    #endif

    // if(camera_param->screen_index == 0){
        struct fb_var_screeninfo fb_var = {0};
        struct fb_fix_screeninfo fb_fix = {0};
        
        /* 打开framebuffer设备 */
        fb_fd = open(FB_DEV, O_RDWR);
        LCD_fb_dev->fb_fd = fb_fd;
        if (0 > fb_fd) {
            fprintf(stderr, "open error: %s: %s\n", FB_DEV, strerror(errno));
            return -1;
        }

        /* 获取framebuffer设备信息 */
        ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_var);
        ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_fix);

        LCD_fb_dev->lcd_screen_size = fb_fix.line_length * fb_var.yres;
        // camera_param->screen_size = LCD_fb_dev->lcd_screen_size;
        LCD_fb_dev->lcd_width = fb_var.xres;
        LCD_fb_dev->lcd_height = fb_var.yres;
        // printf("screen_size:%lu   fb_var  width=%d,    height=%d   line_length=%d \r\n", screen_size, width, height, fb_fix.line_length);

        /* 内存映射 */
        LCD_fb_dev->lcd_screen_base = mmap(NULL, LCD_fb_dev->lcd_screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
        if (MAP_FAILED == (void *)LCD_fb_dev->lcd_screen_base) {
            perror("mmap error 1");
            close(fb_fd);
            return -1;
        }

        /* LCD背景刷白 */
        lcd_clear_to_white(*LCD_fb_dev);
    // }
    return 0;
}

/**
 * @brief fb_dev_deinit - 销毁framebuffer设备，包括取消内存映射等操作。
 *
 * @param LCD_fb_dev   - 指向全局LCD设备的结构体的指针。
 * @return int - 返回0表示成功，返回其他值表示失败。
 */
static void fb_dev_deinit(LCD_fb_DEV* LCD_fb_dev)
{
    munmap(LCD_fb_dev->lcd_screen_base, LCD_fb_dev->lcd_screen_size);
    close(LCD_fb_dev->fb_fd);
}

/**
 * @brief lcd_fb_dev_init - 初始化全局LCD参数结构体，并初始化LCD
 *
 * @param   
 * @return void
 */
void lcd_fb_dev_init(void)
{
    // LCD_fb_DEV* lcd_dev = (struct LCD_fb_DEV*)malloc(sizeof(LCD_fb_DEV));
    // memset(lcd_dev, 0, sizeof(LCD_fb_DEV));
    g_LCD_fb_dev.lcd_width = 0;                              //LCD宽度
    g_LCD_fb_dev.lcd_height = 0;                             //LCD高度
    g_LCD_fb_dev.lcd_screen_base = NULL;     //LCD显存基地址
    g_LCD_fb_dev.fb_fd = -1;                             //LCD设备文件描述符
    g_LCD_fb_dev.lcd_screen_size = 0;          //LCD屏幕大小
    
    fb_dev_init(&g_LCD_fb_dev);
    // return lcd_dev;
}

/**
 * @brief lcd_clear_to_white - LCD屏幕刷白
 *
 * @param  LCD_fb_dev- 指向全局LCD设备的结构体的指针。
 * @return void
 */
void lcd_clear_to_white(LCD_fb_DEV LCD_fb_dev)
{
    /* LCD背景刷白 */
    printf("**************LCD clear to white**************\n");    
    memset(LCD_fb_dev.lcd_screen_base, 0xFF, LCD_fb_dev.lcd_screen_size);
}

/**
 * @brief updateLCD - LCD更新函数
 *
 * @param LCD_fb_dev   - 指向全局LCD设备的结构体的指针。
 * @param lcdWidth      lcd的屏幕的宽高
 * @param lcdHeight     
 * @param startX        更新的lcd区域的左上角点位置
 * @param startY        
 * @param baseAddrBuf   需要写到lcd的图像基地址
 * @param updateWidth   需要更新的宽高
 * @param updateHeight  
 *
 * @note The scaled image will be stored in the 'scale_buf' buffer.
 */
void updateLCD(LCD_fb_DEV* LCD_fb_dev, int lcdWidth, int lcdHeight, int startX, int startY, \
                unsigned short *baseAddrBuf,int updateWidth, int updateHeight) {
    // 根据起始地址计算偏移量
    unsigned short *base;
    int j, bufIndex;
    unsigned short * baseAddr = LCD_fb_dev->lcd_screen_base;

    #ifdef _DEBUG_
        printf("\tdebug: %s\n", __FUNCTION__);
    #endif

    if(lcdHeight < startY+updateHeight)
        perror("lcd update hight param err");
    if(lcdWidth < startX+updateWidth)
        perror("lcd update width param err");
    

    //循环min_h次有问题，如果只是要显示摄像头的数据，只刷新摄像头数据高度即可。
    for (j = 0, base=(baseAddr+startY*lcdWidth+startX), bufIndex = 0;j < updateHeight; j++) {
    
        memcpy((unsigned char*)base, (unsigned char*)(&baseAddrBuf[bufIndex]), updateWidth*2);
        base += LCD_fb_dev->lcd_width;  //LCD显示指向下一行
        bufIndex += updateWidth;//指向下一行数据
    }
}


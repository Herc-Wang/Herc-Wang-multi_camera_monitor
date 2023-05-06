#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "push_steam.h"

/**
 * @brief thread_push_stream - 推流线程，通过system指令来调用ffmpeg进行推流
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
    while(1);
    return NULL;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libudev.h>
#include <sys/select.h>
#include <pthread.h>
#include "usb_monitor.h"
#include "camera_manager_thread.h"


/**
 * @brief print_device_info - 打印给定USB设备的设备信息
 * 
 * @param dev 要打印信息的USB设备的udev_device指针
 * @return void
*/
void print_device_info(struct udev_device *dev) {
    // 获取设备信息
    const char *devpath   = udev_device_get_devpath(dev);
    const char *subsystem = udev_device_get_subsystem(dev);
    const char *devtype   = udev_device_get_devtype(dev);
    const char *syspath   = udev_device_get_syspath(dev);
    const char *sysname   = udev_device_get_sysname(dev);
    const char *sysnum    = udev_device_get_sysnum(dev);
    const char *devnode   = udev_device_get_devnode(dev);

    // 打印设备信息
    printf("Device info:\n");
    printf("  devpath   : %s\n", devpath  );
    printf("  subsystem : %s\n", subsystem);
    printf("  devtype   : %s\n", devtype  );
    printf("  syspath   : %s\n", syspath  );
    printf("  sysname   : %s\n", sysname  );
    printf("  sysnum    : %s\n", sysnum   );
    printf("  devnode   : %s\n", devnode  );
    printf("\n");
}

/**
 * @brief enumerate_usb_devices - 枚举所有USB设备并打印设备信息
 * 
 * @param udev 指向udev context的指针
 * @return 无返回值
*/
void enumerate_usb_devices(struct udev *udev) {
    // 创建枚举器
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    // 设置匹配条件
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    // 扫描设备
    udev_enumerate_scan_devices(enumerate);

    // 获取设备列表
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *dev_list_entry;

    // 遍历设备列表
    udev_list_entry_foreach(dev_list_entry, devices) {
        // 获取设备路径
        const char *path = udev_list_entry_get_name(dev_list_entry);
        // 创建设备对象
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        if (dev) {
            // 打印设备信息
            print_device_info(dev);
            // 释放设备对象
            udev_device_unref(dev);
        }
    }

    // 释放枚举器
    udev_enumerate_unref(enumerate);
}

/**
 * @brief thread_usb_monitor - usb监控线程，用于监测是否有usb设备连接断开
 *
 * @param arg 
 * @note 
 */
void *thread_usb_monitor(void *arg){
    // 创建一个udev对象
    struct udev *udev = udev_new();
    int i;

    if (!udev) {
        // 创建失败
        fprintf(stderr, "Failed to create udev context\n");
        return NULL;
    }

    // 枚举所有USB设备并打印它们的信息
    // enumerate_usb_devices(udev);

    // 创建udev monitor对象，并设置要监听的设备类型为usb
    struct udev_monitor *monitor = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", NULL);

    // 启用udev monitor接收事件
    udev_monitor_enable_receiving(monitor);

    // 获取monitor的文件描述符
    int fd = udev_monitor_get_fd(monitor);
    fd_set fds;

    // 进入循环，等待事件发生
    while (1) {
        // 清空文件描述符集合并将monitor的文件描述符添加到集合中
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        // 调用select函数等待文件描述符就绪
        int ret = select(fd + 1, &fds, NULL, NULL, NULL);

        if (ret > 0 && FD_ISSET(fd, &fds)) {
            // 从monitor中接收一个事件对象
            struct udev_device *dev = udev_monitor_receive_device(monitor);

            if (dev) {
                // 获取事件动作（add或remove）
                const char *action = udev_device_get_action(dev);

                //如果有usb设备add或remove，都释放信号
                if (strcmp(action, "add") == 0 || strcmp(action, "remove") == 0) {
                    // printf("debug: fd=%d USB device added/remove\n", fd);       //debug
                    pthread_mutex_lock(&mutex);
                    // for(i=0; i<global_camera_data->vaild_camera_num; i++){
                    //     printf("pthread_cancel: cancel camera thread[%d]\r\n", i);
                    //     pthread_cancel(g_thread_usb_camera[i]);       //父线程主动发起取消请求
                    // }
                    pthread_cond_signal(&usb_cond);
                    printf("debug: -----pthread_cond_signal-----\n");
                    pthread_mutex_unlock(&mutex);
                }
                // if (strcmp(action, "add") == 0) {
                //     // 如果是add事件，打印提示信息并枚举所有USB设备
                //     printf("USB device added\n");
                //     // enumerate_usb_devices(udev);
                // }
                // else if (strcmp(action, "remove") == 0) {
                //     // 如果是remove事件，打印提示信息即可
                //     printf("USB device removed\n");
                // }

                // 释放事件对象
                udev_device_unref(dev);
            }
        }
    }

    // 关闭monitor和udev对象
    udev_monitor_unref(monitor);
    udev_unref(udev);
    return NULL;
}
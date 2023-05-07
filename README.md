# [multi_camera_monitor(多摄像头监控)](https://github.com/Herc-Wang/Herc-Wang-multi_camera_monitor)

## 项目简介
>基于linux C+ffmpeg实现的多摄像头图传项目，在imx6ull平台上，实现多个摄像头视频显示到LCD屏幕，通过ffmpeg将LCD数据进行rtsp/udp推流。
>![结构图](/IMG/结构图.png)

## 目录结构

| 文件          | 描述 |
| -----------                   | ----------- |
| **IMG/**                      | 存放readme.md中使用到的图片|
| **Makefile**                  |编译脚本|
|**multi_camera_monitor.c**     |创建并启动usb监控线程，推流线程，摄像头捕获线程|
|**camera_manager_thread.c**    |摄像头管理线程文件|
|**global_lock.c**              |全局锁、条件变量的定义文件|
|**lcd_fb.c**                   |lcd相关api|
|**push_steam.c**               |推流线程文件|
|**usb_monitor.c**              |usb监控线程文件|
|**v4l2_camera.c**              |摄像头子线程文件|

## 功能特性
1、开机动态监测当前有多少可用摄像头，自动打开最大数量为4的摄像头并显示到lcd上。

2、热拔插usb摄像头设备时，自动监测摄像头的插入与拔出，更新lcd屏幕上摄像头的打开与重布局。

3、动态适配不同尺寸lcd，实现一分四的监控界面。不同区域显示不同摄像头的数据。

4、摄像头数据与lcd可用区域大小显然不匹配，代码中首先对摄像头设置帧格式为最接近lcd可用区域大小的配置，并且使用软件缩放对原始摄像头数据进行动态缩放。

5、可以通过指定推流地址，目前是通过程序里面的宏进行修改，方便开发，后续开发完善后修改为通过命令行配置。

6、运行multi_camera_monitor的嵌入式设备只作为推流端，只对lcd数据作为视频流推送到流媒体服务器上。因此需要局域网或者连接设备内存在流媒体服务器，拉流端从流媒体服务器上拉取视频流。（流媒体服务器和拉流端可为一体，如PC等设备）

## 硬件依赖
* **硬件平台**：正点原子IMX6ULL
* **摄像头型号**：ANC狼魔35501、乐视三合一体感摄像头、WXSJ-H65HD V01。（均为YUV格式摄像头）


## 环境依赖
* **内核支持**：usb、v4l2、udev
* **软件支持**：
    - 交叉编译ffmpeg  ([参考连接：imx6ull: 从内核、buildroot配置实现ffmpeg+nginx+rtmp+USB摄像头](https://blog.csdn.net/weixin_36432129/article/details/128430809?ydreferer=aHR0cHM6Ly9tcC5jc2RuLm5ldC9tcF9ibG9nL21hbmFnZS9hcnRpY2xlP3NwbT0xMDAwLjIxMTUuMzAwMS41NDQ4))
    - 交叉编译libusb、libudev、eudev ([参考连接：imx6ull开发板环境配置 - libusb、libudev、eudev交叉编译](https://blog.csdn.net/weixin_36432129/article/details/130442076))


## 编译步骤
1.拉取代码
```
git clone https://github.com/Herc-Wang/Herc-Wang-multi_camera_monitor.git
```

2、根据本地环境修改交叉编译工具链

在Makefile脚本里面，有一个cp指令，用于将编译好的文件放到nfs制定目录中。需要自行修改为指定路径。
```
cd Herc-Wang-multi_camera_monitor
vim Makefile
```
![修改Makefile指示图](/IMG/修改Makefile指示图.png "修改Makefile指示图")

3、编译代码
```
make
```
编译完成后将可执行文件放到开发板中

4、运行

## 拉流步骤
>本项目中使用PC同时作为nginx流媒体服务器以及拉流端，因此在使用时需要先确保pc与嵌入式设备处于同一局域网内，或保持网络联通。

1、流媒体服务器搭建

使用nginx作为流媒体服务器，这部分工作网络上已经有很多教程，这里自行搭建不再赘述。
([参考连接：Ubuntu下安装、编译、运行nginx和nginx-rtmp-module](https://blog.csdn.net/u014552102/article/details/86599289))

2、修改推流路径

原代码multi_camera_monitor.c文件中通过PUSH_STREAM_ADDR宏指定推流路径，可根据自身设备进行修改。
原推流路径为：udp://192.168.124.10:1244

3、开始拉流

- 确保嵌入式设备运行multi_camera_monitor
- pc设备上开始拉流``` ffplay udp://192.168.124.10:1244 ```


>### 拉流设备视频界面显示
>![拉流设备视频界面显示](/IMG/Image1.png "拉流设备视频界面显示")

>### 嵌入式设备界面显示
>![嵌入式设备界面显示](/IMG/Image2.png "嵌入式设备界面显示")

## 协议
GPLv2

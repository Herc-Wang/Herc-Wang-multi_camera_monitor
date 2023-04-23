#ARCH=arm64
CROSS=arm-linux-gnueabihf-
#GCC=${CC}
all: multi_camera_monitor
	cp -f multi_camera_monitor ~/linux/nfs/rootfs-alientek/
v4l2_camera:v4l2_camera.c
	$(CROSS)gcc  -o v4l2_camera v4l2_camera.c 

# v4l2_camera_monitor:v4l2_camera_monitor.c
# 	$(CROSS)gcc  -o v4l2_camera v4l2_camera.c 

multi_camera_monitor:
	$(CROSS)gcc  -o multi_camera_monitor multi_camera_monitor.c v4l2_camera.c -lpthread

clean:
	@rm -vf multi_camera_monitor *.o *~


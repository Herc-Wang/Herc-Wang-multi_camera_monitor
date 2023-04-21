#ARCH=arm64
CROSS=arm-linux-gnueabihf-
#GCC=${CC}
all: v4l2_camera
	cp -f v4l2_camera ~/linux/nfs/rootfs-alientek/
v4l2_camera:v4l2_camera.c
	$(CROSS)gcc  -o v4l2_camera v4l2_camera.c 
#	$(CROSS)strip v4l2_camera
clean:
	@rm -vf v4l2_camera *.o *~


CROSS=arm-linux-gnueabihf-
CC = $(CROSS)gcc
CFLAGS =	-lpthread -lusb-1.0 -ludev \
			-I/home/alientek/tools/libusb-1.0.26/build/include	\
			-I/home/alientek/tools/eudev-3.2.11/build/include

LDFLAGS = -L/home/alientek/tools/libusb-1.0.26/build/lib \
			-L/home/alientek/tools/eudev-3.2.11/build/lib


TARGET = multi_camera_monitor
OBJS = multi_camera_monitor.o v4l2_camera.o usb_monitor.o \
		global_lock.o push_steam.o camera_manager_thread.o \
		lcd_fb.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	cp -f ${TARGET} ~/linux/nfs/rootfs-alientek/

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)


# Trình biên dịch mặc định cho host (máy ảo x86 của anh)
CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -lrt

# Trình biên dịch cho ARM QEMU
ARM_CC = arm-linux-gnueabihf-gcc

# Tên file thực thi đầu ra
HOST_BIN = sms_app_x86
ARM_BIN = sms_app

# Danh sách các file mã nguồn
SRCS = sms_app.c sensor_thread.c processor_thread.c alert_thread.c logger_thread.c circular_buffer.c

all: host

# Build cho máy chạy thử (x86)
host:
	$(CC) $(CFLAGS) $(SRCS) -o $(HOST_BIN) $(LDFLAGS)

# Build cho thiết bị nhúng ARM
arm:
	$(ARM_CC) $(CFLAGS) $(SRCS) -o $(ARM_BIN) $(LDFLAGS)

clean:
	rm -f $(HOST_BIN) $(ARM_BIN) /tmp/sms.log*

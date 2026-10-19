/*
 * File: asgn1_ioctl.h
 * Author: Tran Gia Phuc - SE204417
 * Description: Shared header file between Kernel Space (asgn1_driver)
 *              and User Space for ioctl command definitions and data structures.
 */

#ifndef ASGN1_IOCTL_H
#define ASGN1_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#endif

/*
 * Magic Number ('A' = 0x41) giúp phân biệt tập lệnh ioctl của asgn1_driver
 * với các thiết bị khác trong hệ thống, ngăn chặn lỗi gửi nhầm lệnh
 * vào sai file descriptor.
 */
#define ASGN1_IOC_MAGIC        'A'

/* Các chế độ hoạt động của driver cho lệnh ASGN1_SET_MODE */
#define ASGN1_MODE_KERNEL      0   /* Tự động thêm tiền tố "[KERNEL] " khi write */
#define ASGN1_MODE_ECHO        1   /* Lưu dữ liệu nguyên bản (Mặc định) */

/* Kích thước tối đa của chuỗi version trả về cho userspace */
#define ASGN1_VERSION_MAX_LEN  64

/*
 * Cấu trúc thống kê hoạt động của driver (dùng cho ASGN1_GET_STATS).
 * Chứa đầy đủ 5 trường bắt buộc theo yêu cầu đề bài.
 */
struct asgn1_stats {
    int open_count;       /* Tổng số lần thiết bị được mở */
    int write_count;      /* Số lần ghi (reset về 0 khi gọi ASGN1_RESET_BUFFER) */
    int read_count;       /* Số lần đọc (reset về 0 khi gọi ASGN1_RESET_BUFFER) */
    int buffer_len;       /* Độ dài dữ liệu hiện tại trong buffer (bytes) */
    int last_write_size;  /* Kích thước dữ liệu của lần write gần nhất (bytes) */
};

/*
 * Định nghĩa 4 lệnh ioctl bằng các macro chuẩn của Linux Kernel:
 * 1. ASGN1_RESET_BUFFER : Không truyền tham số (_IO)
 * 2. ASGN1_GET_STATS    : Đọc struct asgn1_stats từ kernel về userspace (_IOR)
 * 3. ASGN1_SET_MODE     : Ghi cấu hình mode (kiểu int) từ userspace xuống kernel (_IOW)
 * 4. ASGN1_GET_VERSION  : Đọc chuỗi version (mảng 64 bytes) từ kernel về userspace (_IOR)
 */
#define ASGN1_RESET_BUFFER  _IO(ASGN1_IOC_MAGIC, 1)
#define ASGN1_GET_STATS     _IOR(ASGN1_IOC_MAGIC, 2, struct asgn1_stats)
#define ASGN1_SET_MODE      _IOW(ASGN1_IOC_MAGIC, 3, int)
#define ASGN1_GET_VERSION   _IOR(ASGN1_IOC_MAGIC, 4, char[ASGN1_VERSION_MAX_LEN])

/* Giới hạn số thứ tự lệnh tối đa để kiểm tra tính hợp lệ nếu cần */
#define ASGN1_IOC_MAXNR     4

#endif /* ASGN1_IOCTL_H */

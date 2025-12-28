#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/init.h>

#define MAX_SEATS 5
#define MAX_INFO_LEN 32

// 共享数据结构
struct reading_room_data {
    int seat_count;                    // 当前座位数
    int reader_count;                  // 当前读者数
    char reader_info[MAX_SEATS][MAX_INFO_LEN]; // 读者信息
    struct semaphore seat_sem;         // 座位信号量
    struct semaphore mutex;            // 互斥信号量
    struct semaphore info_mutex;       // 个人信息互斥
};

static struct reading_room_data *room_data = NULL;

// 初始化阅览室
static int __init init_reading_room(void)
{
    room_data = kmalloc(sizeof(struct reading_room_data), GFP_KERNEL);
    if (!room_data)
        return -ENOMEM;
    
    room_data->seat_count = MAX_SEATS;
    room_data->reader_count = 0;
    
    sema_init(&room_data->seat_sem, MAX_SEATS);  // 初始5个座位
    sema_init(&room_data->mutex, 1);             // 互斥锁
    sema_init(&room_data->info_mutex, 1);        // 信息互斥
    
    memset(room_data->reader_info, 0, sizeof(room_data->reader_info));
    
    printk(KERN_INFO "Reading room initialized with %d seats\n", MAX_SEATS);
    return 0;
}

// 读者注册系统调用 - 修正了座位分配逻辑
SYSCALL_DEFINE2(reader_register, char __user *, user_info, int, info_len)
{
    int seat_id = -1;
    int i;
    char info[MAX_INFO_LEN];
    
    if (info_len > MAX_INFO_LEN || info_len <= 0)
        return -EINVAL;
    
    // 复制用户空间信息
    if (copy_from_user(info, user_info, info_len))
        return -EFAULT;
    info[info_len] = '\0';
    
    // 获取座位
    if (down_interruptible(&room_data->seat_sem))
        return -ERESTARTSYS;
    
    // 互斥访问
    if (down_interruptible(&room_data->mutex))
        return -ERESTARTSYS;
    
    // 查找第一个空座位
    for (i = 0; i < MAX_SEATS; i++) {
        if (room_data->reader_info[i][0] == '\0') {
            seat_id = i;
            break;
        }
    }
    
    // 理论上不应该发生，因为有信号量保护
    if (seat_id == -1) {
        up(&room_data->mutex);
        up(&room_data->seat_sem);
        return -EBUSY;
    }
    
    // 互斥写入个人信息
    if (down_interruptible(&room_data->info_mutex))
        return -ERESTARTSYS;
    
    strncpy(room_data->reader_info[seat_id], info, MAX_INFO_LEN-1);
    room_data->reader_info[seat_id][MAX_INFO_LEN-1] = '\0';
    room_data->reader_count++;  // 增加读者计数
    
    up(&room_data->info_mutex);
    up(&room_data->mutex);
    
    printk(KERN_INFO "Reader registered: %s, Seat: %d\n", info, seat_id);
    return seat_id;  // 返回实际的座位号（0-4）
}

// 读者注销系统调用
SYSCALL_DEFINE1(reader_unregister, int, seat_id)
{
    if (seat_id < 0 || seat_id >= MAX_SEATS)
        return -EINVAL;
    
    // 检查座位是否已被占用
    if (down_interruptible(&room_data->info_mutex))
        return -ERESTARTSYS;
    
    // 如果座位是空的，直接返回
    if (room_data->reader_info[seat_id][0] == '\0') {
        up(&room_data->info_mutex);
        return 0;  // 座位本来就是空的，不需要操作
    }
    
    printk(KERN_INFO "Reader unregistered: %s, Seat: %d\n", 
           room_data->reader_info[seat_id], seat_id);
    memset(room_data->reader_info[seat_id], 0, MAX_INFO_LEN);
    
    up(&room_data->info_mutex);
    
    // 互斥更新读者计数
    if (down_interruptible(&room_data->mutex))
        return -ERESTARTSYS;
    
    room_data->reader_count--;
    if (room_data->reader_count < 0) {
        room_data->reader_count = 0;  // 防止负数
    }
    
    up(&room_data->mutex);
    
    // 释放座位
    up(&room_data->seat_sem);
    
    return 0;
}

// 获取阅览室状态 - 完全重写修复所有问题
SYSCALL_DEFINE3(get_room_status, char __user *, buffer, int, buf_size, int __user *, reader_count)
{
    int i, total_len = 0;
    char *kbuf;
    int current_reader_count = 0;
    int occupied_seats = 0;
    
    if (!room_data)
        return -ENODEV;
    
    if (buf_size <= 0)
        return -EINVAL;
    
    kbuf = kmalloc(buf_size, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;
    
    if (down_interruptible(&room_data->mutex)) {
        kfree(kbuf);
        return -ERESTARTSYS;
    }
    
    // 获取当前的读者计数
    current_reader_count = room_data->reader_count;
    
    // 先复制读者计数到用户空间（如果指针有效）
    if (reader_count) {
        if (copy_to_user(reader_count, &current_reader_count, sizeof(int))) {
            up(&room_data->mutex);
            kfree(kbuf);
            return -EFAULT;
        }
    }
    
    // 构建状态信息 - 使用正确的读者计数
    total_len += snprintf(kbuf + total_len, buf_size - total_len, 
                         "Reading Room Status:\n");
    total_len += snprintf(kbuf + total_len, buf_size - total_len,
                         "Total seats: %d, Available: %d\n",
                         MAX_SEATS, MAX_SEATS - current_reader_count);
    total_len += snprintf(kbuf + total_len, buf_size - total_len,
                         "Current readers (%d):\n", current_reader_count);
    
    if (down_interruptible(&room_data->info_mutex)) {
        up(&room_data->mutex);
        kfree(kbuf);
        return -ERESTARTSYS;
    }
    
    // 显示所有有读者的座位
    for (i = 0; i < MAX_SEATS; i++) {
        if (room_data->reader_info[i][0] != '\0') {
            occupied_seats++;
            total_len += snprintf(kbuf + total_len, buf_size - total_len,
                                "  Seat %d: %s\n", i, room_data->reader_info[i]);
        }
    }
    
    // 如果统计的已占座位数与读者计数不一致，修复它
    if (occupied_seats != current_reader_count) {
        printk(KERN_WARNING "Reading room: reader_count (%d) != occupied_seats (%d), fixing...\n",
               current_reader_count, occupied_seats);
        
        // 更新读者计数
        room_data->reader_count = occupied_seats;
        current_reader_count = occupied_seats;
        
        // 如果用户需要，更新用户空间的计数
        if (reader_count) {
            copy_to_user(reader_count, &current_reader_count, sizeof(int));
        }
    }
    
    up(&room_data->info_mutex);
    up(&room_data->mutex);
    
    // 复制状态信息到用户空间
    if (copy_to_user(buffer, kbuf, total_len + 1)) {
        kfree(kbuf);
        return -EFAULT;
    }
    
    kfree(kbuf);
    return total_len;
}

__initcall(init_reading_room);
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/init.h>

#define MAX_SEATS 5
#define MAX_NAME_LEN 32
#define MAX_PHONE_LEN 16

// 读者信息结构体
struct reader_info {
    char name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    int valid;  // 标记该座位是否被占用
};

// 共享数据结构
struct reading_room_data {
    int seat_count;                    // 当前座位数
    int reader_count;                  // 当前读者数
    struct reader_info readers[MAX_SEATS];  // 读者信息数组
    struct semaphore seat_sem;         // 座位信号量
    struct semaphore mutex;            // 互斥信号量
    struct semaphore info_mutex;       // 个人信息互斥
};

static struct reading_room_data *room_data = NULL;

// 初始化阅览室
static int __init init_reading_room(void)
{
    int i;
    
    room_data = kmalloc(sizeof(struct reading_room_data), GFP_KERNEL);
    if (!room_data)
        return -ENOMEM;
    
    room_data->seat_count = MAX_SEATS;
    room_data->reader_count = 0;
    
    sema_init(&room_data->seat_sem, MAX_SEATS);  // 初始5个座位
    sema_init(&room_data->mutex, 1);             // 互斥锁
    sema_init(&room_data->info_mutex, 1);        // 信息互斥
    
    // 初始化所有读者信息
    for (i = 0; i < MAX_SEATS; i++) {
        memset(&room_data->readers[i], 0, sizeof(struct reader_info));
        room_data->readers[i].valid = 0;  // 标记为无效/空座位
    }
    
    printk(KERN_INFO "Reading room initialized with %d seats\n", MAX_SEATS);
    return 0;
}

// 读者注册系统调用 - 非阻塞版本
SYSCALL_DEFINE3(reader_register, char __user *, user_name, 
                char __user *, user_phone, int, name_len)
{
    int seat_id = -1;
    int i;
    char name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    
    if (name_len > MAX_NAME_LEN || name_len <= 0)
        return -EINVAL;
    
    // 复制用户名
    if (copy_from_user(name, user_name, name_len))
        return -EFAULT;
    name[name_len] = '\0';
    
    // 复制手机号
    if (copy_from_user(phone, user_phone, MAX_PHONE_LEN - 1))
        return -EFAULT;
    phone[MAX_PHONE_LEN - 1] = '\0';
    
    // 尝试获取座位（非阻塞）
    if (down_trylock(&room_data->seat_sem)) {
        return -EBUSY;  // 座位已满，立即返回EBUSY错误
    }
    
    // 互斥访问
    if (down_interruptible(&room_data->mutex)) {
        up(&room_data->seat_sem);  // 释放座位信号量
        return -ERESTARTSYS;
    }
    
    // 查找第一个空座位
    for (i = 0; i < MAX_SEATS; i++) {
        if (!room_data->readers[i].valid) {
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
    if (down_interruptible(&room_data->info_mutex)) {
        up(&room_data->mutex);
        up(&room_data->seat_sem);
        return -ERESTARTSYS;
    }
    
    // 保存读者信息（姓名和手机号）
    strncpy(room_data->readers[seat_id].name, name, MAX_NAME_LEN - 1);
    room_data->readers[seat_id].name[MAX_NAME_LEN - 1] = '\0';
    
    strncpy(room_data->readers[seat_id].phone, phone, MAX_PHONE_LEN - 1);
    room_data->readers[seat_id].phone[MAX_PHONE_LEN - 1] = '\0';
    
    room_data->readers[seat_id].valid = 1;
    room_data->reader_count++;  // 增加读者计数
    
    up(&room_data->info_mutex);
    up(&room_data->mutex);
    
    printk(KERN_INFO "Reader registered: %s (Phone: %s), Seat: %d\n", 
           name, phone, seat_id);
    return seat_id;  // 返回实际的座位号（0-4）
}

// 读者注销系统调用 - 修改为显示姓名和手机号
SYSCALL_DEFINE1(reader_unregister, int, seat_id)
{
    if (seat_id < 0 || seat_id >= MAX_SEATS)
        return -EINVAL;
    
    // 检查座位是否已被占用
    if (down_interruptible(&room_data->info_mutex))
        return -ERESTARTSYS;
    
    // 如果座位是空的，直接返回
    if (!room_data->readers[seat_id].valid) {
        up(&room_data->info_mutex);
        return 0;  // 座位本来就是空的，不需要操作
    }
    
    // 打印注销信息（包括姓名和手机号）
    printk(KERN_INFO "Reader unregistered: %s (Phone: %s), Seat: %d\n", 
           room_data->readers[seat_id].name, 
           room_data->readers[seat_id].phone, 
           seat_id);
    
    // 清除读者信息
    memset(&room_data->readers[seat_id], 0, sizeof(struct reader_info));
    room_data->readers[seat_id].valid = 0;
    
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

// 获取阅览室状态 - 修改为显示姓名和手机号
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
    
    // 显示所有有读者的座位（现在显示姓名和手机号）
    for (i = 0; i < MAX_SEATS; i++) {
        if (room_data->readers[i].valid) {
            occupied_seats++;
            total_len += snprintf(kbuf + total_len, buf_size - total_len,
                                "  Seat %d: %s (Phone: %s)\n", 
                                i, 
                                room_data->readers[i].name,
                                room_data->readers[i].phone);
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
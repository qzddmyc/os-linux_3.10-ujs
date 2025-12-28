#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage long sys_calculate_cube(int num) {
    long result;
    result = (long)num * num * num;
    printk(KERN_ALERT "系统调用触发: 数字 %d 的三次方是 %ld\n", num, result);
    return result;
}
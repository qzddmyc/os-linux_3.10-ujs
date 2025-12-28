#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_calculate_cube(int num, int __user *result)
{
    int val;
    val = num * num * num;
    printk(KERN_DEBUG "DebugLog: cube(%d) = %d\n", num, val);
    if (copy_to_user(result, &val, sizeof(int))) {
        return -EFAULT;
    }
    return 0;
}

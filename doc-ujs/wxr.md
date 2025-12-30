# 关于第一题的额外编译步骤

## v2

### 编译时添加的步骤

> 在执行完第 8 步的 chmod +x ~/os-run/initramfs/init 内容后，补充以下内容：

```bash
cd ~/os-run
mkdir workdir-wxr
cd workdir-wxr
vim test_cube.c
```
将以下内容复制进去：
```c
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_calculate_cube 351 

int main()
{
    int num = 5;
    int result_val = 0;
    long ret;

    printf("准备调用系统调用，计算 %d 的三次方...\n", num);

    ret = syscall(__NR_calculate_cube, num, &result_val);

    if(ret == 0) {
        printf("系统调用执行成功。\n");
        printf("计算结果: %d\n", result_val);
        
        if(result_val == 125) {
            printf("验证通过：结果正确！\n");
        } else {
            printf("验证失败：结果不正确！\n");
        }
    } else {
        printf("系统调用执行出错 (返回了错误码)！\n");
    }

    return 0;
}
```
保存退出后使用以下方式编译并复制：
```bash
gcc -m32 -static test_cube.c -o test_cube
cp ./test_cube ../initramfs/test_cube
vim cube.c
```
将以下内容复制进去：
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#define __NR_calculate_cube 351

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: cube <Integer>\n");
        return 1;
    }

    char *endptr;
    long val;

    errno = 0;
    val = strtol(argv[1], &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
        || (errno != 0 && val == 0)) {
        perror("Error: Input error");
        return 1;
    }

    if (endptr == argv[1] || *endptr != '\0') {
        fprintf(stderr, "Error: parameter should be an Integer\n");
        return 1;
    }

    if (val > INT_MAX || val < INT_MIN) {
        fprintf(stderr, "Error: Integer out of range\n");
        return 1;
    }

    int num = (int)val;
    int result = 0;
    long status;

    status = syscall(__NR_calculate_cube, num, &result);

    if (status == 0) {
        printf("%d\n", result);
        return 0;
    } else {
        perror("Syscall failed");
        return 1;
    }
}
```
保存退出后再次使用以下方式编译并复制至 bin ：
```bash
gcc -m32 -static cube.c -o cube
cp ./cube ../initramfs/bin/cube
```

之后，继续执行第 8 步的剩余步骤（cd ~/os-run/initramfs 开始）。

### 运行测试代码

在进入编译出的系统后，可以使用以下方式对该功能进行测试：
```bash
./test_cube
```
这份调用会使得系统写入一条日志，可以通过以下命令查看：
```bash
dmesg | tail
```
同时，由于在 bin 中添加了 cube，可以直接使用这个命令行工具，如：
```bash
cube 8
cube -12
```
使用命令行工具也可以生成日志，同样的方法可以查看。

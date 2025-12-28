# 关于第一题的额外编译步骤

## v1

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
    long ret;

    printf("准备调用系统调用，计算 %d 的三次方...\n", num);
    ret = syscall(__NR_calculate_cube, num);
    printf("系统调用返回结果: %ld\n", ret);
    if(ret == 125) {
        printf("结果正确！\n");
    } else {
        printf("结果错误！\n");
    }

    return 0;
}
```
保存退出后使用以下方式编译并复制：
```bash
gcc -m32 -static test_cube.c -o test_cube
cd ..
cp ./workdir-wxr/test_cube ./initramfs/test_cube
```
之后，继续执行第 8 步的剩余步骤（cd ~/os-run/initramfs 开始）。

### 运行测试代码

在进入编译出的系统后，可以使用以下方式对该功能进行测试：
```bash
./test_cube
```
这份调用会使得系统输出一条日志，可以通过以下命令查看：
```bash
dmesg | tail
```

## 编译部分

1. 创建 docker 容器
    ```bash
    docker build -t linux-3.10-native .
    docker run -it --rm -v "$(pwd)/output":/output linux-3.10-native
    ```
2. 执行编译
    ```bash
    make V=1 mrproper
    make ARCH=x86 i386_defconfig
    make ARCH=x86 -j$(nproc)
    make ARCH=x86 M=fs/myext3 modules
    make ARCH=x86 -C . M=./my_pfcount modules
    ```
3. 拷贝出编译产物
    ```bash
    cp arch/x86/boot/bzImage /output/
    cp ./fs/jbd/jbd.ko /output/
    cp drivers/block/mg_disk.ko /output/
    cp fs/myext3/myext3.ko /output/
    cp my_pfcount/pfcount.ko /output/
    exit
    ```
4. 将产物拷贝到运行目录（此步骤不再展示依赖项的安装）
    ```bash
    mkdir -p ~/os-run
    cd ~/os-run
    cp ~/os-linux_3.10-ujs/output/bzImage ~/os-run/
    ```
5. 预处理 busybox
    ```bash
    cp -r ~/os-linux_3.10-ujs/busybox/. ./busybox/
    cd busybox
    make defconfig
    sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
    sed -i 's/CONFIG_EXTRA_CFLAGS=""/CONFIG_EXTRA_CFLAGS="-m32"/' .config
    sed -i 's/CONFIG_EXTRA_LDFLAGS=""/CONFIG_EXTRA_LDFLAGS="-m32"/' .config
    sed -i 's/CONFIG_TC=y/# CONFIG_TC is not set/' .config
    sed -i 's/CONFIG_FEATURE_TC_INGRESS=y/# CONFIG_FEATURE_TC_INGRESS is not set/' .config
    ```
6. 编译 busybox
    ```bash
    make -j$(nproc)
    make install CONFIG_PREFIX=../_install
    mkdir ~/os-run/initramfs/
    cp -a ../_install/* ~/os-run/initramfs/
    ```
7. 初始化环境
    ```bash
    cd ~/os-run
    mkdir -p initramfs/{bin,sbin,etc,proc,sys,dev,mnt}
    cp ~/os-linux_3.10-ujs/output/mg_disk.ko ~/os-run/initramfs/
    cp ~/os-linux_3.10-ujs/output/myext3.ko ~/os-run/initramfs/
    cp ~/os-linux_3.10-ujs/output/jbd.ko ~/os-run/initramfs/
    cp ~/os-linux_3.10-ujs/output/pfcount.ko ~/os-run/initramfs/

    cat > ~/os-run/initramfs/init << 'EOF'
    #!/bin/sh
    mount -t proc none /proc
    mount -t sysfs none /sys
    mount -t devtmpfs none /dev
    echo
    echo "=== Linux 3.10 Minimal Shell ==="
    echo "<=== 2 ===>"
    echo "=== My custom Linux with MyExt3 Module ==="
    insmod /jbd.ko  
    insmod /myext3.ko  
    echo "--- Module Load Status ---"
    lsmod
    echo "--- Supported Filesystems ---"
    cat /proc/filesystems | grep myext3
    echo "=== MyExt3 OS Shell Ready ==="
    echo "<=== 3 ===>"
    echo "=== Loading Driver ==="
    echo "<=== 4 ===>"
    echo "=== System Booted ==="
    echo "Loading module..."
    insmod /pfcount.ko
    echo "Checking Page Faults:"
    cat /proc/pfcount
    exec /bin/sh
    EOF

    chmod +x ~/os-run/initramfs/init
    ```
8. 添加预设的测试文件
    ```bash
    cd ~/os-run
    mkdir workdir
    cd workdir
    vim test_cube.c
    ```
    写入以下代码：
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
    继续执行：
    ```bash
    gcc -m32 -static test_cube.c -o test_cube
    cp ./test_cube ../initramfs/test_cube
    vim cube.c
    ```
    写入以下代码：
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
    执行：
    ```bash
    gcc -m32 -static cube.c -o cube
    cp ./cube ../initramfs/bin/cube
    vim test_reader.c
    ```
    写入以下代码：
    ```c
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <string.h>
    #include <errno.h>
    #define __NR_reader_register 352
    #define __NR_reader_unregister 353
    #define __NR_get_room_status 354
    int count_seats(char *buffer) {
        int count = 0;
        char *ptr = buffer;
        while ((ptr = strstr(ptr, "Seat ")) != NULL) {
            count++;
            ptr += 5;
        }
        return count;
    }
    void show_status() {
        char buffer[1024];
        int dummy_count;
        long result = syscall(__NR_get_room_status, buffer, sizeof(buffer), &dummy_count);
        if (result <= 0) {
            printf("状态获取失败: %ld (errno: %d)\n", result, errno);
            return;
        }
        int actual_count = count_seats(buffer);
        printf("%s\n", buffer);
        printf("实际读者数: %d\n", actual_count);
        printf("可用座位: %d\n", 5 - actual_count);
    }
    int main() {
        printf("阅览室系统测试 - 带手机号版本（非阻塞版）\n");
        printf("==========================================\n\n");
        printf("1. 清理座位\n");
        for (int i = 0; i < 5; i++) {
            long ret = syscall(__NR_reader_unregister, i);
            printf("  座位%d: 返回 %ld\n", i, ret);
        }
        sleep(1);
        printf("\n2. 初始状态\n");
        show_status();
        printf("\n3. 注册3个读者（带手机号）\n");
        char *users1[] = {"张三", "李四", "王五"};
        char *phones1[] = {"13800138000", "13900139000", "13600136000"};

        for (int i = 0; i < 3; i++) {
            long seat = syscall(__NR_reader_register, users1[i], phones1[i], strlen(users1[i]));
            printf("   %s (%s) -> 座位 %ld\n", users1[i], phones1[i], seat);
            sleep(1);
        }
        printf("\n4. 当前状态\n");
        show_status();
        printf("\n5. 再注册2个读者\n");
        char *users2[] = {"赵六", "孙七"};
        char *phones2[] = {"13700137000", "13500135000"};
        for (int i = 0; i < 2; i++) {
            long seat = syscall(__NR_reader_register, users2[i], phones2[i], strlen(users2[i]));
            printf("   %s (%s) -> 座位 %ld\n", users2[i], phones2[i], seat);
            sleep(1);
        }
        printf("\n6. 满座状态\n");
        show_status();
        printf("\n7. 尝试注册第6个读者（测试非阻塞功能）\n");
        long test_result = syscall(__NR_reader_register, "周八", "13400134000", 6);

        if (test_result < 0) {
            if (errno == EBUSY) {
                printf("   正确：注册失败，返回 -EBUSY (座位已满)\n");
            } else {
                printf("   注册失败: %ld (errno: %d)\n", test_result, errno);
            }
        } else {
            printf("   注册成功: %ld (这可能是个错误，应该失败)\n", test_result);
        }
        printf("\n8. 注销座位1 (李四)\n");
        syscall(__NR_reader_unregister, 1);
        sleep(1);
        printf("\n9. 注销后状态\n");
        show_status();
        printf("\n10. 注册新读者到空出的座位\n");
        test_result = syscall(__NR_reader_register, "周八", "13400134000", 6);
        if (test_result >= 0) {
            printf("   周八 (%s) -> 座位 %ld\n", "13400134000", test_result);
        } else {
            printf("   注册失败: %ld (errno: %d)\n", test_result, errno);
        }
        printf("\n11. 测试边界情况\n");
        printf("   a) 测试无效用户名长度(0): ");
        test_result = syscall(__NR_reader_register, "测试", "13800138000", 0);
        printf("返回 %ld (errno: %d)\n", test_result, errno);
        printf("   b) 测试无效用户名长度(过长): ");
        test_result = syscall(__NR_reader_register, "测试用户", "13800138000", 100);
        printf("返回 %ld (errno: %d)\n", test_result, errno);
        printf("   c) 测试无效座位号注销(-1): ");
        test_result = syscall(__NR_reader_unregister, -1);
        printf("返回 %ld (errno: %d)\n", test_result, errno);
        printf("   d) 测试无效座位号注销(10): ");
        test_result = syscall(__NR_reader_unregister, 10);
        printf("返回 %ld (errno: %d)\n", test_result, errno);
        printf("   e) 重复注销同一个座位(2): ");
        test_result = syscall(__NR_reader_unregister, 2);
        printf("第一次返回 %ld\n", test_result);
        test_result = syscall(__NR_reader_unregister, 2);
        printf("第二次返回 %ld (应该是0，座位本来就是空的)\n", test_result);
        printf("\n12. 再次查看状态\n");
        show_status();
        printf("\n13. 清理所有座位\n");
        for (int i = 0; i < 5; i++) {
            long ret = syscall(__NR_reader_unregister, i);
            printf("  座位%d: 返回 %ld\n", i, ret);
        }
        sleep(1);
        printf("\n14. 清理后状态\n");
        show_status();
        printf("\n15. 测试空房间重新注册\n");
        for (int i = 0; i < 3; i++) {
            long seat = syscall(__NR_reader_register, users1[i], phones1[i], strlen(users1[i]));
            printf("   %s (%s) -> 座位 %ld\n", users1[i], phones1[i], seat);
        }
        printf("\n16. 最终状态\n");
        show_status();
        printf("\n17. 最终清理\n");
        for (int i = 0; i < 5; i++) {
            syscall(__NR_reader_unregister, i);
        }
        printf("\n测试完成，程序退出\n");
        return 0;
    }
    ```
    执行：
    ```bash
    gcc -m32 -static test_reader.c -o test_reader
    cp ./test_reader ../initramfs/test_reader
    ```
9. 根据环境生成 img 文件
    ```bash
    cd ~/os-run/initramfs
    find . | cpio -o -H newc | gzip > ../initramfs.img
    cd ~/os-run
    dd if=/dev/zero of=test_disk.img bs=1M count=20
    mkfs.ext3 test_disk.img
    ```
10. 运行
    ```bash
    cd ~/os-run

    qemu-system-i386 \
      -m 512M \
      -kernel bzImage \
      -initrd initramfs.img \
      -drive file=$(pwd)/test_disk.img,format=raw,if=ide \
      -append "console=ttyS0 earlyprintk=serial rdinit=/init" \
      -nographic \
      -serial mon:stdio
    ```

## 测试部分
1. 第一题（三次方）
    ```bash
    ./test_cube
    cube 8
    cube -12
    dmesg | tail
    ```
2. 第二题（基于模块的 ext3 文件系统）
    ```bash
    lsmod | grep myext3
    # 确认系统已识别新的文件系统名称，预期输出包含 myext3
    cat /proc/filesystems | grep myext3
    # 创建挂载点，并使用新文件系统名称挂载磁盘
    mknod /dev/sda b 8 0
    mount -t myext3 /dev/sda /mnt
    # 执行写操作，触发在 file.c 中添加的 printk
    echo "UJS Experiment Test" > /mnt/lab_test.txt
    # 强制将缓存写入磁盘，确保触发内核函数
    sync
    # 查看后台打印信息，需要类似 MyExt3: Writing to file... 日志
    dmesg | tail -n 10
    # 先取消挂载
    umount /mnt
    # 动态卸载模块
    rmmod myext3
    rmmod jbd
    # 检查模块列表，预期无 myext3 输出
    lsmod
    ```
3. 第三题（新增 Linux 驱动）
    ```bash
    # 动态加载驱动
    insmod /mg_disk.ko
    # 查找主设备号
    cat /proc/devices | grep mg_disk
    # 手动创建设备节点（???替换为上一步出现的数字）
    mknod /dev/mg_disk b ??? 0
    # 检测设备是否存在
    ls -l /dev/mg_disk
    # 手动创建 /dev/zero 和 /dev/null
    mknod /dev/zero c 1 5
    mknod /dev/null c 1 3
    # 彻底清空设备数据
    dd if=/dev/zero of=/dev/mg_disk bs=1M count=256
    # 格式化
    mkfs.ext2 /dev/mg_disk
    # 挂载设备
    mount -t ext2 /dev/mg_disk /mnt
    # 验证磁盘容量
    df -h
    # 文件读写 - 写入测试
    echo "Linux OS Course Design Passed!" > /mnt/final_success.txt
    # 读取测试
    cat /mnt/final_success.txt
    # 大文件压力测试
    dd if=/dev/zero of=/mnt/testfile bs=1M count=20
    # 详细查看文件信息
    ls -lh /mnt/testfile
    # 文件查看
    cat > /mnt/poem.txt << 'EOF'
    Two roads diverged in a yellow wood,
    And sorry I could not travel both
    And be one traveler, long I stood
    And looked down one as far as I could
    To where it bent in the undergrowth;
    EOF
    cat /mnt/poem.txt
    # 动态卸载 - 卸载文件系统
    umount /mnt
    # 移除驱动
    rmmod mg_disk
    # 确认驱动已移除（不应有输出）
    lsmod | grep mg_disk
    ```
4. 第四题（统计缺页次数）
    ```bash
    ls
    cat /proc/pfcount
    ls
    cat /proc/pfcount
    ```
5. 第五题（**进程**/线程通信）
    ```bash
    ./test_reader
    ```

执行第8步时，到注释位置停止，然后继续下面的操作：

```bash
cd ~/os-run
mkdir worker
cd worker
vim test_reader.c
```

写入：

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

#define __NR_reader_register 352
#define __NR_reader_unregister 353
#define __NR_get_room_status 354

// 统计座位数
int count_seats(char *buffer) {
    int count = 0;
    char *ptr = buffer;
    
    while ((ptr = strstr(ptr, "Seat ")) != NULL) {
        count++;
        ptr += 5;
    }
    
    return count;
}

// 显示状态
void show_status() {
    char buffer[1024];
    int dummy_count;
    
    long result = syscall(__NR_get_room_status, buffer, sizeof(buffer), &dummy_count);
    
    if (result <= 0) {
        printf("状态获取失败: %ld\n", result);
        return;
    }
    
    int actual_count = count_seats(buffer);
    
    printf("%s\n", buffer);
    printf("实际读者数: %d\n", actual_count);
    printf("可用座位: %d\n", 5 - actual_count);
}

int main() {
    printf("阅览室系统测试\n");
    printf("================\n\n");
    
    // 清理
    printf("1. 清理座位\n");
    for (int i = 0; i < 5; i++) {
        syscall(__NR_reader_unregister, i);
    }
    sleep(2);
    
    printf("\n2. 初始状态\n");
    show_status();
    
    // 注册3个
    printf("\n3. 注册3个读者\n");
    char *users1[] = {"用户1", "用户2", "用户3"};
    for (int i = 0; i < 3; i++) {
        long seat = syscall(__NR_reader_register, users1[i], strlen(users1[i]));
        printf("   %s -> 座位 %ld\n", users1[i], seat);
        sleep(1);
    }
    
    printf("\n4. 当前状态\n");
    show_status();
    
    // 再注册2个
    printf("\n5. 再注册2个读者\n");
    char *users2[] = {"用户4", "用户5"};
    for (int i = 0; i < 2; i++) {
        long seat = syscall(__NR_reader_register, users2[i], strlen(users2[i]));
        printf("   %s -> 座位 %ld\n", users2[i], seat);
        sleep(1);
    }
    
    printf("\n6. 满座状态\n");
    show_status();
    
    // 尝试第6个
    printf("\n7. 尝试注册第6个读者\n");
    long result = syscall(__NR_reader_register, "用户6", 9);
    if (result < 0) {
        printf("   注册失败: %ld\n", result);
    } else {
        printf("   注册成功: %ld\n", result);
    }
    
    // 注销一个
    printf("\n8. 注销座位1\n");
    syscall(__NR_reader_unregister, 1);
    sleep(1);
    
    printf("\n9. 注销后状态\n");
    show_status();
    
    // 注册新的
    printf("\n10. 注册新读者\n");
    result = syscall(__NR_reader_register, "用户6", 9);
    printf("   结果: %ld\n", result);
    
    printf("\n11. 最终状态\n");
    show_status();
    
    // 清理
    printf("\n12. 清理\n");
    for (int i = 0; i < 5; i++) {
        syscall(__NR_reader_unregister, i);
    }
    sleep(1);
    
    printf("\n13. 清理后状态\n");
    show_status();
    
    // 日志
    printf("\n14. 内核日志\n");
    system("dmesg | grep -i 'reader\\|seat' | tail -15");
    
    printf("\n测试完成，程序退出\n");
    
    // 退出程序
    return 0;
}
```

然后：

```bash
gcc -m32 -static test_reader.c -o test_reader
cd ..
cp ./worker/test_reader ./initramfs/test_reader
```

接着继续8的剩余两行。
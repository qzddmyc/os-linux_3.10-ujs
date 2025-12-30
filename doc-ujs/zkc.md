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
#include <errno.h>

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
    
    // 清理
    printf("1. 清理座位\n");
    for (int i = 0; i < 5; i++) {
        long ret = syscall(__NR_reader_unregister, i);
        printf("  座位%d: 返回 %ld\n", i, ret);
    }
    sleep(1);
    
    printf("\n2. 初始状态\n");
    show_status();
    
    // 注册3个读者（带手机号）
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
    
    // 再注册2个读者
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
    
    // 尝试第6个读者（应该立即返回失败，不会阻塞）
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
    
    // 注销座位1
    printf("\n8. 注销座位1 (李四)\n");
    syscall(__NR_reader_unregister, 1);
    sleep(1);
    
    printf("\n9. 注销后状态\n");
    show_status();
    
    // 注册新读者到空出的座位
    printf("\n10. 注册新读者到空出的座位\n");
    test_result = syscall(__NR_reader_register, "周八", "13400134000", 6);
    if (test_result >= 0) {
        printf("   周八 (%s) -> 座位 %ld\n", "13400134000", test_result);
    } else {
        printf("   注册失败: %ld (errno: %d)\n", test_result, errno);
    }
    
    printf("\n11. 测试边界情况\n");
    
    // 测试无效参数
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
    
    // 清理
    printf("\n13. 清理所有座位\n");
    for (int i = 0; i < 5; i++) {
        long ret = syscall(__NR_reader_unregister, i);
        printf("  座位%d: 返回 %ld\n", i, ret);
    }
    sleep(1);
    
    printf("\n14. 清理后状态\n");
    show_status();
    
    // 测试空房间注册
    printf("\n15. 测试空房间重新注册\n");
    for (int i = 0; i < 3; i++) {
        long seat = syscall(__NR_reader_register, users1[i], phones1[i], strlen(users1[i]));
        printf("   %s (%s) -> 座位 %ld\n", users1[i], phones1[i], seat);
    }
    
    printf("\n16. 最终状态\n");
    show_status();
    
    // 清理
    printf("\n17. 最终清理\n");
    for (int i = 0; i < 5; i++) {
        syscall(__NR_reader_unregister, i);
    }
    
    printf("\n测试完成，程序退出\n");
    
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
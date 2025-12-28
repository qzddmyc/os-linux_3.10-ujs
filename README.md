本仓库使用的 Linux 版本为 3.10，推荐在 wsl 的 Ubuntu24.04 中完成实验。

## 提交代码
请遵循以下步骤提交代码：
1. 克隆仓库到本地：
   ```bash
   cd ~
   git clone https://github.com/qzddmyc/os-linux_3.10-ujs.git
   cd os-linux_3.10-ujs
   git checkout main
   ```
   实验将基于 main 分支进行开发。*注意，请将文件夹放至虚拟机文件夹下，不可以放在 /mnt 中！*

   如果 clone 出现问题，请向下翻，查看“wsl 中无法链接到 github 仓库的解决方案”。
2. 新建分支进行开发:
   ```bash
   git checkout -b feat/your-feature-name origin/main
   ```
   *你需要将 your-feature-name 改为你想取的分支名字。*

   *注意，在每次执行编码前，请使用 git branch 查看你此时所在的分支！如果此时在 main，请使用 git checkout feat/your-feature-name 进入你的分支！*
3. 完成代码修改后，提交代码:
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```
   你可以在 commit 的引号内写入你本次修改的主要内容。**请务必作出个性化修改，不要直接使用 add your feature description 作为说明！同时，你的类型前缀需要遵守 [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) 命名规范。**
4. Push 分支到远程仓库:
   ```bash
   git push origin feat/your-feature-name
   ```
   *这里的 your-feature-name 要使用你刚刚取的分支名。*
5. 在 GitHub 上创建 Pull Request，描述你的修改内容。
> 注意：main 分支已做分支保护，**请勿直接在 main 分支上进行修改**。

## 使用说明
### 安装依赖
> 以下操作均基于 wsl 的 Ubuntu-24.04 版本实现，请先进入 wsl 环境。
1. 安装 docker：
    ```bash
    sudo apt update
    sudo apt install docker.io -y
    sudo usermod -aG docker $USER
    ```
2. 重启 wsl：
    ```bash
    # 退出 wsl 环境
    exit
    # 这是在 windows 终端，如 cmd 中执行的
    wsl --shutdown
    # 重新进入 wsl
    wsl
    ```
3. 开启 docker 服务（此时你应该在 wsl 中）：
    ```bash
    sudo systemctl start docker
    sudo systemctl enable docker
    ```
4. 修改代理配置文件：
    ```bash
    sudo vim /etc/docker/daemon.json
    ```
    按下 i 键，并复制以下内容粘贴至文件末尾：
    ```text
    {
      "registry-mirrors": [
        "https://docker.m.daocloud.io",
        "https://huecker.io",
        "https://dockerhub.timeweb.cloud",
        "https://noohub.net"
      ]
    }
    ```
    按下 Esc 键，输入 :wq 保存并返回。
5. 重启 docker 服务：
    ```bash
    sudo systemctl daemon-reload
    sudo systemctl restart docker
    ```

### 编译与运行你的内核

> 以下操作需要在你的项目目录（os-linux_3.10-ujs）中执行。

> 下面的操作的大致描述如下：创建一个包含 centos7 环境的 docker 容器，然后在容器内使用 gcc 将你的代码编译为一份二进制文件。之后退出 docker，将编译出的容器拷贝到另一份目录中（不在你的代码目录，具体而言是 ~/os-run），然后在此目录下使用 QEMU 与 BusyBox 创建环境并运行你的系统。

1. 使用 Dockerfile（已写好） 创建镜像并进入容器：
    ```bash
    docker build -t linux-3.10-native .
    docker run -it --rm -v "$(pwd)/output":/output linux-3.10-native
    ```
2. 执行编译
    ```bash
    make V=1 mrproper
    make ARCH=x86 i386_defconfig
    make ARCH=x86 -j$(nproc)

    ```
    编译成功的标志为出现以下内容（大致相同即可）：
    ```text
    # 注意这部分不是命令

    Setup is 14864 bytes (padded to 15360 bytes).
    System is 5285 kB
    CRC 309f814c
    Kernel: arch/x86/boot/bzImage is ready  (#1)
    ```
    *如果编译出现报错，可能是你的代码存在一些 bug。请使用 "exit" 退出 docker 环境并修改你的代码，之后重新从第1步（创建容器）开始重新操作。*
3. 拷贝出结果并退出：
    ```bash
    cp arch/x86/boot/bzImage /output/
    exit
    ```
    拷贝完成后，编译出的文件位于项目目录的 ./output/bzImage
4. 准备运行工作：

    ***注意：下方命令中最后一条中的 \~/Your-Path/os-linux_3.10-ujs 需要替换为你的实际项目目录，你可以使用 pwd 命令查看当前所在的目录 (如果你按照上面的操作进行，你的项目目录在：\~/os-linux_3.10-ujs)***

    ```bash
    sudo apt update
    sudo apt install -y qemu-system-x86 build-essential bc bison flex libncurses-dev libssl-dev libelf-dev cpio git
    sudo apt install -y gcc-multilib
    mkdir -p ~/os-run
    cd ~/os-run
    cp ~/Your-Path/os-linux_3.10-ujs/output/bzImage ~/os-run/
    ```
5. 克隆 busybox 并准备编译：

    ***注意：这里第一条命令中 ~/Your-Path/os-linux_3.10-ujs 同样需要替换为你的实际项目目录。***

    ```bash
    cp -r ~/Your-Path/os-linux_3.10-ujs/busybox/. ./busybox/
    cd busybox
    make defconfig
    sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
    sed -i 's/CONFIG_EXTRA_CFLAGS=""/CONFIG_EXTRA_CFLAGS="-m32"/' .config
    sed -i 's/CONFIG_EXTRA_LDFLAGS=""/CONFIG_EXTRA_LDFLAGS="-m32"/' .config
    ```
6. 手动去除一个模块，因为这个模块会导致fatal error：
    ```bash
    make menuconfig
    ```
    你需要通过上下键找到 Networking Utilities，按下Enter，向下翻找到 tc，按下空格（Space）取消勾选，再通过左右键选中 Exit，退出两次，最后选择保存。

    如果未能打开一个 GUI 界面，有可能是你的窗口太小了。
7. 编译 busybox：
    ```bash
    make -j$(nproc)
    make install CONFIG_PREFIX=../_install
    mkdir ~/os-run/initramfs/
    cp -a ../_install/* ~/os-run/initramfs/
    ```
    过程会出现很多 warning，无关紧要。

    成功的标志为出现以下内容：
    ```text
    # 注意这部分不是命令

    --------------------------------------------------
    You will probably need to make your busybox binary
    setuid root to ensure all configured applets will
    work properly.
    --------------------------------------------------
    ```
8. 创建环境：
    ```bash
    cd ~/os-run
    mkdir -p initramfs/{bin,sbin,etc,proc,sys,dev}

    cat > ~/os-run/initramfs/init << 'EOF'
    #!/bin/sh
    mount -t proc none /proc
    mount -t sysfs none /sys
    mount -t devtmpfs none /dev
    echo
    echo "=== Linux 3.10 Minimal Shell ==="
    exec /bin/sh
    EOF

    chmod +x ~/os-run/initramfs/init

    # 可在此进行附加文件的添加

    cd ~/os-run/initramfs
    find . | cpio -o -H newc | gzip > ../initramfs.img
    ```
    成功的标志为出现：4734 blocks。这个数字可能不同，但只要不是个位数就可以。

    **如果需要在你编译出的系统中添加测试文件，请在 chmod +x ~/os-run/initramfs/init 步骤后，在 ~/os-run/initramfs 目录下添加你想要带到新系统中的额外初始文件，之后按照剩余步骤执行即可。**
9. 运行：
    ```bash
    cd ~/os-run

    qemu-system-i386 \
      -kernel bzImage \
      -initrd initramfs.img \
      -append "console=ttyS0 earlyprintk=serial rdinit=/init" \
      -nographic \
      -serial mon:stdio
    ```
    会有很多日志输出，停止后按几下 Enter，出现#号即为成功。

    *如果想要退出，按下 Ctrl+A，松开后按下 X 即可。*

## 其他事项
### 如何在 wsl 中使用 vpn
> 请优先看下面的 “wsl 中无法链接到 github 仓库的解决方案” 步骤，如果仍然无法拉取仓库再考虑此说明。

> 如果 pull 与 push 操作失败，可参考以下基于 Clash Verge 的方法。
1. 打开clash的配置文件并修改这部分内容：
    ```yaml
    tun:
      enable: true
      auto-route: true
      strict-route: true
      dns-hijack:
        - any:53
    ```
    注意，你需要修改共三个文件：clash-verge.yaml、clash-verge-check.yaml、config.yaml。
2. 可以在 wsl 中使用以下命令测试，有输出大抵是没有问题的：
    ```bash
    curl ipinfo.io
    ```

### 关于代码的二次编译

1. 如果你的代码已经被编译过了，你在修改了代码之后想重新编译，你可以使用以下命令。***执行目录为项目根目录（os-linux_3.10-ujs 文件夹）***：
    ```bash
    sudo rm ./output/bzImage
    sudo rm -rf ~/os-run/
    ```
    之后，重新执行“编译与运行你的内核”部分的命令即可。

### 关于此项目中的 busybox 目录

1. 这里的 busybox 是使用以下方式获取的，在此略作说明：
    ```bash
    git clone https://git.busybox.net/busybox
    cd ./busybox/
    rm -rf ./.git
    rm ./.gitignore
    ```

### 关于 raw 分支

1. raw 分支为未进行任何开发时的源码备份，不可以进行任何修改，已作锁定。

### 如何在 vscode 中打开 wsl 中的文件

1. 前往扩展商店下载 “WSL” 插件，注意是 Microsoft 发行的。

2. 在 vscode 顶端的搜索框输入：“> wsl”，选择 “Connect to wsl”，然后打开你的项目文件夹即可。

### wsl 中无法链接到 github 仓库的解决方案

1. 在 wsl 中输入以下命令，创建你的 ssh key：
    ```bash
    ssh-keygen -t ed25519 -C "你的GitHub邮箱"
    ```
    之后一路回车即可。
2. 使用以下命令，得到你的公钥：
    ```bash
    cat ~/.ssh/id_ed25519.pub
    ```
    然后复制屏幕出现的你的公钥到剪贴板。
3. 按照以下操作执行：GitHub → 头像 → Settings → SSH and GPG keys → New SSH key → 粘贴公钥并随意取个名字 → 保存。
4. 在wsl中使用以下命令：
    ```bash
    vim ~/.ssh/config
    ```
    然后，按照 vim 编辑器的方式将以下内容贴进去：
    ```text
    Host github.com
      Hostname ssh.github.com
      Port 443
      User git
    ```
5. 然后使用以下方式去拉取代码即可：
    ```bash
    cd ~
    git clone git@github.com:qzddmyc/os-linux_3.10-ujs.git
    ```

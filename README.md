本仓库使用的 Linux 版本为 3.10。

## 提交代码
请遵循以下步骤提交代码：
1. 克隆仓库到本地：
   ```bash
   git clone https://github.com/qzddmyc/os-linux_3.10-ujs.git
   cd os-linux_3.10-ujs
   git checkout main
   ```
   *实验将基于 main 分支进行开发。注意，请将文件夹放至虚拟机文件夹下，不可以放在 /mnt 中！*
2. 新建分支进行开发:
   ```bash
   git checkout -b feat/your-feature-name origin/main
   ```
3. 完成代码修改后，提交代码:
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```
4. Push 分支到远程仓库:
   ```bash
   git push origin feat/your-feature-name
   ```
5. 在 GitHub 上创建 Pull Request，描述你的修改内容。
> 注意：main 分支为非直接修改分支，它为课程实验修改分支。**请勿直接在 main 分支上进行修改**。

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
    wsl --shutdown
    wsl
    ```
3. 开启 docker 服务：
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

### 编译运行
> 以下操作需要在你的项目目录中执行。
1. 使用 Dockerfile（已写好） 创建镜像并进入容器：
    ```bash
    docker build -t linux-3.10-native .
    docker run -it --rm -v "$(pwd)/output":/output linux-3.10-native
    ```
2. 执行编译
    ```bash
    make V=1 mrproper
    make defconfig
    make -j$(nproc)
    ```
3. 拷贝出结果：
    ```bash
    cp arch/x86/boot/bzImage /output/
    ```
    拷贝完成后，编译出的文件位于 ./output/bzImage

## 其他事项
### 如何在 wsl 中使用 vpn
> 如果 pull 与 push 操作失败，可参考以下基于 Clash 的方法。
1. 打开clash的配置文件并修改这部分内容：
    ```yaml
    tun:
      enable: true
      auto-route: true
      strict-route: true
      dns-hijack:
        - any:53
    ```
2. 使用以下命令测试即可：
    ```bash
    curl ipinfo.io
    ```

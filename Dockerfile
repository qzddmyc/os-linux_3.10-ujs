FROM centos:7

RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-* && \
    sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN yum install -y make gcc bc openssl-devel ncurses-devel bison flex \
    elfutils-libelf-devel perl git vim which cpio kmod

WORKDIR /root/linux-src
COPY . .

CMD ["/bin/bash"]
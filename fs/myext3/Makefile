#
# Makefile for the linux myext3-filesystem routines.
#

obj-m += myext3.o

myext3-y	:= balloc.o bitmap.o dir.o file.o fsync.o ialloc.o inode.o \
	   ioctl.o namei.o super.o symlink.o hash.o resize.o myext3_jbd.o

myext3-$(CONFIG_MYEXT3_FS_XATTR)	 += xattr.o xattr_user.o xattr_trusted.o
myext3-$(CONFIG_MYEXT3_FS_POSIX_ACL) += acl.o
myext3-$(CONFIG_MYEXT3_FS_SECURITY)	 += xattr_security.o
myext3-objs := $(myext3-y)
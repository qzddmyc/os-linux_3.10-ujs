/*
 *  linux/fs/myext3/file.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  myext3 fs regular file handling primitives
 *
 *  64-bit file support on 64-bit platforms by Jakub Jelinek
 *	(jj@sunsite.ms.mff.cuni.cz)
 */

#include <linux/quotaops.h>
#include "myext3.h"
#include "xattr.h"
#include "acl.h"

/*
 * Called when an inode is released. Note that this is different
 * from myext3_file_open: open gets called at every open, but release
 * gets called only when /all/ the files are closed.
 */
static int myext3_release_file (struct inode * inode, struct file * filp)
{
	if (myext3_test_inode_state(inode, MYEXT3_STATE_FLUSH_ON_CLOSE)) {
		filemap_flush(inode->i_mapping);
		myext3_clear_inode_state(inode, MYEXT3_STATE_FLUSH_ON_CLOSE);
	}
	/* if we are the last writer on the inode, drop the block reservation */
	if ((filp->f_mode & FMODE_WRITE) &&
			(atomic_read(&inode->i_writecount) == 1))
	{
		mutex_lock(&MYEXT3_I(inode)->truncate_mutex);
		myext3_discard_reservation(inode);
		mutex_unlock(&MYEXT3_I(inode)->truncate_mutex);
	}
	if (is_dx(inode) && filp->private_data)
		myext3_htree_free_dir_info(filp->private_data);

	return 0;
}

static ssize_t myext3_file_write_verbose(struct kiocb *iocb, const struct iovec *iov,
                                         unsigned long nr_segs, loff_t pos)
{
    // 向内核日志打印信息
   printk(KERN_INFO "MyExt3: Writing to file...\n");
    // 调用原本的通用写函数
    return generic_file_aio_write(iocb, iov, nr_segs, pos);
}

const struct file_operations myext3_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= myext3_file_write_verbose,
	.unlocked_ioctl	= myext3_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= myext3_compat_ioctl,
#endif
	.mmap		= generic_file_mmap,
	.open		= dquot_file_open,
	.release	= myext3_release_file,
	.fsync		= myext3_sync_file,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
};

const struct inode_operations myext3_file_inode_operations = {
	.setattr	= myext3_setattr,
#ifdef CONFIG_MYEXT3_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= myext3_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.get_acl	= myext3_get_acl,
	.fiemap		= myext3_fiemap,
};


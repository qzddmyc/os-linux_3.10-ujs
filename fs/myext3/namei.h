/*  linux/fs/myext3/namei.h
 *
 * Copyright (C) 2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
*/

extern struct dentry *myext3_get_parent(struct dentry *child);

static inline struct buffer_head *myext3_dir_bread(handle_t *handle,
						 struct inode *inode,
						 int block, int create,
						 int *err)
{
	struct buffer_head *bh;

	bh = myext3_bread(handle, inode, block, create, err);

	if (!bh && !(*err)) {
		*err = -EIO;
		myext3_error(inode->i_sb, __func__,
			   "Directory hole detected on inode %lu\n",
			   inode->i_ino);
		return NULL;
	}
	return bh;
}

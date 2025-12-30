/*
  File: fs/myext3/acl.h

  (C) 2001 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/

#include <linux/posix_acl_xattr.h>

#define MYEXT3_ACL_VERSION	0x0001

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
	__le32		e_id;
} myext3_acl_entry;

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
} myext3_acl_entry_short;

typedef struct {
	__le32		a_version;
} myext3_acl_header;

static inline size_t myext3_acl_size(int count)
{
	if (count <= 4) {
		return sizeof(myext3_acl_header) +
		       count * sizeof(myext3_acl_entry_short);
	} else {
		return sizeof(myext3_acl_header) +
		       4 * sizeof(myext3_acl_entry_short) +
		       (count - 4) * sizeof(myext3_acl_entry);
	}
}

static inline int myext3_acl_count(size_t size)
{
	ssize_t s;
	size -= sizeof(myext3_acl_header);
	s = size - 4 * sizeof(myext3_acl_entry_short);
	if (s < 0) {
		if (size % sizeof(myext3_acl_entry_short))
			return -1;
		return size / sizeof(myext3_acl_entry_short);
	} else {
		if (s % sizeof(myext3_acl_entry))
			return -1;
		return s / sizeof(myext3_acl_entry) + 4;
	}
}

#ifdef CONFIG_MYEXT3_FS_POSIX_ACL

/* acl.c */
extern struct posix_acl *myext3_get_acl(struct inode *inode, int type);
extern int myext3_acl_chmod (struct inode *);
extern int myext3_init_acl (handle_t *, struct inode *, struct inode *);

#else  /* CONFIG_MYEXT3_FS_POSIX_ACL */
#include <linux/sched.h>
#define myext3_get_acl NULL

static inline int
myext3_acl_chmod(struct inode *inode)
{
	return 0;
}

static inline int
myext3_init_acl(handle_t *handle, struct inode *inode, struct inode *dir)
{
	return 0;
}
#endif  /* CONFIG_MYEXT3_FS_POSIX_ACL */


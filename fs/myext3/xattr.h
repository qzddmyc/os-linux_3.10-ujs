/*
  File: fs/myext3/xattr.h

  On-disk format of extended attributes for the myext3 filesystem.

  (C) 2001 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/

#include <linux/xattr.h>

/* Magic value in attribute blocks */
#define MYEXT3_XATTR_MAGIC		0xEA020000

/* Maximum number of references to one attribute block */
#define MYEXT3_XATTR_REFCOUNT_MAX		1024

/* Name indexes */
#define MYEXT3_XATTR_INDEX_USER			1
#define MYEXT3_XATTR_INDEX_POSIX_ACL_ACCESS	2
#define MYEXT3_XATTR_INDEX_POSIX_ACL_DEFAULT	3
#define MYEXT3_XATTR_INDEX_TRUSTED		4
#define	MYEXT3_XATTR_INDEX_LUSTRE			5
#define MYEXT3_XATTR_INDEX_SECURITY	        6

struct myext3_xattr_header {
	__le32	h_magic;	/* magic number for identification */
	__le32	h_refcount;	/* reference count */
	__le32	h_blocks;	/* number of disk blocks used */
	__le32	h_hash;		/* hash value of all attributes */
	__u32	h_reserved[4];	/* zero right now */
};

struct myext3_xattr_ibody_header {
	__le32	h_magic;	/* magic number for identification */
};

struct myext3_xattr_entry {
	__u8	e_name_len;	/* length of name */
	__u8	e_name_index;	/* attribute name index */
	__le16	e_value_offs;	/* offset in disk block of value */
	__le32	e_value_block;	/* disk block attribute is stored on (n/i) */
	__le32	e_value_size;	/* size of attribute value */
	__le32	e_hash;		/* hash value of name and value */
	char	e_name[0];	/* attribute name */
};

#define MYEXT3_XATTR_PAD_BITS		2
#define MYEXT3_XATTR_PAD		(1<<MYEXT3_XATTR_PAD_BITS)
#define MYEXT3_XATTR_ROUND		(MYEXT3_XATTR_PAD-1)
#define MYEXT3_XATTR_LEN(name_len) \
	(((name_len) + MYEXT3_XATTR_ROUND + \
	sizeof(struct myext3_xattr_entry)) & ~MYEXT3_XATTR_ROUND)
#define MYEXT3_XATTR_NEXT(entry) \
	( (struct myext3_xattr_entry *)( \
	  (char *)(entry) + MYEXT3_XATTR_LEN((entry)->e_name_len)) )
#define MYEXT3_XATTR_SIZE(size) \
	(((size) + MYEXT3_XATTR_ROUND) & ~MYEXT3_XATTR_ROUND)

# ifdef CONFIG_MYEXT3_FS_XATTR

extern const struct xattr_handler myext3_xattr_user_handler;
extern const struct xattr_handler myext3_xattr_trusted_handler;
extern const struct xattr_handler myext3_xattr_acl_access_handler;
extern const struct xattr_handler myext3_xattr_acl_default_handler;
extern const struct xattr_handler myext3_xattr_security_handler;

extern ssize_t myext3_listxattr(struct dentry *, char *, size_t);

extern int myext3_xattr_get(struct inode *, int, const char *, void *, size_t);
extern int myext3_xattr_set(struct inode *, int, const char *, const void *, size_t, int);
extern int myext3_xattr_set_handle(handle_t *, struct inode *, int, const char *, const void *, size_t, int);

extern void myext3_xattr_delete_inode(handle_t *, struct inode *);
extern void myext3_xattr_put_super(struct super_block *);

extern int init_myext3_xattr(void);
extern void exit_myext3_xattr(void);

extern const struct xattr_handler *myext3_xattr_handlers[];

# else  /* CONFIG_MYEXT3_FS_XATTR */

static inline int
myext3_xattr_get(struct inode *inode, int name_index, const char *name,
	       void *buffer, size_t size, int flags)
{
	return -EOPNOTSUPP;
}

static inline int
myext3_xattr_set(struct inode *inode, int name_index, const char *name,
	       const void *value, size_t size, int flags)
{
	return -EOPNOTSUPP;
}

static inline int
myext3_xattr_set_handle(handle_t *handle, struct inode *inode, int name_index,
	       const char *name, const void *value, size_t size, int flags)
{
	return -EOPNOTSUPP;
}

static inline void
myext3_xattr_delete_inode(handle_t *handle, struct inode *inode)
{
}

static inline void
myext3_xattr_put_super(struct super_block *sb)
{
}

static inline int
init_myext3_xattr(void)
{
	return 0;
}

static inline void
exit_myext3_xattr(void)
{
}

#define myext3_xattr_handlers	NULL

# endif  /* CONFIG_MYEXT3_FS_XATTR */

#ifdef CONFIG_MYEXT3_FS_SECURITY
extern int myext3_init_security(handle_t *handle, struct inode *inode,
			      struct inode *dir, const struct qstr *qstr);
#else
static inline int myext3_init_security(handle_t *handle, struct inode *inode,
				     struct inode *dir, const struct qstr *qstr)
{
	return 0;
}
#endif

/*
 * drivers/block/mg_disk.c
 * Encrypted 256MB Ramdisk Driver - Final Atomic Fix
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h> 
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/highmem.h> 

#define MG_DEVICE_NAME "mg_disk"
#define MG_DISK_SIZE (256 * 1024 * 1024) 
#define MG_SECTOR_SIZE 512
#define MG_SECTORS (MG_DISK_SIZE / MG_SECTOR_SIZE)
#define MG_MINORS 16
#define CRYPTO_KEY 0x5A  

static int mg_major = 0;

static struct mg_device {
    u8 *data;
    spinlock_t lock;
    struct request_queue *queue;
    struct gendisk *gd;
} mg_dev;

static int mg_open(struct block_device *bdev, fmode_t mode) { return 0; }
static void mg_release(struct gendisk *disk, fmode_t mode) { }

static const struct block_device_operations mg_fops = {
    .owner = THIS_MODULE,
    .open = mg_open,
    .release = mg_release,
};

static void mg_transfer(struct request *req) 
{
    struct req_iterator iter;
    struct bio_vec *bvec;
    unsigned long offset = blk_rq_pos(req) * MG_SECTOR_SIZE;
    u8 *disk_mem = mg_dev.data + offset;
    
    /* 遍历每一个段 */
    rq_for_each_segment(bvec, req, iter) {
        /* 
         * 关键修复：使用 kmap_atomic 代替 kmap
         * kmap_atomic 可以在中断/自旋锁上下文中使用，不会导致死锁/卡顿
         */
        void *kaddr = kmap_atomic(bvec->bv_page);
        u8 *buffer = kaddr + bvec->bv_offset;
        unsigned long len = bvec->bv_len;
        unsigned long i;

        /* 边界检查 */
        if (offset + len > MG_DISK_SIZE) {
             printk(KERN_ERR "mg_disk: overrun detected\n");
             kunmap_atomic(kaddr);
             return;
        }

        /* 异或加密/解密 */
        if (rq_data_dir(req) == WRITE) {
            for (i = 0; i < len; i++) {
                disk_mem[i] = buffer[i] ^ CRYPTO_KEY;
            }
        } else {
            for (i = 0; i < len; i++) {
                buffer[i] = disk_mem[i] ^ CRYPTO_KEY;
            }
        }
        
        /* 必须成对使用 kunmap_atomic */
        kunmap_atomic(kaddr);
        
        disk_mem += len;
        offset += len;
    }
}

static void mg_request(struct request_queue *q)
{
    struct request *req;

    while ((req = blk_fetch_request(q)) != NULL) {
        if (req->cmd_type != REQ_TYPE_FS) {
            __blk_end_request_all(req, -EIO);
            continue;
        }
        
        if (unlikely(!mg_dev.data)) {
            __blk_end_request_all(req, -EIO);
            continue;
        }

        if (blk_rq_pos(req) + blk_rq_sectors(req) > MG_SECTORS) {
            __blk_end_request_all(req, -EIO);
            continue;
        }

        mg_transfer(req);
        __blk_end_request_all(req, 0);
    }
}

static int __init mg_init(void)
{
    mg_dev.data = vmalloc(MG_DISK_SIZE);
    if (!mg_dev.data) return -ENOMEM;

    mg_major = register_blkdev(0, MG_DEVICE_NAME);
    if (mg_major < 0) {
        vfree(mg_dev.data);
        return -EBUSY;
    }

    spin_lock_init(&mg_dev.lock);
    mg_dev.queue = blk_init_queue(mg_request, &mg_dev.lock);
    if (!mg_dev.queue) {
        unregister_blkdev(mg_major, MG_DEVICE_NAME);
        vfree(mg_dev.data);
        return -ENOMEM;
    }
    
    blk_queue_logical_block_size(mg_dev.queue, MG_SECTOR_SIZE);
    blk_queue_physical_block_size(mg_dev.queue, MG_SECTOR_SIZE);

    mg_dev.gd = alloc_disk(MG_MINORS); 
    if (!mg_dev.gd) {
        blk_cleanup_queue(mg_dev.queue);
        unregister_blkdev(mg_major, MG_DEVICE_NAME);
        vfree(mg_dev.data);
        return -ENOMEM;
    }

    mg_dev.gd->major = mg_major;
    mg_dev.gd->first_minor = 0;
    mg_dev.gd->fops = &mg_fops;
    mg_dev.gd->queue = mg_dev.queue;
    mg_dev.gd->private_data = &mg_dev;
    snprintf(mg_dev.gd->disk_name, 32, MG_DEVICE_NAME);
    set_capacity(mg_dev.gd, MG_SECTORS);

    add_disk(mg_dev.gd);
    printk(KERN_INFO "mg_disk: Secure Ramdisk Loaded (Atomic-Fixed).\n");
    return 0;
}

static void __exit mg_exit(void)
{
    if (mg_dev.gd) {
        del_gendisk(mg_dev.gd);
        put_disk(mg_dev.gd);
    }
    if (mg_dev.queue) {
        blk_cleanup_queue(mg_dev.queue);
    }
    unregister_blkdev(mg_major, MG_DEVICE_NAME);
    if (mg_dev.data) vfree(mg_dev.data);
    printk(KERN_INFO "mg_disk: module unloaded.\n");
}

module_init(mg_init);
module_exit(mg_exit);
MODULE_LICENSE("GPL");
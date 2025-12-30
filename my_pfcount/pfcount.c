#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>

extern unsigned long volatile pfcount;

static int pfcount_show(struct seq_file *m, void *v) {
    seq_printf(m, "Current Page Faults: %lu\n", pfcount);
    return 0;
}

static int pfcount_open(struct inode *inode, struct file *file) {
    return single_open(file, pfcount_show, NULL);
}

static const struct file_operations pfcount_fops = {
    .owner = THIS_MODULE,
    .open = pfcount_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init pfcount_init(void) {
    struct proc_dir_entry *entry;
    entry = proc_create("pfcount", 0444, NULL, &pfcount_fops);
    if (!entry) return -ENOMEM;
    printk(KERN_INFO "Page Fault Counter Module Loaded.\n");
    return 0;
}

static void __exit pfcount_exit(void) {
    remove_proc_entry("pfcount", NULL);
    printk(KERN_INFO "Page Fault Counter Module Unloaded.\n");
}

module_init(pfcount_init);
module_exit(pfcount_exit);
MODULE_LICENSE("GPL");

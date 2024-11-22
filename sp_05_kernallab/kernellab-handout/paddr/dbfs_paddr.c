#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;

static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
        pid_t pid;
        uint64_t va;                          // virtual address
        uint64_t pa = 0, ppn = 0, ppo = 0;    // physical address
        struct mm_struct *mm;
        char *buff;

        buff = kmalloc(length, GFP_KERNEL);

        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

        copy_from_user(&pid, user_buffer, 4);           // 4 byte aligned
        copy_from_user(&va, user_buffer + 8, 8);        // 8 byte aligned

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

        mm = task->mm;

        pgd = pgd_offset(mm, va);
        pud = pud_offset((p4d_t *)pgd, va);
        pmd = pmd_offset(pud, va);
        pte = pte_offset_kernel(pmd, va);

        pa = 0;
        ppn = (pte_pfn(*pte) << PAGE_SHIFT);
        ppo = va & ~PAGE_MASK;
        pa = ppn | ppo;

        copy_to_user(user_buffer + 16, &pa, 8);
        return length;
}

static const struct file_operations dbfs_fops = {
        // Mapping file operations with your functions
        .read = read_output,
};

static int __init dbfs_module_init(void)
{
        // Implement init module
        dir = debugfs_create_dir("paddr", NULL);

        if (!dir) {
                printk("Cannot create paddr dir\n");
                return -1;
        }

        // Fill in the arguments below
        output = debugfs_create_file("output", 0666, dir, NULL, &dbfs_fops);

	printk("dbfs_paddr module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module

	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

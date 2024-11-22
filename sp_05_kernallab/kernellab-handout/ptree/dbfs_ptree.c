#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *inputdir, *ptreedir;
static struct task_struct *curr;

struct ptree_node {
        struct task_struct *task;
        size_t size;
        struct list_head list;
};

static struct debugfs_blob_wrapper *blob_wrapper;

LIST_HEAD(ptree_list);

static ssize_t write_pid_to_input(struct file *fp, 
                                const char __user *user_buffer, 
                                size_t length, 
                                loff_t *position)
{
        pid_t input_pid;                                
        struct ptree_node *node, *temp;                 // curr node와 safe traversal시 사용할 temporary next node
        struct task_struct *task;                       // task를 iterate하기 위해 사용
        size_t total_size = 0, prev_size = 0;           // 전체 buffer 크기, 이전 `ptree_node`의 `size`
        char *ptree_buffer, *temp_buffer;               // 전체 buffer, size가 정해지기 이전의 임시 buffer

        sscanf(user_buffer, "%u", &input_pid);
        
        // Find task_struct using input_pid. Hint: pid_task
        curr = pid_task(find_vpid(input_pid), PIDTYPE_PID);

        // Tracing process tree from input_pid to init(1) process
        temp_buffer = kmalloc(1024, GFP_KERNEL);

        for (task = curr; task->pid != 0; task = task->parent) {
                node = kmalloc(sizeof(struct ptree_node), GFP_KERNEL);  // node 크기 만큼 kmalloc
                node->task = task;                                      // node task 지정
                node->size = sprintf(temp_buffer, "%s (%u)\n", task->comm, task->pid) + 1;      // sprintf 후 return 되는 출력된 글자 수를 node size로 지정
                total_size += node->size;                               
                INIT_LIST_HEAD(&node->list);                            
                list_add(&node->list, &ptree_list);                     // linked list의 맨 앞에 node 추가
	}

        // Make Output Format string: process_command (process_id)
        ptree_buffer = kmalloc(total_size + 1, GFP_KERNEL);
        prev_size = 0;          // 초기화 필수
        list_for_each_entry_safe(node, temp, &ptree_list, list) {
                /* for debug only */
                // printk("ptree %p prev_size %ld curr_size %ld pid %d\n", ptree_buffer, prev_size, node->size, node->task->pid);
                sprintf(ptree_buffer+prev_size, "%s (%u)\n", node->task->comm, node->task->pid);
                prev_size += node->size;
                list_del(&node->list);          // delete node 필수
        }

        blob_wrapper->size = (unsigned long) total_size;
	blob_wrapper->data = (void *) ptree_buffer;

        return length;
}

static const struct file_operations dbfs_fops = {
        .write = write_pid_to_input,
};

static int __init dbfs_module_init(void)
{
        // Implement init module code
        dir = debugfs_create_dir("ptree", NULL);
        
        if (!dir) {
                printk("Cannot create ptree dir\n");
                return -1;
        }

        blob_wrapper = kmalloc(sizeof(struct debugfs_blob_wrapper),GFP_KERNEL);

        inputdir = debugfs_create_file("input", 0666, dir, NULL, &dbfs_fops);
        ptreedir = debugfs_create_blob("ptree", 0666, dir, blob_wrapper); // Find suitable debugfs API
	
	printk("dbfs_ptree module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module code
	debugfs_remove_recursive(dir);
	printk("dbfs_ptree module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

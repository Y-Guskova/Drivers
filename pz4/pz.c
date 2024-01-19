#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");

static struct task_struct *my_thread; 
static DEFINE_MUTEX(my_mutex); 

static int shared_variable = 0; 

static int my_thread_function(void *data) {
    while (!kthread_should_stop()) {
        pr_info("My kernel thread is running!\n"); 
        msleep(1000); 
    }

    pr_info("My kernel thread is stopping!\n"); 
    return 0;
}

static int my_device_open(struct inode *inode, struct file *file) {
    pr_info("Device file opened.\n"); 
    return 0;
}

static int my_device_release(struct inode *inode, struct file *file) {
    pr_info("Device file closed.\n"); 
    return 0;
}

static ssize_t my_device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos) {
    int bytes_to_copy;
    mutex_lock(&my_mutex); 

    if (*ppos > 0) {
        mutex_unlock(&my_mutex);
        return 0; 
    }

    bytes_to_copy = min(count, sizeof(int));
    if (copy_to_user(user_buffer, &shared_variable, bytes_to_copy) != 0) {
        mutex_unlock(&my_mutex);
        return -EFAULT; 
    }

    *ppos += bytes_to_copy;
    mutex_unlock(&my_mutex);

    return bytes_to_copy;
}

static ssize_t my_device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos) {
    int bytes_to_copy;
    mutex_lock(&my_mutex); 

    if (*ppos > 0) {
        mutex_unlock(&my_mutex);
        return -ENOSPC;
    }

    bytes_to_copy = min(count, sizeof(int));
    if (copy_from_user(&shared_variable, user_buffer, bytes_to_copy) != 0) {
        mutex_unlock(&my_mutex);
        return -EFAULT; 
    }

    *ppos += bytes_to_copy;
    mutex_unlock(&my_mutex);

    return bytes_to_copy;
}

static struct file_operations fops = {
    .open = my_device_open,
    .release = my_device_release,
    .read = my_device_read,
    .write = my_device_write,
};


static int __init my_module_init(void) {
    pr_info("My module is being loaded.\n"); 

    my_thread = kthread_run(my_thread_function, NULL, "my_kernel_thread");
    if (my_thread) {
        pr_info("My kernel thread is started.\n");
    } else {
        pr_err("Failed to create kernel thread.\n"); 
        return -ENOMEM;
    }

    if (register_chrdev(0, "my_device", &fops) < 0) {
        pr_err("Failed to register character device.\n");
        return -EBUSY;
    }

    return 0;
}

static void __exit my_module_exit(void) {
    pr_info("My module is being unloaded.\n"); 

    if (my_thread) {
        kthread_stop(my_thread);
        pr_info("My kernel thread is stopped.\n"); 
    }

    unregister_chrdev(0, "my_device"); 
}

module_init(my_module_init);
module_exit(my_module_exit);



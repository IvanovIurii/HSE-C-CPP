#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>

MODULE_DESCRIPTION("Kernel module");
MODULE_AUTHOR("iurii.ivanov");
MODULE_LICENSE("GPL");

#define BUFFER_SIZE 2
#define P_POOL 3

static int major_number;
static struct class *my_class;
static struct device *my_device;

static int signal_num = SIGKILL; // default signal to send via kill_process; todo: make it a parameter
static int timeout = 10;         // default timeout is seconds; todo: parameter

DEFINE_MUTEX(my_device_mutex);

struct process_info
{
    pid_t pid;
    unsigned long start; // jiffies
};

static struct process_info processes[P_POOL]; // todo: implement connection pool

static int device_open(struct inode *inode, struct file *file)
{
    // unsigned long start = jiffies;
    pid_t pid = current->pid;
    printk(KERN_INFO "Device opened by process: %ld\n", (long)pid);

    // todo: extract as a function
    struct process_info *array = processes;

    mutex_lock(&my_device_mutex);

    int i;
    int count;
    for (i = 0; i < P_POOL; i++)
    {
        if (array[i].pid == 0)
        {
            array[i].pid = pid;
            array[i].start = jiffies;
            break;
        }
        count++;

        if (count == P_POOL)
        {
            printk(KERN_ERR "Connction refused. Try after timeout\n");
            return -EAGAIN;
        }
    }

    mutex_unlock(&my_device_mutex);

    for (i = 0; i < P_POOL; i++)
    {
        if (processes[i].pid != 0)
        {
            printk(KERN_INFO "Process in list: %ld\n", (long)processes[i].pid);
        }
    }

    // long end = jiffies;
    // printk(KERN_INFO "Time elapsed: %ld\n", jiffies_to_msecs(end - start) / 1000); // 3 sec

    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    // when finish writing and process releases FD
    printk(KERN_INFO "Device closed!\n");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset) // todo: user???
{
    // TODO: read logic here
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    // in real world this would require a mutex

    char buffer[BUFFER_SIZE];
    if (count - 1 > BUFFER_SIZE)
    {
        printk(KERN_ERR "Buffer overflow!\n");
        return -EINVAL;
    }

    if (copy_from_user(buffer, buf, BUFFER_SIZE))
    {
        return -EFAULT;
    }
    printk(KERN_INFO "Write: %s", buffer);

    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int device_init(void)
{
    major_number = register_chrdev(0, "my_device", &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "Device registration failed!\n");
        return major_number;
    }

    my_class = class_create(THIS_MODULE, "my_class");
    my_device = device_create(my_class, NULL, MKDEV(major_number, 0), NULL, "my_device");

    printk(KERN_INFO "Device registered with major number %d\n", major_number);
    return 0;
}
static void device_exit(void)
{
    // todo: delete from /dev/ as well
    device_destroy(my_class, MKDEV(major_number, 0));
    class_unregister(my_class);
    class_destroy(my_class);
    unregister_chrdev(major_number, "my_device"); // todo: #define DEVICE_NAME
    printk(KERN_INFO "Device unregistered!\n");
}

static void watchdog_thread(void *arg)
{
    // kernel thread to periodically check process activity
    // while (true)
    // {
    //     thread_sleep(1); // for example 1 sec
    // }
}

module_init(device_init);
module_exit(device_exit);

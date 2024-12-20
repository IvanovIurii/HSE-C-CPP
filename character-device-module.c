#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/pid.h>

MODULE_DESCRIPTION("Kernel module");
MODULE_AUTHOR("iurii.ivanov");
MODULE_LICENSE("GPL");

#define CLASS_NAME "my_class"
#define DEVICE_NAME "my_device"
#define BUFFER_SIZE 2
#define P_POOL 3

static int major_number;
static struct class *my_class;
static struct device *my_device;

struct process_info
{
    pid_t pid;
    unsigned long start; // jiffies
};

static struct process_info processes[P_POOL];
static struct task_struct *kthread;

static int device_init(void);
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static void device_exit(void);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

int watchdog_function(void *arg);
bool is_supported(int signal);
void update_start_timestamp(void);

static int signal = SIGALRM;
static int timeout = 30;

module_param(timeout, int, S_IRUGO);
MODULE_PARM_DESC(timeout, "Timeout value in seconds");

module_param(signal, int, S_IRUGO);
MODULE_PARM_DESC(debug, "Signal number");

DEFINE_MUTEX(my_device_mutex);

static int device_open(struct inode *inode, struct file *file)
{
    pid_t pid = current->pid;
    printk(KERN_INFO "Device has been opened by the process: %ld\n", (long)pid);

    mutex_lock(&my_device_mutex);

    int i;
    int count;
    for (i = 0; i < P_POOL; i++)
    {
        if (processes[i].pid == 0)
        {
            processes[i].pid = pid;
            processes[i].start = jiffies;
            break;
        }
        count++;

        if (count == P_POOL)
        {
            printk(KERN_INFO "Connction refused. Try again after timeout\n");
            return -EAGAIN;
        }
    }

    mutex_unlock(&my_device_mutex);

    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    int i;
    pid_t pid = current->pid;

    mutex_lock(&my_device_mutex);

    for (i = 0; i < P_POOL; i++)
    {
        if (processes[i].pid == pid)
        {
            processes[i].pid = 0;
            processes[i].start = 0;

            printk(KERN_INFO "Device has been released for the process: %ld\n", (long)pid);
        }
    }

    mutex_unlock(&my_device_mutex);

    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    printk(KERN_INFO "Updating process start timestamp on READ");
    update_start_timestamp();

    char *message = "PONG";
    if (copy_to_user(buf, message, strlen(message)))
    {
        return -EFAULT;
    }

    return count;
}

static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    printk(KERN_INFO "Updating process start timestamp on WRITE");
    update_start_timestamp();

    char buffer[BUFFER_SIZE];
    if (count - 1 > BUFFER_SIZE)
    {
        printk(KERN_ERR "Buffer overflow. Exit\n");
        return -EINVAL;
    }

    if (copy_from_user(buffer, buf, BUFFER_SIZE))
    {
        return -EFAULT;
    }

    printk(KERN_INFO "WRITE: %s", buffer);

    return count;
}

static int device_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ERR "Device registration failed\n");
        return major_number;
    }

    my_class = class_create(THIS_MODULE, CLASS_NAME);
    my_device = device_create(my_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(my_device))
    {
        printk(KERN_ERR "Cannot create device\n");
        return 1;
    }
    printk(KERN_INFO "Device registered with the major number %d\n", major_number);
    printk(KERN_INFO "Signal: %d\n", signal);
    printk(KERN_INFO "Timeout: %d\n", timeout);

    kthread = kthread_run(watchdog_function, NULL, "my_device_thread");
    if (IS_ERR(kthread))
    {
        printk(KERN_ERR "Cannot start a kernel thread. Exit\n");
        return 1;
    }

    return 0;
}

int watchdog_function(void *arg)
{
    while (true)
    {
        msleep(1000);

        int i;
        for (i = 0; i < P_POOL; i++)
        {
            unsigned long elapsed_time_sec = jiffies_to_msecs(jiffies - processes[i].start) / 1000;
            if (processes[i].pid != 0 && elapsed_time_sec >= timeout)
            {
                printk(KERN_INFO "Process %ld reached an idle timeout, sending a signal %d\n", (long)processes[i].pid, signal);

                if (kill_pid(find_vpid(processes[i].pid), signal, 1) < 0)
                {
                    printk(KERN_ERR "Failed to send a signal %d to a process %ld\n", signal, (long)processes[i].pid);
                }
            }
        }
    }
    return 0;
}

void update_start_timestamp()
{
    int i;
    pid_t pid = current->pid;

    mutex_lock(&my_device_mutex);

    for (i = 0; i < P_POOL; i++)
    {
        if (processes[i].pid == pid)
        {
            processes[i].start = jiffies;
        }
    }

    mutex_unlock(&my_device_mutex);
}

static void device_exit(void)
{
    device_destroy(my_class, MKDEV(major_number, 0));
    class_unregister(my_class);
    class_destroy(my_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Device has been unregistered\n");
}

module_init(device_init);
module_exit(device_exit);

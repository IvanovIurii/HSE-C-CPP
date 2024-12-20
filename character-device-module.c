// #define _POSIX_C_SOURCE
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

static int signal_num = SIGKILL; // todo: should be a map of supported signals
static int timeout = 10;         // timeout is seconds

struct process_info
{
    pid_t pid;
    unsigned long start; // jiffies
};

static struct process_info processes[P_POOL];
static struct task_struct *kthread;

int watchdog_function(void *arg);

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
            printk(KERN_ERR "Connction refused. Try after timeout\n");
            return -EAGAIN;
        }
    }

    mutex_unlock(&my_device_mutex);

    // TODO: ioctl get list of processes
    // for (i = 0; i < P_POOL; i++)
    // {
    //     if (processes[i].pid != 0) // hm, maybe there is no need in array*
    //     {
    //         printk(KERN_INFO "Process in list: %ld\n", (long)processes[i].pid);
    //     }
    // }

    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    // when finish writing and process releases FD
    printk(KERN_INFO "Device has been released \n");

    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf, size_t count, loff_t *offset) // todo: user???
{
    // TODO: read logic here
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    mutex_lock(&my_device_mutex);

    // TODO: add check if the process is capable of writing

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

    mutex_unlock(&my_device_mutex);

    return count;
}

static int device_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "Device registration failed\n");
        return major_number;
    }

    my_class = class_create(THIS_MODULE, CLASS_NAME);
    my_device = device_create(my_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

    printk(KERN_INFO "Device registered with the major number %d\n", major_number);

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
                printk(KERN_INFO "Process %ld reached an idle timeout, sending a signal %d\n", (long)processes[i].pid, signal_num);

                if (kill_pid(find_vpid(processes[i].pid), signal_num, 1) < 0)
                {
                    printk(KERN_ERR "Failed to send a signal %d to a process %ld\n", signal_num, processes[i].pid);
                }
                else
                {
                    if (signal_num == SIGKILL)
                    {
                        long pid = (long)processes[i].pid;

                        mutex_lock(&my_device_mutex);
                        processes[i].pid = 0;
                        processes[i].start = 0;
                        mutex_unlock(&my_device_mutex);

                        printk(KERN_INFO "Process %ld was removed from the pool", pid);
                    }
                }
            }
        }
    }
    return 0;
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

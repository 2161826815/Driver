#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#define COUNT 1
#define NAME "MY_DEV"
#define KEY_NUM 1
#define KEY_INVALUE 0xFF


struct key_desc {
    int gpio;                                   /*gpio id*/
    int irq;                                    /*irq id*/
    char name[10];                              /*key name*/
    unsigned long flags;                        /*Trigger mode*/
    irqreturn_t (*handler)(int,void*);          /*irq function*/
};

struct timer_desc {
    volatile unsigned long period;
};

struct device_desc{
    dev_t dev_id;
    int major;
    int minor;
    int gpio;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    struct key_desc my_key[KEY_NUM];
    struct timer_list timer;
    struct timer_desc time_args;

    spinlock_t lock;
}my_device;


static int dev_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &my_device;
    return 0;
}

static int dev_release(struct inode *nodep,struct file *filep)
{
    return 0;
}

static ssize_t dev_read(struct file *filep,char __user *buf,size_t count,loff_t *loffp)
{
    return 0;
}

static const struct file_operations dev_fop = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .open = dev_open,
    .release = dev_release,
};

static void timer_function(struct timer_list *t)
{  

}

static irqreturn_t key0_irq_handler(int irq, void* dev)
{
    /*
        when you init the timer
        Remember to define the timer function name
        change the struct key_desc.hander
    */
    return IRQ_HANDLED;
}

static int init_irq(struct device_desc *dev)
{


    return 0;
}

static void init_timer(struct device_desc *dev)
{
    struct device_desc *my_key = dev;
    /*init timer*/
    timer_setup(&my_key->timer,timer_function,0);

    /*set period*/
    my_key->time_args.period = 15;
    my_key->timer.expires = jiffies+msecs_to_jiffies(my_key->time_args.period);

    //add_timer(&my_key->timer);

}

static int init_key(struct device_desc *dev)
{
    return 0;
}

static int device_init(void)
{
    /*init device*/
    int ret;
    /*register a device*/
    if(my_device.major){
        my_device.dev_id = MKDEV(my_device.major,0);
        ret = register_chrdev_region(my_device.dev_id,COUNT,NAME);
    }else{
        ret = alloc_chrdev_region(&my_device.dev_id,0,COUNT,NAME);
        my_device.major = MAJOR(my_device.dev_id);
        my_device.minor = MINOR(my_device.dev_id);
    }
    if(ret<0){
        goto register_err;
    }
    printk("device major = %d\n",my_device.major);
    printk("device minor = %d\n",my_device.minor);

    /*add the device*/
    my_device.cdev.owner = THIS_MODULE;
    cdev_init(&my_device.cdev,&dev_fop);
    ret = cdev_add(&my_device.cdev,my_device.dev_id,COUNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automatically*/
    my_device.class = class_create(THIS_MODULE,NAME);
    if(IS_ERR(my_device.class)){
        goto class_cread_err;
    }
    my_device.device = device_create(my_device.class,NULL,my_device.dev_id,NULL,NAME);
    if(IS_ERR(my_device.class)){
        goto device_cread_err;
    }

    return ret;

device_cread_err:
    class_destroy(my_device.class);
    return PTR_ERR(my_device.device);
class_cread_err:
    cdev_del(&my_device.cdev);
    return PTR_ERR(my_device.class);
cdev_add_err:
    printk("cdev add error");
    unregister_chrdev_region(my_device.dev_id,COUNT);
register_err:
    printk("register device error\n");
    return -EINVAL;
}

static int __init dev_init(void)
{
    int ret;

    /*init device*/
    ret = device_init();
    if(ret<0){
        return -EINVAL;
    }

    /*init spinlock*/
    spin_lock_init(&my_device.lock);

    /*init key*/
    ret = init_key(&my_device);
    if(ret<0){
        return -EINVAL;
    }

    /*init timer*/
    init_timer(&my_device);

    /*init irq*/
    ret = init_irq(&my_device);
    if(ret<0){
        return -EINVAL;
    }


    return 0;

}

static void __exit dev_exit(void)
{   
    int i;

    /*release irq*/
    for(i=0;i<KEY_NUM;i++){
        free_irq(my_device.my_key[i].irq,&my_device);
    }

    /*release gpio*/
    for(i=0;i<KEY_NUM;i++){
        gpio_free(my_device.my_key[i].gpio);
    }
 
    /*release timer*/
    del_timer_sync(&my_device.timer);

    cdev_del(&my_device.cdev);
    unregister_chrdev_region(my_device.dev_id,COUNT);

    device_destroy(my_device.class,my_device.dev_id);
    class_destroy(my_device.class);
  
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");


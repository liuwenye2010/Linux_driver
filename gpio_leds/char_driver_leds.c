/*包含初始化宏定义的头文件,代码中的module_init和module_exit在此文件中*/
#include <linux/init.h>
/*包含初始化加载模块的头文件,代码中的MODULE_LICENSE在此头文件中*/
#include <linux/module.h>
/*定义module_param module_param_array的头文件*/
#include <linux/moduleparam.h>
/*定义module_param module_param_array中perm的头文件*/
#include <linux/stat.h>
/*三个字符设备函数*/
#include <linux/fs.h>
/*MKDEV转换设备号数据类型的宏定义*/
#include <linux/kdev_t.h>
/*定义字符设备的结构体*/
#include <linux/cdev.h>
/*分配内存空间函数头文件*/
#include <linux/slab.h>
/*包含函数device_create 结构体class等头文件*/
#include <linux/device.h>

/*自定义头文件*/
#include "char_driver_leds.h"

/*Linux中申请GPIO的头文件*/
#include <linux/gpio.h>
/*三星平台的GPIO配置函数头文件*/
/*三星平台EXYNOS系列平台，GPIO配置参数宏定义头文件*/
#include <plat/gpio-cfg.h>
/*三星平台4412平台，GPIO宏定义头文件*/
#include <mach/gpio-exynos4.h>


MODULE_LICENSE("Dual BSD/GPL");
/*声明是开源的，没有内核版本限制*/
MODULE_AUTHOR("iTOPEET_dz");
/*声明作者*/

static int led_gpios[] = {
	EXYNOS4_GPL2(0),EXYNOS4_GPK1(1),
};
#define LED_NUM		ARRAY_SIZE(led_gpios)


int numdev_major = DEV_MAJOR;
int numdev_minor = DEV_MINOR;

/*输入主设备号*/
module_param(numdev_major,int,S_IRUSR);
/*输入次设备号*/
module_param(numdev_minor,int,S_IRUSR);

static struct class *myclass;
struct reg_dev *my_devices;

/*打开操作*/
static int chardevnode_open(struct inode *inode, struct file *file){
	printk(KERN_EMERG "chardevnode_open is success!\n");
	
	return 0;
}
/*关闭操作*/
static int chardevnode_release(struct inode *inode, struct file *file){
	printk(KERN_EMERG "chardevnode_release is success!\n");
	
	return 0;
}
/*IO操作*/
static long chardevnode_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	
	switch(cmd)
	{
		case 0:
		case 1:
			if (arg > LED_NUM) {
				return -EINVAL;
			}

			gpio_set_value(led_gpios[arg], cmd);
			break;

		default:
			return -EINVAL;
	}
	
	printk(KERN_EMERG "chardevnode_ioctl is success! cmd is %d,arg is %d \n",cmd,arg);
	
	return 0;
}

ssize_t chardevnode_read(struct file *file, char __user *buf, size_t count, loff_t *f_ops){
	return 0;
}

ssize_t chardevnode_write(struct file *file, const char __user *buf, size_t count, loff_t *f_ops){
	return 0;
}

loff_t chardevnode_llseek(struct file *file, loff_t offset, int ence){
	return 0;
}
struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = chardevnode_open,
	.release = chardevnode_release,
	.unlocked_ioctl = chardevnode_ioctl,
	.read = chardevnode_read,
	.write = chardevnode_write,
	.llseek = chardevnode_llseek,
};


/*设备注册到系统*/
static void reg_init_cdev(struct reg_dev *dev,int index){
	int err;
	int devno = MKDEV(numdev_major,numdev_minor+index);

	/*数据初始化*/
	cdev_init(&dev->cdev,&my_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &my_fops;
	
	/*注册到系统*/
	err = cdev_add(&dev->cdev,devno,1);
	if(err){
		printk(KERN_EMERG "cdev_add %d is fail! %d\n",index,err);
	}
	else{
		printk(KERN_EMERG "cdev_add %d is success!\n",numdev_minor+index);
	}
}

static int gpio_init(void){
	int i=0,ret;
	
	for(i=0;i<LED_NUM;i++){
		ret = gpio_request(led_gpios[i], "LED");
		if (ret) {
			printk("%s: request GPIO %d for LED failed, ret = %d\n", DEVICE_NAME,i,ret);
			return -1;
		}
		else{
			s3c_gpio_cfgpin(led_gpios[i], S3C_GPIO_OUTPUT);
			gpio_set_value(led_gpios[i], 1);			
		}
	}
	
	return 0;
}


static int scdev_init(void)
{
	int ret = 0,i;
	dev_t num_dev;
	
	
	printk(KERN_EMERG "numdev_major is %d!\n",numdev_major);
	printk(KERN_EMERG "numdev_minor is %d!\n",numdev_minor);
	
	if(numdev_major){
		num_dev = MKDEV(numdev_major,numdev_minor);
		ret = register_chrdev_region(num_dev,DEVICE_MINOR_NUM,DEVICE_NAME);
	}
	else{
		/*动态注册设备号*/
		ret = alloc_chrdev_region(&num_dev,numdev_minor,DEVICE_MINOR_NUM,DEVICE_NAME);
		/*获得主设备号*/
		numdev_major = MAJOR(num_dev);
		printk(KERN_EMERG "adev_region req %d !\n",numdev_major);
	}
	if(ret<0){
		printk(KERN_EMERG "register_chrdev_region req %d is failed!\n",numdev_major);		
	}
	myclass = class_create(THIS_MODULE,DEVICE_NAME);
	
	
	my_devices = kmalloc(DEVICE_MINOR_NUM * sizeof(struct reg_dev),GFP_KERNEL);
	if(!my_devices){
		ret = -ENOMEM;
		goto fail;
	}
	memset(my_devices,0,DEVICE_MINOR_NUM * sizeof(struct reg_dev));
	
	/*设备初始化*/
	for(i=0;i<DEVICE_MINOR_NUM;i++){
		my_devices[i].data = kmalloc(REGDEV_SIZE,GFP_KERNEL);
		memset(my_devices[i].data,0,REGDEV_SIZE);
		/*设备注册到系统*/
		reg_init_cdev(&my_devices[i],i);
		
		/*创建设备节点*/
		device_create(myclass,NULL,MKDEV(numdev_major,numdev_minor+i),NULL,DEVICE_NAME"%d",i);
	}
	
	ret = gpio_init();
	if(ret){
		printk(KERN_EMERG "gpio_init failed!\n");
	}	
		
	printk(KERN_EMERG "scdev_init!\n");
	/*打印信息，KERN_EMERG表示紧急信息*/
	return 0;

fail:
	/*注销设备号*/
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
	printk(KERN_EMERG "kmalloc is fail!\n");
	
	return ret;
}

static void scdev_exit(void)
{
	int i;
	printk(KERN_EMERG "scdev_exit!\n");
	
	/*除去字符设备*/
	for(i=0;i<DEVICE_MINOR_NUM;i++){
		cdev_del(&(my_devices[i].cdev));
		/*摧毁设备节点函数d*/
		device_destroy(myclass,MKDEV(numdev_major,numdev_minor+i));
	}
	/*释放设备class*/
	class_destroy(myclass);
	/*释放内存*/
	kfree(my_devices);
	
	/*释放GPIO*/
	for(i=0;i<LED_NUM;i++){
		gpio_free(led_gpios[i]);
	}
		
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
}


module_init(scdev_init);
/*初始化函数*/
module_exit(scdev_exit);
/*卸载函数*/
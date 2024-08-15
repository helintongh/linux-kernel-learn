#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#define CHAR_MAJOR 200 //主设备号
#define CHAR_NAME "mychardev" //设备名称

static char readbuf[100];
static char writebuf[100];

//打开设备
static int chrdev_open(struct inode *inode, struct file *filp)
{
  printk(KERN_EMERG"open my char dev");
  return 0;
}


//用户空间从设备读取数据
static ssize_t chrdev_read(struct file *filp, char __user *buf,
                      size_t cnt, loff_t *offt)
{
  int ret;

  ret = copy_to_user(buf, readbuf, cnt); //向用户空间拷贝数据
  if(ret == 0) {
    printk(KERN_EMERG"kernel send ok");
  }
  else {
    printk(KERN_EMERG"kernel send err");
  }
  return 0;
}
//用户空间向设备写数据
static ssize_t chrdev_write(struct file *filp,
                      const char __user *buf, size_t cnt, loff_t *offt)
{
  int ret;
  ret = copy_from_user(writebuf, buf, cnt);//从用户空间拷贝数据
  if(ret == 0) {
    memcpy(readbuf, writebuf, sizeof(readbuf));
    printk(KERN_EMERG"kernel recv :%s" ,writebuf);
  }
  else {
    printk(KERN_EMERG"kernel recv err");
  }
  return 0;
}
//设备释放
static int chrdev_release(struct inode *inode, struct file *filp)
{
  printk(KERN_EMERG"release my char dev");
  return 0;
}

//相关操作函数结构体填充
static struct file_operations chrdev_fops = {
  .owner = THIS_MODULE,
  .open = chrdev_open,
  .read = chrdev_read,
  .write = chrdev_write,
  .release = chrdev_release,
};

static int __init chrdev_init(void)
{
  int ret;
  memcpy(readbuf,"hello i am dev date",sizeof("hello i am dev dat"));

  ret = register_chrdev(CHAR_MAJOR,CHAR_NAME,&chrdev_fops); //注册字符设备
  if(ret == 0) {
    printk(KERN_EMERG"char dev register failed !\r\n");
    return 0;
  }

  printk(KERN_EMERG"char dev register success !\r\n");
}

static void __exit chrdev_exit(void)
{
  unregister_chrdev(CHAR_MAJOR,CHAR_NAME); //注销字符设备
  printk(KERN_EMERG"chr dev exit");
}

//指定设备驱动入口和出口函数
module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("helintong");
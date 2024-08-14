# 字符设备驱动编程入门

本文主要从三个方面讲解:

1. 字符设备驱动程序背后的概念
2. 可以在字符设备上执行的各种操作
3. 处理等待队列

## 概览

Linux将驱动分为三大类：字符设备驱动、块设备驱动以及网络设备驱动，字符类驱动是按字节流进行读写操作的，例如I2C，GPIO等驱动。

在UNIX 中(Linux继承于Unix)，用户通过特殊的设备文件来访问硬件设备。这些文件被分组到/dev目录中，系统调用open、read、write、close、lseek、mmap等被操作系统重定向到与物理设备关联的设备驱动程序。设备驱动程序是与硬件设备交互的内核组件（通常是模块）。

这里说明一点，DPDK中用到的UIO其实就是字符设备驱动的一种。

一般每个字符设备或者块设备都会在/dev目录下对应一个设备文件。Linux用户层程序通过设备文件来使用驱动程序操作字符设备或块设备。

注:网络驱动，尤其是DPDK网络驱动会在/dev/uioX(X为0,1,2....)中对应设备文件。

## 字符驱动编程步骤概览

简单描述linux下应用程序如何调用驱动程序：

1. 应用程序中调用`open()`,`close()`,`read()`,`write()`等API函数。
2. API函数会调用Linux提供的对应库函数。
3. 通过系统调用进入内核。
4. 最终调动驱动程序来控制硬件设备。

Linux字符设备驱动，主要关注以下函数及结构体实现：
1. `module_init()`，模块加载函数；
2. `module_exit()`，模块卸载函数；
3. `file_operations`结构体：
4. 结构体之下的各类操作函数实现，`open()`,`read()`,`write()`,`ioctl()`,`release()`等.
5. 字符设备号的申请；

具体细节来讲分为三大模块,对应上面提到的驱动中的3，4，5的实现:

**1. 驱动初始化**

- 分配cdev
  - 静态分配
  - 动态分配
- 初始化cdev -- `cdev_init()`
- 注册cdev -- `cdev_add()`
- 硬件初始化

**2. 实现设备操作**

这里核心是结构体`struct file_operations`。

```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iterate) (struct file *, struct dir_context *);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
    // 省略
};
```

内核模块编程中非常多的类似的操作，声明一个结构体，该结构体是内核声明的内部为函数指针。实现对应函数赋值到该结构体的函数指针变量上即完成了对应功能。

- `open()`
- `read()`，read函数的实现往往会用到一个内核提供的函数`copy_to_user`作用是向用户空间拷贝数据。
- `write()`，与之相对的write函数也往往会用到内核提供的函数`copy_from_user`作用是从用户空间拷贝数据。注意和read的差别，一个是把字符传递给用户空间，write是从用户空间获取数据。
- `lseek()`
- `close()`

一般来说至少得实现上述五个函数，分别对应操作为打开，读取，写入，获取文件偏移，关闭。

重中之重为读取，写入和获取文件偏移。分别对应的操作是查，增，改(改是通过文件偏移+写入实现的)。

**3. 实现驱动注销相关操作**

- `cdev_del`
- `unregister_chrdev_region`



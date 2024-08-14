# 内核模块编程入门

内核模块是Linux内核向外部提供的一个插口，其全称为动态可加载内核模块（Loadable Kernel Module，LKM），一般简称为模块。Linux内核之所以提供模块机制，是因为它本身是一个单内核（monolithic kernel）。单内核的最大优点是效率高，因为所有的内容都集成在一起，但其缺点是可扩展性和可维护性相对较差，模块机制就是为了弥补这一缺陷。

## 什么是模块

模块是具有独立功能的程序，它可以被单独编译，但不能独立运行。它在运行时被链接到内核作为内核的一部分在内核空间运行，这与运行在用户空间的进程是不同的。模块通常由一组函数和数据结构组成，用来实现一种文件系统、一个驱动程序或其他内核上层的功能。

注: **一般来说模块最好使用linux内核提供的数据结构和函数，不使用glibc的函数。**

## 编写一个简单的模块功能

下面写一个内核版本的Hello World。

代码如下:

```c
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

//内核模块初始化函数
static int __init lkm_init(void)
{
	printk("Hello World\n");
	return 0;
}

//内核模块退出函数
static void __exit lkm_exit(void)
{
	printk("Goodbye\n");
}

module_init(lkm_init);
module_exit(lkm_exit);

MODULE_LICENSE("GPL");
```

需要的头文件为:

```c
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
```

1. module.h头文件中包含了对模块的结构定义以及模块的版本控制，任何模块程序的编写都要包含这个头文件。
2. kernel.h包含了常用的内核函数
3. init.h包含了宏`_init`和`_exit`，宏`_init`告诉编译器相关的函数和变量仅用于初始化，编译程序将标有`_init`的所有代码存储到特殊的内存段中，初始化结束后就释放这段内存。

关键内核函数调用为:

```c
module_init(lkm_init);
module_exit(lkm_exit);

MODULE_LICENSE("GPL");
```

内核模块使用特定的函数进行初始化和清理。

其中，`module_init`是注册初始化函数，`module_exit`是注册退出清理函数。

`MODULE_LICENSE`是注册你这个模块所使用的开源协议。

同时还有两个可选项:

```c
MODULE_AUTHOR("ChuanShao258"); 
MODULE_DESCRIPTION("hello kernel"); 
```

可选项可以帮助看代码的人快速找到维护者是谁，最好写上。

实际函数调用:


```c
//内核模块初始化函数
static int __init lkm_init(void)
{
	printk("Hello World\n");
	return 0;
}

//内核模块退出函数
static void __exit lkm_exit(void)
{
	printk("Goodbye\n");
}
```

这里唯一要说明的是，内核中的`printf`其实是`printk`。

## 内核模块编译

```Makefile
#Makefile文件注意：假如前面的.c文件起名为first.c，那么这里的Makefile文件中的.o文
#件就要起名为first.o    只有root用户才能加载和卸载模块
obj-m:=helloworld.o                          #产生helloworld模块的目标文件
#目标文件  文件  要与模块名字相同
CURRENT_PATH:=$(shell pwd)             #模块所在的当前路径
LINUX_KERNEL:=$(shell uname -r)        #linux内核代码的当前版本
LINUX_KERNEL_PATH:=/usr/src/linux-headers-$(LINUX_KERNEL)

all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules    #编译模块
#[Tab]              内核的路径       当前目录编译完放哪  表明编译的是内核模块

clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean      #清理模块
```

关键点在于makefile调用shell命令获取当前编译环境的内核头文件，然后进行编译。

## 编译后加载与卸载

编译出来的文件为`helloworld.ko`这个就是内核模块文件。

然后执行命令`insmod helloworld.ko`把模块加载到内核。

卸载的话执行命令`rmmod helloworld`即可。

## 查看模块是否加载成功

大多数linux系统可以使用`dmesg`。

我的主机执行insmod后上面输出如下:

```c
[177990.739946] helloworld: loading out-of-tree module taints kernel.
[177990.739978] helloworld: module verification failed: signature and/or required key missing - tainting kernel
[177990.742094] Hello World
```

执行rmmod后输出如下:

```c
[178078.643855] Goodbye
```

## 附录:怎么带参数

内核模块同样可以注册进入内核时传递一个参数给该模块。

```c
#include <linux/init.h> 
#include <linux/module.h> 

static int num=0; 
// 接收参数num，模块加载时指定num=xxx，可以设置该值 
module_param(num, int, S_IRUGO); 
// 模块加载时调用 
static int __init lkm_init(void) 
{ 
    printk("Hello World\n");
    printk("num = %d\n", num); 
    return 0; 
} 
// 模块卸载时调用 
static void lkm_exit(void) 
{ 
    printk("Goodbye\n");
} 
// 注册加载和卸载函数 
module_init(lkm_init); 
module_exit(lkm_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("ChuanSao258"); 
MODULE_DESCRIPTION("hello kernel"); 
```

此时注册命令可以为:

```shell
 insmod helloworld.ko num=888 
```

# 总结

内核模块编程主要分为三大块:

1. 实际函数实现。
2. `module_init`和`module_exit`注册。
3. 模块编写人信息，该模块代码遵循的开源协议等。
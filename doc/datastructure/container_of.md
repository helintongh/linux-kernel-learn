# container_of

linux内核中经常能够见到这个宏。`container_of(ptr,type,member)`

其函数原型如下:

```c
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
```

其作用是，通过某个已知的变量找到包含它的结构体变量。具体一点是通过member的地址计算得到包含该member的type的首地址。

其第一个参数为: 指向成员的指针。 第二个参数为: 嵌入的容器结构的类型。第三个参数为: 结构中成员的名称。

它的原理也不复杂: 已知结构体type的成员member的地址ptr。求解结构体type的起始地址。

**type的起始地址 = ptr - size**。这里需要说明一下此时操作的是内存，不关注其类型。

这里还剩下一个没有讲。即**如何计算size**。

计算size的实现是`offsetof`宏，原型如下:

```c
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
```

## 1.通过0指针进行size的计算

```c
#include <stdio.h>

typedef struct test_s
{
    int a;
    int b;
    int c;
}test;

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

int main()
{
    test temp;
    temp.a = 1;
    temp.b = 3;
    temp.c = 4;
    printf("&temp = %p\n", &temp);
    printf("&temp.b = %p\n", &temp.b);
    printf("&((test *)0)->b = %d\n", ((int)&((test *)0)->c));
    printf("offsetof(test, b) = %d\n", offsetof(test, c));
    return 0;
}
```

上述代码的输出为:

```
&temp = 0x7ffc3fcd6bf0
&temp.c = 0x7ffc3fcd6bf4
&((test *)0)->c = 8
offsetof(test, c) = 8
```

结构体test有三个变量: a,b,c。变量全部为int类型占4字节(无论32位还是64位)。

所以结构体总长度为12字节。因此，若声明了一个结构体`test temp`,temp.a的起始地址为0，temp.b的起始地址为4，temp.c的起始位置为8。

由输出可以知道**offsetof宏的作用即为得到size。**

`&((test *)0)->c`的作用就是求结构体的字段(field)`c`到结构体temp起始地址的字节数大小(即size)。在这里0被强制转化为`test *`型， 它的作用是作为**指向该结构体起始地址的指针**，而`&((struct test *)0)->c` 的作用便是**求k到该起始指针的字节数**。**其实是求相对地址，起始地址为0，则&k的值便是size大小**（注：打印时因为需要整型，所以有个int强转）所以便可以求出需要的size了 。

## 2.container_of实现详解

完整代码在 [on github](../../src/list_example/)

下面是示例代码片段:

`list_entry`即是`container_of`,把其改为`container_of`效果相同。

```c
int main()
{
  // 略
  struct list_head *cursor;
  list_for_each(cursor, &arp_table) {
    arp_entry_s *tmp = list_entry(cursor, arp_entry_s, entry);
    printf("ip is: %d\n",tmp->ip);
  }
  // 略
}
```

通过`container_of`成功获取到了arp_entry_s实例。

那么container_of核心为地址计算`(type *)( (char *)__mptr - offsetof(type,member) );})`,将变量的地址`(__mptr)`减去偏移地址`offsetof`就得到了宿主地址。

所以其实container_of可以简化为:

```c
#define container_of(ptr, type, member) ({
    (type *)( (char *)ptr - ((size_t)&((type  *)0)->member )); \
})
```

## 3.差错处理

讲解完了核心代码还剩下一行代码。

下面是宏原型。

```c
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
```

```c
const typeof( ((type *)0)->member ) *__mptr = (ptr);
```

首先`typeof( ((type *)0)->member )`的作用是获取member的类型。

然后 `*__mptr = (ptr);`作用是指针赋值，把`ptr`的值赋给`__mptr`。

这段代码的作用是当开发者输入的参数有问题: ptr与member类型不匹配，编译时会有warning。

## 总结

1. 判断ptr 与 member 是否为同意类型
2. 计算size大小，结构体的起始地址 = (type *)((char *)ptr - size) (注：强转为该结构体指针)

**container_of()的作用就是通过一个结构变量中一个成员的地址找到这个结构体变量的首地址。**

**container_of(ptr,type,member),这里面有ptr,type,member分别代表指针、类型、成员。**


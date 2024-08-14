# 内核链表

链表是Linux内核中最简单、最常用的一种数据结构。与数组相比，链表中可以动态插入或删除元素，在编译时不必知道要创建的元素个数。也因为链表中每个元素的创建时间各不相同，因此，它们在内存无需占用连续的内存单元，因为单元的不连续，因此各元素需要通过某种方式被链接在一起，于是，每个元素都包含一个指向下一个元素的指针，当有元素加入链表或者从链表中删除元素时，只需要调整下一个节点的指针就可以了。

链表的前置的关键宏讲解在上一章 [container_of详解](container_of.md)。

# 内核链表使用

首先要明确内核链表是侵入式链表。同时其为双向链表，其代码在include/linux/list.h中。

与此同时我将这部分代码抽到了用户态上 [list](../../src/list_example/) ,该文件夹中就是用内核list实现arp管理的一个例子。怎么使用使用代码在文件夹中的`main.c`中。


对于C语言来说双向链表一般采用如下格式:

```c
struct fox {
    unsigned long tail_length; /* 尾巴长度，以厘米为单位 */
    unsigned long weight;      /* 重量，以千克为单位 */
    bool is_fantastic;         /* 这只狐狸奇妙吗? */
    struct fox *next;          /* 指向下一只狐狸 */
    struct fox *prev;          /* 指向前一只狐狸 */
};
```

这种方法有一个缺陷，如果要生成一个rabbit链表又要重复声明`next`和`prev`。

这里要明确C语言之所以高效就在于能够轻便的对地址进行直接访问。

如果结构体的field可以通过结构体地址 + 偏移量得到。反之，知道field和偏移量可以知道结构体的地址。

因此内核链表采取了一种特殊的实现方式，把链表结点塞入其他数据结构。

如果要定义一个fox链表，如下操作即可:

```c
struct fox {
    unsigned long tail_length; /* 尾巴长度，以厘米为单位 */
    unsigned long weight;      /* 重量，以千克为单位 */
    bool is_fantastic;         /* 这只狐狸奇妙吗? */
    struct list_head list;     /* 所有 fox 结构体形成链表 */
};

// 定义链表头部
LIST_HEAD(fox_list);
```

## 链表结构体

linux内核链表仅有一个结构体`list_head`，存放了前驱和后继。

```c
struct list_head {
	struct list_head *next, *prev;
};
```

## 链表接口

下面讲解内核链表的对应接口。

### 声明和初始化

Linux内核中只定义了链表节点, 并没有专门定义链表头, 那么一个链表结构是如何建立起来的呢？答`LIST_HEAD()` 宏：

```c
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)
```

当用`LIST_HEAD(nf_sockopts)` 声明一个名为 `nf_sockopts` 的链表头时, 它的 `next`, `prev` 指针都初始化为指向其自己.
这样, 就产生一个空链表(Linux内核中用头指针的 next 是否指向自身来判断链表是否为空)

```c
static inline int list_empty(const struct list_head * head)
{
    return head->next == head;
}
```

除了用 `LIST_HEAD()` 宏在声明的时候初始化一个链表以外, `INIT_LIST_HEAD` 宏用于运行时初始化链表

```c
#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)
```

使用时`INIT_LIST_HEAD(&nf_sockopts) `

### 插入

对链表的插入操作有两种
    * 在表头插入
 
```c
static inline void list_add(struct list_head * new, struct list_head * head);
```

   * 在表尾插入

```c
static inline void list_add_tail(struct list_head * new, struct list_head * head);
```

因为链表是循环表, 且表头的`next`, `prev` 分别指向链表中的第一个和最末一个节点, 所以 `list_add` 和 `list_add_tail` 的区别并不大.
实际上二者分别实现为

```c
__list_add(new, head, head->next);
__list_add(new, head->prev, head);
static inline void __list_add(struct list_head * new,
                              struct list_head * prev,
                              struct list_head * next)
{
      next->prev = new;
      new->next = next;
      new->prev = prev;
      prev->next = new;
}

```

可见: 在表头插入是插入在 head 之后, 而在表尾插入是插入在 head->prev 之后.

添加一个新 nf_sockopt_ops 结构变量 new_sockopt 到 nf_sockopts 链表头

```c
list_add(&new_sockopt.list, &nf_sockopts);
```

`nf_sockopts`链表中记录的并不是`new_sockopt`的地址, 而是其中的`list`元素的地址


可见, list建立好后将形如以下的结构:

![list结构图](resource/kernel_list_example.png)


### 移动

将原本属于一个链表的节点移动到另一个链表的操作，并根据插入到新链表的位置分为两类：

```c
static inline void list_move(struct list_head * list, struct list_head * head);
static inline void list_move_tail(struct list_head * list, struct list_head * head);
```
例如

```c
list_move(&new_sockopt.list, &nf_sockopts); /*把 new_sockopt 从它所在的链表上删除, 并将其再链入 nf_sockopts 的表头*/
```

### 合并

整个链表的插入

```c
static inline void list_splice(struct list_head * list, struct list_head * head);
```

假设当前有两个链表, 表头分别是 list1 和 list2, 当调用 `list_splice(&list1,&list2)` 时,
只要 list1 非空，list1 链表的内容将被挂接在 list2 链表上, 位于 list2 和 list2.next (原 list2 表的第一个节点) 之间/
新 list2 链表将以原 list1 表的第一个节点为首节点, 而尾节点不变.

当 list1 被挂接到 list2 之后, 为原表头指针的 list1 的 next、prev 仍然指向原来的节点.
为了避免引起混乱

```c
static inline void list_splice_init(struct list_head * list, struct list_head * head);
```

在将 list 合并到 head 链表的基础上, 调用 INIT_LIST_HEAD(list) 将 list 设置为空链

### 由链表节点得到数据项

`list_entry(ptr, type, member)`宏
   * ptr 是指向该数据中 list_head 成员的指针, 也就是存储在链表中的地址值
   * type 是数据项的类型
   * member 则是数据项类型定义中 list_head 成员的变量名

```c
#define list_entry(ptr, type, member) container_of(ptr, type, member)
/*include/linux/kernel.h*/
#define container_of(ptr, type, member) ({                   \
        const typeof( ((type * )0)->member ) * ____mptr = (ptr); \
        (type * )( (char * ) ____mptr - offsetof(type,member) );})
/*include/linux/stddef.h*/
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE * )0)->MEMBER)
```

**总结**: 先求得结构成员在该结构中的偏移量，然后根据成员变量的地址反过来得出属主结构变量的地址

### 遍历

```c
/*include/linux/list.h*/
#define list_for_each(pos, head) \
        for (pos = (head)->next, prefetch(pos->next); pos != (head); \
        pos = pos->next, prefetch(pos->next))
```

可以看出:`list_for_each`使用了辅助指针pos, pos是 **从第一节点开始的，并没有访问头节点**,
直到pos到达头节点指针head的时候结束。

示例
```c
struct list_head *i;
list_for_each(i, &nf_sockopts) {
    struct nf_sockopt_ops * ops = list_entry(i, struct nf_sockopt_ops, list);
}
```
```c
struct nf_sockopt_ops *ops;
list_for_each_entry(ops, &nf_sockopts, list) {
 /*code*/
}
```

### 其它遍历

某些应用需要反向遍历链表，Linux 提供了 list_for_each_prev() 和 list_for_each_entry_reverse() 来完成这一操作，使用方法和上面介绍的 list_for_each()、list_for_each_entry() 完全相同。

如果遍历不是从链表头开始，而是从已知的某个节点 pos 开始，则可以使用 list_for_each_entry_continue(pos, head, member)。有时还会出现这种需求，即经过一系列计算后，如果 pos 有值，则从 pos 开始遍历，如果没有，则从链表头开始，为此，Linux 专门提供了一个 list_prepare_entry(pos, head, member) 宏，将它的返回值作为 list_for_each_entry_continue() 的 pos 参数，就可以满足这一要求。

### 释放

```c
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
     next->prev = prev;
     prev->next = next;
}

static inline void list_del(struct list_head * entry)
{
     __list_del(entry->prev, entry->next);
     entry->next = LIST_POISON1;
     entry->prev = LIST_POISON2;
}
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

```

使用`list_for_each_safe`遍历并 list_del 节点后 **free**/**kfree**

# 完整示例

[main.c](../../src/list_example/main.c)
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/delay.h>

// 主要功能:1. 在内核中申请一个页面 2. 利用内核提供的函数按照寻页的步骤一步步查询各级页目录最终找到对应的物理地址(手动模拟MMU单元寻页过程)
static unsigned long cr0,cr3;

static unsigned long vaddr = 0;

// 打印页机制中的一些重要参数
static void get_pgtable_macro(void)
{
    cr0 = read_cr0(); // 
    cr3 = read_cr3_pa(); // 获取cr3寄存器

    printk("cr0 = 0x%lx, cr3 = 0x%lx\n",cr0,cr3);
    // 这写宏用来指示线性地址中 ，线性地址中相应字段所能映射区域大小的对数的
    printk("PGDIR_SHIFT = %d\n", PGDIR_SHIFT); // (4kb,2的12次方)page offset字段所能映射的区域大小的对数。(该字段映射的是一个页面的大小)
    printk("P4D_SHIFT = %d\n",P4D_SHIFT);
    printk("PUD_SHIFT = %d\n", PUD_SHIFT);
    printk("PMD_SHIFT = %d\n", PMD_SHIFT);
    printk("PAGE_SHIFT = %d\n", PAGE_SHIFT)	;
 	// 指示相应的页目录表中项的个数的
    printk("PTRS_PER_PGD = %d\n", PTRS_PER_PGD);
    printk("PTRS_PER_P4D = %d\n", PTRS_PER_P4D);
    printk("PTRS_PER_PUD = %d\n", PTRS_PER_PUD);
    printk("PTRS_PER_PMD = %d\n", PTRS_PER_PMD);
    printk("PTRS_PER_PTE = %d\n", PTRS_PER_PTE);
    printk("PAGE_MASK = 0x%lx\n", PAGE_MASK); // 页内偏移掩码
}
 // 线性地址到物理地址转换
static unsigned long vaddr2paddr(unsigned long vaddr)
{	// 为每一个页目录项创建一个变量来将他们保存
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long paddr = 0;
    unsigned long page_addr = 0;
    unsigned long page_offset = 0;
    pgd = pgd_offset(current->mm,vaddr); // 传入当前进程的mm_struct结构。可以用当前进程页表是因为所有进程都共享同一个内核页表
    printk("pgd_val = 0x%lx, pgd_index = %lu\n", pgd_val(*pgd),pgd_index(vaddr));
    if (pgd_none(*pgd)){
        printk("not mapped in pgd\n");
        return -1;
    }
    // 页全局目录作为参数传入到p4d_offset函数中,找到了p4d
    p4d = p4d_offset(pgd, vaddr);
    printk("p4d_val = 0x%lx, p4d_index = %lu\n", p4d_val(*p4d),p4d_index(vaddr));
    if(p4d_none(*p4d))
    { 
        printk("not mapped in p4d\n");
        return -1;
    }
    // 找到pud
    pud = pud_offset(p4d, vaddr);
    printk("pud_val = 0x%lx, pud_index = %lu\n", pud_val(*pud),pud_index(vaddr));
    if (pud_none(*pud)) {
        printk("not mapped in pud\n");
        return -1;
    }
 	// 找到pmd页表项
    pmd = pmd_offset(pud, vaddr);
    printk("pmd_val = 0x%lx, pmd_index = %lu\n", pmd_val(*pmd),pmd_index(vaddr));
    if (pmd_none(*pmd)) {
        printk("not mapped in pmd\n");
        return -1;
    }
 	// 最终找到页表
    pte = pte_offset_kernel(pmd, vaddr);
    printk("pte_val = 0x%lx, ptd_index = %lu\n", pte_val(*pte),pte_index(vaddr));

    if (pte_none(*pte)) {
        printk("not mapped in pte\n");
        return -1;
    }
    // 页表中取出,该页表所映射页框的物理地址。
    page_addr = pte_val(*pte) & PAGE_MASK; // 将其与PAGE_MASK变量进行位或的操作,取出其高48位。得到页框的物理地址
    page_offset = vaddr & ~PAGE_MASK; // 取出页偏移地址级线性地址中的低12位。PAGE_MASK按位取反与vaddr位或操作
    paddr = page_addr | page_offset; // 将其拼接起来就得到了物理地址
    printk("page_addr = %lx, page_offset = %lx\n", page_addr, page_offset);
    printk("vaddr = %lx, paddr = %lx\n", vaddr, paddr);
    return paddr;
}
// 内核模块的注册
static int __init v2p_init(void)
{
    unsigned long vaddr = 0 ;
    printk("vaddr to paddr module is running..\n");
    get_pgtable_macro();
    printk("\n");
    /*
    * 64位系统中没有高端内存概念,线性映射已经可以将所有的物理内存映射到线性地址空间了。64位的ZONE_HIGHMEM区总是为空的
    */
    vaddr = __get_free_page(GFP_KERNEL); // 在内核的ZONE_NORMAL中申请了一块页面,GFP_KERNEL表示优先从ZONE_NORMAL申请页框
    if (vaddr == 0) {
        printk("__get_free_page failed..\n");
        return 0;
    }
    sprintf((char *)vaddr, "hello world from kernel"); // 写入一段话
    printk("get_page_vaddr=0x%lx\n", vaddr);
    vaddr2paddr(vaddr); // 线性地址到物理地址转换
    ssleep(600);
    return 0;
}
static void __exit v2p_exit(void) // 内核模块卸载
{
    printk("vaddr to paddr module is leaving..\n");
    free_page(vaddr);
}


module_init(v2p_init);
module_exit(v2p_exit);
MODULE_LICENSE("GPL"); 

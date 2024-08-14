

#include "list.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ARP_ENTRY_STATUS_DYNAMIC    0
#define ARP_ENTRY_STATUS_STATIC     1

// arp lists
LIST_HEAD(arp_table);
// arp表中实例结构,通过指针链成双向链表最为arp表
typedef struct arp_entry {
  uint32_t ip;                          // ip地址
  uint8_t hw_addr[6];  // mac地址

  uint8_t type;                         // 类型(动态还是静态)

  // 缺少index字段，为了简单去掉

  struct list_head entry; // 前驱 next, 后继是 prev->都在此结构中
} arp_entry_s;

// 创建链表头结点
uint8_t* ht_get_dst_macaddr(uint32_t dip) {
  struct list_head *cursor;
  list_for_each(cursor, &arp_table) {
    arp_entry_s *tmp = list_entry(cursor, arp_entry_s, entry);
    if (dip == tmp->ip) { // dip在表中被查到则找到了
      return tmp->hw_addr;
    }
  }
  return NULL;
}

int main()
{
  arp_entry_s *fir,*sec,*third;

  fir = (arp_entry_s*)malloc(sizeof(arp_entry_s));
  sec = (arp_entry_s*)malloc(sizeof(arp_entry_s));
  third = (arp_entry_s*)malloc(sizeof(arp_entry_s));

  fir->ip = 1;
  fir->hw_addr[0] = 1;
  fir->hw_addr[1] = 1;
  fir->hw_addr[2] = 1;
  fir->hw_addr[3] = 1;
  fir->hw_addr[4] = 1;
  fir->hw_addr[5] = 1;
  fir->type = 0;
  list_add_tail(&fir->entry, &arp_table);

  sec->ip = 2;
  sec->hw_addr[0] = 1;
  sec->hw_addr[1] = 1;
  sec->hw_addr[2] = 1;
  sec->hw_addr[3] = 4;
  sec->hw_addr[4] = 3;
  sec->hw_addr[5] = 1;
  sec->type = 0;
  list_add_tail(&sec->entry, &arp_table);

  third->ip = 3;
  third->hw_addr[0] = 1;
  third->hw_addr[1] = 2;
  third->hw_addr[2] = 0;
  third->hw_addr[3] = 5;
  third->hw_addr[4] = 3;
  third->hw_addr[5] = 1;
  third->type = 1;
  list_add_tail(&third->entry, &arp_table);

  struct list_head *cursor;
  list_for_each(cursor, &arp_table) {
    arp_entry_s *tmp = list_entry(cursor, arp_entry_s, entry);
    printf("ip is: %d\n",tmp->ip);
  }

  uint8_t *dstmac = ht_get_dst_macaddr(2);
  printf("%d %d %d %d %d %d\n", *dstmac, *(dstmac + 1), *(dstmac + 2), *(dstmac + 3), *(dstmac + 4), *(dstmac + 5));
  free(fir);
  free(sec);
  free(third);
  return 0;
}
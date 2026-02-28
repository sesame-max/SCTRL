/*
 * maz_cpnt_list.c
 *
 *  Created on: 2020年2月24日
 *      Author: wangbing
 *      Email : mz8023yt@163.com
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "maz_cpnt_list.h"
#include "stddef.h"

/**
 * @brief 初始化链表头
 */
void MAZ_CPNT_list_init(struct list_head *head)
{
    head->next = head;
    head->prev = head;
}

/**
 * @brief 内部接口, 添加节点到链表中
 */
void __list_add(struct list_head *new_node, struct list_head *prev_node, struct list_head *next_node)
{
    next_node->prev = new_node;
    new_node->next = next_node;
    new_node->prev = prev_node;
    prev_node->next = new_node;
}

/**
 * @brief 添加节点到链表头部
 */
void MAZ_CPNT_list_add(struct list_head *new_node, struct list_head *head)
{
    __list_add(new_node, head, head->next);
}

/**
 * @brief 添加节点到链表尾部
 */
void MAZ_CPNT_list_add_tail(struct list_head *new_node, struct list_head *head)
{
    __list_add(new_node, head->prev, head);
}

/**
 * @brief 添加节点到链表特定位置
 */
void MAZ_CPNT_list_insert(struct list_head *new_node, struct list_head *prev_node, struct list_head *next_node)
{
    __list_add(new_node, prev_node, next_node);
}

/**
 * @brief 内部接口, 删除链表中的节点
 */
void __list_del(struct list_head *prev_node, struct list_head *next_node)
{
    next_node->prev = prev_node;
    prev_node->next = next_node;
}

/**
 * @brief 删除链表中的节点
 */
void MAZ_CPNT_list_del(struct list_head *node)
{
    __list_del(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
}

/**
 * @brief 替换链表中的节点
 */
void MAZ_CPNT_list_replace(struct list_head *old_node, struct list_head *new_node)
{
    new_node->next = old_node->next;
    new_node->next->prev = new_node;
    new_node->prev = old_node->prev;
    new_node->prev->next = new_node;
}

/**
 * @brief 判断链表是否为空
 */
int MAZ_CPNT_list_is_empty(const struct list_head *head)
{
    struct list_head *next = head->next;
    return (next == head) && (next == head->prev);
}

#ifdef __cplusplus
}
#endif

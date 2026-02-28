/*
 * list.h
 *
 *  Created on: 2020年2月24日
 *      Author: wangbing
 *      Email : mz8023yt@163.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAZ_LIST_H_
#define MAZ_LIST_H_

/* Offset of member MEMBER in a struct of type TYPE. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

/**
 * @brief:  container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) ); })

/**
 * @brief:  return value for list_is_empty func
 */
#define MAZCPNT_LIST_EMPTY     1
#define MAZCPNT_LIST_NOT_EMPTY 0

/**
 * @brief 通用链表节点
 */
struct list_head {
	struct list_head* next;
	struct list_head* prev;
};

void list_init(struct list_head* head);
void list_add(struct list_head* new_node, struct list_head* head);
void list_add_tail(struct list_head* new_node, struct list_head* head);
void list_insert(struct list_head* new_node, struct list_head* prev_node, struct list_head* next_node);
void list_del(struct list_head* node);
void list_replace(struct list_head* old_node, struct list_head* new_node);
int list_is_empty(const struct list_head* head);

#endif /* MAZ_LIST_H_ */

#ifdef __cplusplus
}
#endif

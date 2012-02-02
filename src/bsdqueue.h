/*
 * +-----------------------------------------------------------------------+
 * | Generic Single-Linked-List                                            |
 * |                                                                       |
 * | Migrates from FreeBSD 6.2 Kernel Source Code.                         |
 * +-----------------------------------------------------------------------+
 * | Author: jingmi@gmail.com                                              |
 * +-----------------------------------------------------------------------+
 *
 * $Id: bsdqueue.h,v 1.5 2008/10/27 07:42:34 jingmi Exp $
 *
 * Refactored code(2012/01/15 05:29):
 * 1. Only STAILQ used
 * 2. Use __typeof__ to remove the need for item structure type
 * 3. Add testsuites
 */

#ifndef _BSDQUEUE_H_
#define	_BSDQUEUE_H_

#include <fcntl.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <pthread.h>

#ifndef __offset_of
#define	__offset_of(type, field)	((size_t)(&((type *)0)->field))
#endif

#ifndef mem_calloc
  #ifdef SLAB_CALLOC
    #define mem_calloc slab_calloc
  #else
    #define mem_calloc malloc
  #endif
#endif

#ifndef mem_alloc
  #ifdef SLAB_ALLOC
    #define mem_alloc slab_alloc
  #else
    #define mem_alloc malloc
  #endif
#endif

#ifndef mem_free
  #ifdef SLAB_FREE
    #define mem_free slab_free
  #else
    #define mem_free free
  #endif
#endif

/* ==================== USER INTERFACE ==================== */

/* single linked list */
#define list_head_name(name) name##_Head_S
#define list_item_name(name) name##_Item_S

#define list_head_ptr(listname) struct list_head_name(listname) *
#define list_item_ptr(listname) struct list_item_name(listname) *
#define list_next_ptr(listname) STAILQ_ENTRY(list_item_name(listname)) LINK_NEXT_PTR

#define list_def(listname) \
    STAILQ_HEAD(list_head_name(listname), list_item_name(listname));

#define list_new(listname, head_ptr) \
    struct list_head_name(listname) *head_ptr = mem_calloc(sizeof(struct list_head_name(listname))); \
    STAILQ_INIT(head_ptr)

#define list_item_new(listname, item_ptr) \
    struct list_item_name(listname) *item_ptr = mem_alloc(sizeof(struct list_item_name(listname)));

#define list_item_init(listname, item_ptr) \
    item_ptr = mem_alloc(sizeof(struct list_item_name(listname)));

#define list_lock(head_ptr) do { pthread_mutex_lock(&((head_ptr)->lock)); } while (0)
#define list_unlock(head_ptr) do { pthread_mutex_unlock(&((head_ptr)->lock)); } while (0)

#define list_size(head_ptr) ((*(head_ptr)).size)

#define list_push(head_ptr, item_ptr)  STAILQ_INSERT_TAIL(head_ptr, item_ptr, LINK_NEXT_PTR)
#define list_unshift(head_ptr, item_ptr) STAILQ_INSERT_HEAD(head_ptr, item_ptr, LINK_NEXT_PTR)
#define list_insert(head_ptr, pos_ptr, item_ptr) STAILQ_INSERT_AFTER(head_ptr, pos_ptr, item_ptr, LINK_NEXT_PTR)
#define list_shift(head_ptr, item_ptr)    do {    \
    item_ptr = list_first(head_ptr);    \
    list_remove_head(head_ptr);         \
} while (0)
#define list_pop(head_ptr, item_ptr)    do {    \
    item_ptr = list_last(head_ptr);    \
    list_remove(head_ptr, item_ptr);   \
} while (0)

#define list_first(head_ptr) STAILQ_FIRST(head_ptr)
#define list_last(head_ptr) STAILQ_LAST(head_ptr, LINK_NEXT_PTR)

#define list_foreach(head_ptr, item_ptr)    STAILQ_FOREACH(item_ptr, head_ptr, LINK_NEXT_PTR)
#define list_foreach_safe(head_ptr, item_ptr, temp_ptr) STAILQ_FOREACH_SAFE(item_ptr, head_ptr, LINK_NEXT_PTR, temp_ptr)
#define list_next(item_ptr) STAILQ_NEXT(item_ptr, LINK_NEXT_PTR)
#define list_addr(head_ptr, n, item_ptr)    do { \
    int m = (int)(n);                            \
    if (m >= (int)list_size(head_ptr)) {         \
        item_ptr = NULL;    break;  \
    }                               \
    item_ptr = list_first(head_ptr);    \
    while (--m >= 0) {           \
        item_ptr = list_next(item_ptr); \
    }   \
} while (0)

#define list_empty(head_ptr) STAILQ_EMPTY(head_ptr)

#define list_cat(head1, head2)  STAILQ_CONCAT(head1, head2)

#define list_remove(head_ptr, item_ptr) STAILQ_REMOVE(head_ptr, item_ptr, LINK_NEXT_PTR)
#define list_remove_head(head_ptr)  STAILQ_REMOVE_HEAD(head_ptr, LINK_NEXT_PTR)

#define list_delete(head_ptr, item_ptr) do {    \
    list_remove(head_ptr, item_ptr);            \
    mem_free(item_ptr);                         \
} while(0)

#define list_delete_head(head_ptr)    do{ \
    void *first_ptr = (void *)list_first(head_ptr);  \
    list_remove_head(head_ptr);             \
    mem_free(first_ptr);               \
} while (0)

#define list_destroy(head_ptr) do {    \
    while (list_first(head_ptr) != NULL)    {           \
        list_delete_head(head_ptr);                    \
    }                                                   \
    pthread_mutex_destroy(&((head_ptr)->lock));         \
    mem_free(head_ptr);                                \
} while (0)

#define list_clear(head_ptr) do {    \
    while (list_first(head_ptr) != NULL)    {           \
        list_delete_head(head_ptr);                    \
    }                                                   \
} while (0)

/* ==================== SYSTEM IMPLEMENTATION ==================== */

/*
 * Singly-linked Tail queue declarations.
 */
#define	STAILQ_HEAD(name, type)						\
struct name {								\
    struct type *stqh_first;/* first element */			\
    struct type **stqh_last;/* addr of last next element */		\
    pthread_mutex_t lock;                                       \
    unsigned int size;                                         \
}

#define	STAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).stqh_first, PTHREAD_MUTEX_INITIALIZER, 0,}

#define	STAILQ_ENTRY(type)						\
struct {								\
	struct type *stqe_next;	/* next element */			\
}

/*
 * Singly-linked Tail queue functions.
 */
#define	STAILQ_CONCAT(head1, head2) do {				\
	if (!STAILQ_EMPTY((head2))) {					\
		*(head1)->stqh_last = (head2)->stqh_first;		\
		(head1)->stqh_last = (head2)->stqh_last;		\
        (head1)->size += (head2)->size;         \
		STAILQ_INIT((head2));					\
	}								\
} while (0)

#define	STAILQ_EMPTY(head)	((head)->stqh_first == NULL)

#define	STAILQ_FIRST(head)	((head)->stqh_first)

#define	STAILQ_FOREACH(var, head, field)				\
	for((var) = STAILQ_FIRST((head));				\
	   (var);							\
	   (var) = STAILQ_NEXT((var), field))


#define	STAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = STAILQ_FIRST((head));				\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	STAILQ_INIT(head) do {						\
    STAILQ_FIRST((head)) = NULL;					\
    (head)->stqh_last = &STAILQ_FIRST((head));			\
    pthread_mutex_init(&(head)->lock, NULL);            \
    (head)->size = 0;                               \
} while (0)

#define	STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {		\
	if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL)\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_NEXT((tqelm), field) = (elm);				\
    (head)->size ++;                           \
} while (0)

#define	STAILQ_INSERT_HEAD(head, elm, field) do {			\
	if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)	\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_FIRST((head)) = (elm);					\
    (head)->size ++;                           \
} while (0)

#define	STAILQ_INSERT_TAIL(head, elm, field) do {			\
	STAILQ_NEXT((elm), field) = NULL;				\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &STAILQ_NEXT((elm), field);			\
    (head)->size ++;                           \
} while (0)

#define	STAILQ_LAST(head, field)					\
	(STAILQ_EMPTY((head)) ?						\
		NULL :							\
	        ((__typeof__(head->stqh_first))					\
		((char *)((head)->stqh_last) - __offset_of(__typeof__(*(head->stqh_first)), field))))

#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#define	STAILQ_REMOVE(head, elm, field) do {			\
	if (STAILQ_FIRST((head)) == (elm)) {				\
		STAILQ_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		__typeof__(head->stqh_first) curelm = STAILQ_FIRST((head));		\
		while (STAILQ_NEXT(curelm, field) != (elm))		\
			curelm = STAILQ_NEXT(curelm, field);		\
		if ((STAILQ_NEXT(curelm, field) =			\
		     STAILQ_NEXT(STAILQ_NEXT(curelm, field), field)) == NULL)\
			(head)->stqh_last = &STAILQ_NEXT((curelm), field);\
        (head)->size --;                                        \
	}								\
} while (0)

#define	STAILQ_REMOVE_HEAD(head, field) do {				\
	if ((STAILQ_FIRST((head)) =					\
	     STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)		\
		(head)->stqh_last = &STAILQ_FIRST((head));		\
        (head)->size --;                       \
} while (0)

#define	STAILQ_REMOVE_HEAD_UNTIL(head, elm, field) do {			\
	if ((STAILQ_FIRST((head)) = STAILQ_NEXT((elm), field)) == NULL)	\
		(head)->stqh_last = &STAILQ_FIRST((head));		\
} while (0)

#endif /* !_SYS_QUEUE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */

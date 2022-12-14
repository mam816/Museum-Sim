#ifndef _LINUX_CS1550_H
#define _LINUX_CS1550_H

#include <linux/list.h>

/**
 * A generic semaphore, providing serialized signaling and
 * waiting based upon a user-supplied integer value.M
 */
struct cs1550_sem
{
	/* Current value. If nonpositive, wait() will block */
	long value;

	/* Sequential numeric ID of the semaphore */
	long sem_id;

	/* Per-semaphore lock, serializes access to value */
	spinlock_t lock;

     //head of the semaphore list
	struct list_head list;

     // head of waiting queue
	struct list_head waiting_tasks;
};

//struct list_head
//{
//	struct list_head *next, *prev;
//};

struct cs1550_task
{
	struct list_head list; //task list head
	struct task_struct *task; 
};
#endif

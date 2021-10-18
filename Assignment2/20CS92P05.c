/*
Assignment 2
------------------------------------------
Submitted By Argha Sen 20CS92P05
------------------------------------------
Kernel Version : 5.6.9-generic
OS : Ubuntu 20.04 LTS
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h> 
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>

#define DEVICE_NAME "cs60038_a2_20CS92P05"
#define current get_current()

#define PB2_SET_CAPACITY 	_IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_RIGHT 	_IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_LEFT 	_IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO 		_IOR(0x10, 0x34, int32_t*)
#define PB2_EXTRACT_LEFT 	_IOR(0x10, 0x35, int32_t*)
#define PB2_EXTRACT_RIGHT 	_IOR(0x10, 0x36, int32_t*)

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Argha Sen");

static DEFINE_MUTEX(mutex_val);

struct obj_info {
	int32_t deque_size; // current number of elements in deque
	int32_t capacity; // maximum capacity of the deque
};

struct deque {
	int32_t *arr;
	int32_t count;
	int32_t capacity;
};
typedef struct deque deque;

struct h_struct {
	int 				key;
	deque* 				global_deque;
	struct h_struct* 	next;
};
struct h_struct *htable, *entry;


/* Functions*/
/* Deque methods */

static deque* 	CreateDeque(int32_t capacity);
static deque* 	DestroyDeque(deque* dq);
static int32_t 	InsertRight(deque* dq, int32_t item);
static int32_t 	InsertLeft(deque* dq, int32_t item);
static int32_t 	PopFromLeft(deque *dq);
static int32_t 	PopFromRight(deque *dq);

/*  Methods for handling concurrency and separate data from multiple processes */
static struct h_struct* get_entry(int key);
static void 			key_add(struct h_struct*);
static void 			DestroyHashTable(void);
static void 			key_del(int key);
static void 			print_key(void);

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int  dev_open(struct inode *, struct file *);
static int  dev_release(struct inode *, struct file *);


static int    			numberOpens = 0;
static int32_t 			num;
static int32_t 			size_of_deque;
static int 				args_set = 0;
static int32_t 			retval = -1;
static struct obj_info 	deque_info;

static struct proc_ops file_ops =
{
	.proc_open = 	dev_open,
	.proc_release = dev_release,
	.proc_ioctl = 	dev_ioctl,
};

/* Add a new process with pid to the linked list */
static void key_add(struct h_struct* entry) {
	entry->next = htable->next;
	htable->next = entry;
}

/* Return a process with a specific pid */
static struct h_struct* get_entry(int key) {
	struct h_struct* temp = htable->next;
	while (temp != NULL) {
		if (temp->key == key) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

/* Deletes a process entry */
static void key_del(int key) {
	struct h_struct *prev, *temp;
	prev = temp = htable;
	temp = temp->next;
	
	while (temp != NULL) {
		if (temp->key == key) {
			prev->next = temp->next;
			temp->global_deque = DestroyDeque(temp->global_deque);
			temp->next = NULL;
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}

/* Prints all the process pid */
static void print_key(void){
	struct h_struct *temp;
	temp = htable->next;
	while (temp != NULL) {
		temp = temp->next;
	}
}

/* Destroy Linked List of porcesses */
static void DestroyHashTable(void) {
	struct h_struct *temp, *temp2;
	temp = htable->next;
	htable->next = NULL;
	while (temp != NULL) {
		temp2 = temp;
		temp = temp->next;
		kfree(temp2);
	}
	kfree(htable);
}

// populates the result, whose pointer is passed while making the call
struct result {
	int32_t result; // value of left/right most element extracted.
	int32_t deque_size; // size of the deque after extracting.
};

/* Create DeQue */
static deque *CreateDeque(int32_t capacity) {
	deque *dq = (deque * ) kmalloc(sizeof(deque), GFP_KERNEL);

	//check if memory allocation is fails
	if (dq == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error in allocating DeQue!", current->pid);
		return NULL;
	}
	dq->count = 0;
	dq->capacity = capacity;
	dq->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes

	//check if allocation succeed
	if ( dq->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating deque->arr!", current->pid);
		return NULL;
	}
	return dq;
}

/* Destroy DeQue */
static deque* DestroyDeque(deque* dq) {
	if (dq == NULL)
		return -EACCES; // Deque is not allocated
	printk(KERN_INFO DEVICE_NAME ": PID %d, %ld bytes of deque Space freed.\n", current->pid, sizeof(dq->arr));
	kfree_const(dq->arr);
	kfree_const(dq);
	return 0;
}

/* Insert a number into DeQue */
static int32_t InsertRight(deque *dq, int32_t key) {
	if (dq->count > dq->capacity)
	{
		return -EACCES;
	}
	dq->arr[dq->count] = key;
	dq->count++;

	return 0;
}

static int32_t InsertLeft(deque *dq, int32_t key) {
	if (dq->count > dq->capacity)
	{
		return -EACCES;
	}
	dq->count++;
	int32_t i;
	for (i = dq->count; i > 0; i--){
		dq->arr[i] = dq->arr[i-1];
	}
	dq->arr[0] = key;

	return 0;
}

/* Extract the left node of a DeQue */
static int32_t PopFromLeft(deque *dq) {
	int32_t pop;
	if (dq->count == 0) {
		printk(KERN_INFO DEVICE_NAME ": PID %d, DeQue is Empty__\n", current->pid);
		return NULL;
	}

	pop = dq->arr[0];
	int32_t i;
	for (i = 0; i < dq->count-1; i++)
	{
		dq->arr[i] = dq->arr[i+1];
	}
	dq->count--;
	return pop;
}

/* Extract the Right node of a DeQue */
static int32_t PopFromRight(deque *dq) {
	int32_t pop;
	if (dq->count == 0) {
		printk(KERN_INFO DEVICE_NAME ": PID %d, DeQue is Empty__\n", current->pid);
		return NULL;
	}
	pop = dq->arr[dq->count - 1];
	dq->count--;
	return pop;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	switch (cmd) {
	case PB2_SET_CAPACITY:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d RAISED ERROR in dev_ioctl:PB2_SET_CAPACITY entry is non-existent", current->pid);
			return -EACCES;
		}
		
		retval = copy_from_user(&size_of_deque, (int32_t *)arg, sizeof(int32_t));
		if (retval)
			return -EINVAL;

		printk(DEVICE_NAME ": PID %d DeQue SIZE: %d", current->pid, size_of_deque);

		if (size_of_deque <= 0 || size_of_deque > 100) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of DeQue %d!!\n", current->pid, size_of_deque);
			return -EINVAL;
		}

		entry->global_deque = DestroyDeque(entry->global_deque); // destroy any existing DeQue before creating a new one
		entry->global_deque = CreateDeque(size_of_deque); // allocating space for new DeQue

		break;

	case PB2_INSERT_RIGHT:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_ioctl:PB2_INSERT_RIGHT entry is non-existent", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the DeQue is initialized(not NULL)
		args_set = (entry->global_deque) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d DeQue not initialized",current->pid);
			return -EACCES;
		}

		// Get the integer number and insert into DeQue
		retval = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(retval)
			return -EINVAL;

		printk(DEVICE_NAME ": PID %d writing %d to DeQue\n", current->pid, num);
		retval = InsertRight(entry->global_deque, num);
		if (retval < 0) { // DeQue is filled to capacity
			return -EACCES;
		}
		break;

	case PB2_INSERT_LEFT:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_ioctl:PB2_INSERT_LEFT entry is non-existent", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the DeQue is initialized(not NULL)
		args_set = (entry->global_deque) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d DeQue not initialized",current->pid);
			return -EACCES;
		}

		// Get the integer number and insert into DeQue
		retval = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(retval)
			return -EINVAL;

		printk(DEVICE_NAME ": PID %d writing %d to DeQue\n", current->pid, num);
		retval = InsertLeft(entry->global_deque, num);
		if (retval < 0) { // DeQue is filled to capacity
			return -EACCES;
		}
		break;

	case PB2_GET_INFO:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": RAISED ERROR in dev_ioctl:PB2_GET_INFO entry is non-existent %d", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the DeQue is initialized(not NULL)
		args_set = (entry->global_deque) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d DeQue not initialized",current->pid);
			return -EACCES;
		}

		// Create a structure to send to userspace
		deque_info.deque_size = entry->global_deque->count;
		deque_info.capacity = size_of_deque;

		retval = copy_to_user((struct obj_info *)arg, &deque_info, sizeof(struct obj_info));
		if (retval)
			return -EACCES;
		break;

	case PB2_EXTRACT_LEFT:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": RAISED ERROR in dev_ioctl:PB2_EXTRACT_LEFT entry is non-existent %d", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the DeQue is initialized(not NULL)
		args_set = (entry->global_deque) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d DeQue not initialized",current->pid);
			return -EACCES;
		}
		
		// populates the result, whose pointer is passed while making the call
		struct result res;
		res.result = PopFromLeft(entry->global_deque);
		res.deque_size = entry->global_deque->count;
		printk(DEVICE_NAME ": PID %d popped %d from DeQue\n", current->pid, res.result);
		printk(DEVICE_NAME ": PID %d Size of the DeQue after Pop operation at left %d\n", current->pid, res.deque_size);
		retval = copy_to_user((struct result *)arg, &res, sizeof(struct result));
		if (retval != 0 || res.result == NULL){
			printk(KERN_INFO DEVICE_NAME ": PID %d Failed to send retval : %d, topnode is %d\n", current->pid, retval, res.result);
			return -EACCES;
		}
		break;
	case PB2_EXTRACT_RIGHT:
		// Get the process corresponing DeQue
		entry = get_entry(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": RAISED ERROR in dev_ioctl:PB2_EXTRACT_RIGHT entry is non-existent %d", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the DeQue is initialized(not NULL)
		args_set = (entry->global_deque) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d DeQue not initialized",current->pid);
			return -EACCES;
		}
		
		// Create Structure to send to userspace
		struct result res2;
		res2.result = PopFromRight(entry->global_deque);
		res2.deque_size = entry->global_deque->count;
		printk(DEVICE_NAME ": PID %d popped %d from DeQue\n", current->pid, res2.result);
		printk(DEVICE_NAME ": PID %d Size of the DeQue after Pop operation at Right %d\n", current->pid, res2.deque_size);

		retval = copy_to_user((struct result *)arg, &res2, sizeof(struct result));
		if (retval != 0 || res2.result == NULL){
			printk(KERN_INFO DEVICE_NAME ": PID %d Failed to send retval : %d, topnode is %d\n", current->pid, retval, res2.result);
			return -EACCES;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}


static int dev_open(struct inode *inodep, struct file *filep) {
	// If same process has already opened the file
	if (get_entry(current->pid) != NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n", current->pid);
		return -EACCES;
	}

	// Create a new entry to the process linked list
	entry = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
	*entry = (struct h_struct) {current->pid, NULL, NULL};
	if (!mutex_trylock(&mutex_val)) {
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is at dev_open", current->pid);
		return -EBUSY;
	}
	printk(DEVICE_NAME ": PID %d !!!! Adding %d to HashTable\n", current->pid, entry->key);
	key_add(entry);

	numberOpens++;
	printk(KERN_INFO DEVICE_NAME ": PID %d Device has been opened %d time(s)\n", current->pid, numberOpens);
	print_key();
	mutex_unlock(&mutex_val);
	return 0;
}


static int dev_release(struct inode *inodep, struct file *filep) {
	if (!mutex_trylock(&mutex_val))
	{
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is Busy", current->pid);
	 	return -EBUSY;
	}
	// Delete the process entry from the process linked list
	key_del(current->pid);
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully closed\n", current->pid);
	print_key();
	mutex_unlock(&mutex_val);
	return 0;
}


static int queue_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0666, NULL, &file_ops);
	if (!entry)
		return -ENOENT;
	htable = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
	*htable = (struct h_struct) { -1, NULL, NULL};
	printk(KERN_ALERT DEVICE_NAME ": Hello world\n");
	mutex_init(&mutex_val);	
	return 0;
}

static void queue_cleanup(void) {
	mutex_destroy(&mutex_val);
	DestroyHashTable();
	remove_proc_entry(DEVICE_NAME, NULL);
	printk(KERN_INFO ":Successfully released all memory and closed\n");
	printk(KERN_ALERT DEVICE_NAME "Goodbye\n");
}

module_init(queue_init);
module_exit(queue_cleanup);


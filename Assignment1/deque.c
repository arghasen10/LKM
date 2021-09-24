/*
PART B: Assignment 1
--------------------------------
Submitted By Argha Sen 20CS92P05
--------------------------------
Kernel Version : 5.6.9-generic
OS : Ubuntu 20.04 LTS
*/


// Header Files
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include<linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/mutex.h>

// Current Process details
#define current get_current()


MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Argha Sen");


// Functions needed
static struct list* NewNode(void);
static char* PopFromLeft(void);
static void InsertLast(char *item);
static void InsertFirst(char *item);
static bool check_string(const char* string);

// Proc Functions/Callbacks
static ssize_t pop_queue(struct file *filp,char *buf,size_t count,loff_t *offp);
static ssize_t push_queue(struct file *filp,const char *buf,size_t count,loff_t *offp);
static int dev_open(struct inode *inodep, struct file *filp);
static int dev_release(struct inode *inodep, struct file *filp); 

// Initialize Global Variables
static int start_flag = 0;
static int sizeofDeque;
static int currSizeDeque = 0;
static int returnval = -1;
static int dataval;
static struct proc_dir_entry *ent;
static int frontdata;
static int ret;
static int pop_flag = 1;
static char *msg;

// Concurrency checks
static DEFINE_MUTEX(mutex_val);

// DeQue struct
struct list
{
	char *info;
	struct list*next;
}*ptr=NULL,*temp=NULL,*rear=NULL,*front=NULL;

static struct list* NewNode(void)
{
	struct list* node;
	node=(struct list*)kmalloc(sizeof(struct list), GFP_KERNEL);
	node->next=NULL;
	if(node!=NULL)
		return node;
	else
	{
		printk(KERN_ALERT "\nOverflow\n");
		return NULL;
	}
}

static char* PopFromLeft(void)   //delete at the beginning
{
	char *item;
	temp=front;
	item=temp->info;
	front=front->next;
	kfree(temp);
	currSizeDeque -=1;
	return item;
}

static void InsertLast(char *item)  //insertion at the end
{
	struct list* node=NewNode();
	node->info=item;
	if(front==NULL)
	{
		front=node;
		rear=node;
	}
	else
	{
		rear->next=node;
		rear=node;
	}
	return;
}

static void InsertFirst(char *item)
{
  struct list *node=NewNode();
  node->info=item;
  if(front==NULL)
  {
		front=node;
		rear=node;
		return;
	}
  node->next=front;
  front=node;
}

static bool check_string(const char* string) 
{
  int string_len = strlen(string);
  int i;
  for(i = 0; i < string_len; i++) 
  {
    if(!isdigit(string[i])) 
      return false;
  }
  return true;
}

static ssize_t pop_queue(struct file *filp,char *buf,size_t count,loff_t *offp ) 
{
	if(front==NULL)
	{
		printk(KERN_ALERT "\nQueue empty\n");
		return -EACCES;
	}
	if (pop_flag == 1)
	{
		msg = PopFromLeft();
		ret = strlen(msg);
		pop_flag = 0;
	}
	if (count > ret)
	{
		count = ret;
	}
	ret = ret-count;

	returnval = copy_to_user(buf,msg,count);
	if (returnval == 0)
	{	
		sscanf(msg, "%d", &frontdata);
		printk(KERN_INFO "\n data popped out from DeQue is %s and size of data read %ld bytes\n" ,msg,sizeof(frontdata));
	}
	else
	{
		printk(KERN_INFO "\nFailed to send data : %d\n", returnval);
		return -EACCES;
	}

	if (count == 0)
	{
		pop_flag = 1;
	}
	return count;
	
}

static ssize_t push_queue(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	// if(!check_string(buf))
	// 	return -EINVAL;
	if (start_flag==0)
	{
		msg = kmalloc(10*sizeof(char), GFP_KERNEL);
		if(copy_from_user(msg,buf,count))
			return -ENOBUFS;
		sscanf(msg, "%d", &sizeofDeque);
		if (sizeofDeque <= 0 || sizeofDeque > 100)
		{
			printk(KERN_ALERT ":Wrong size of Deque %d!!\n", sizeofDeque);
			return -EINVAL;
		}
		printk(KERN_INFO "\nSize of Deque set is %d\n", sizeofDeque);
		start_flag = 1;
		return count;
	}
	else
	{
		if(currSizeDeque <= sizeofDeque)
		{	
			currSizeDeque += 1;
			msg=kmalloc(10*sizeof(char),GFP_KERNEL);
			if(copy_from_user(msg,buf,count))
				return -ENOBUFS;
			sscanf(msg, "%d", &dataval);
			if(dataval % 2 == 0)
				InsertLast(msg);						//Insert Last
			else
				InsertFirst(msg);					//Insert First

			printk(KERN_INFO "\nNew data pushed to the Deque is %d",dataval);
			printk(KERN_INFO "and size of data write is %ld bytes\n", sizeof(dataval));
			return count;
		}
		else
		{
			printk(KERN_ALERT "\nWrite calls more than the actual DeQue size\n");
			return -EACCES;
		}
	}
	
}

static int dev_open(struct inode *inodep, struct file *filp) 
{
	printk(KERN_INFO "PID %d is here\n", current->pid);

	if (!mutex_trylock(&mutex_val)) {
		printk(KERN_ALERT "PID %d Device is Busy", current->pid);
		return -EBUSY;
	}
	mutex_unlock(&mutex_val);
	return 0;
}


static int dev_release(struct inode *inodep, struct file *filp) 
{
	printk(KERN_INFO "PID %d is leaving\n", current->pid);
	if (!mutex_trylock(&mutex_val))
	{
		printk(KERN_ALERT "PID %d Device is Busy", current->pid);
	 	return -EBUSY;
	}
	mutex_unlock(&mutex_val);
	return 0;
}


static struct proc_ops proc_fops = 
{
	.proc_open = dev_open,
	.proc_read= pop_queue,
	.proc_write= push_queue,
	.proc_release = dev_release,
};


static int queue_init (void) 
{
	ent=proc_create("partb_1_20CS92P05",0660,NULL,&proc_fops);
	mutex_init(&mutex_val);
	printk(KERN_ALERT "hello...\n");
	return 0;
}

static void queue_cleanup(void) 
{
	proc_remove(ent);
	mutex_destroy(&mutex_val);
	printk(KERN_INFO ":Successfully released all memory and closed\n");
}

module_init(queue_init);
module_exit(queue_cleanup);

/*
Assignment 2
------------------------------------------
Submitted By Argha Sen 20CS92P05
------------------------------------------
Kernel Version : 5.6.9-generic
OS : Ubuntu 20.04 LTS
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define PB2_SET_CAPACITY _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_RIGHT _IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_LEFT _IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x34, int32_t*)
#define PB2_EXTRACT_LEFT _IOR(0x10, 0x35, int32_t*)
#define PB2_EXTRACT_RIGHT _IOR(0x10, 0x36, int32_t*)

struct obj_info {
        int32_t deque_size; // current number of elements in deque
        int32_t capacity; // maximum capacity of the deque
};

// populates the result, whose pointer is passed while making the call
struct result {
        int32_t result; // value of left/right most element extracted.
        int32_t deque_size; // size of the deque after extracting.
};


int main(){


        int pid = fork();
        fork(); fork();

        int32_t deque_size;
        if (pid == 0)
        {
                deque_size = rand() % 15 + 1;
        }
        else
        {
                deque_size = rand() % 20 + 1;
        }
        printf("Size of the DeQue is %d\n", deque_size);
        int fd, value, number;
        fd = open("/proc/cs60038_a2_20CS92P05", O_RDWR);
        if (fd < 0){
        	printf("Error: \n");
        	return -1;
        }
        printf("File opened\n");

        int ret = ioctl(fd, PB2_SET_CAPACITY, &deque_size);
        if(ret < 0)
        {
        	printf("Initialization failed\n");
        	close(fd);
                return 0;
        }

        printf("DeQue Initialized\n");
        struct obj_info myobj_info;
        ret = ioctl(fd, PB2_GET_INFO, &myobj_info);
        struct result res;

        for (int i = 0; i < deque_size; i++)
        {
                int32_t random_number;
                if (pid == 0)
                {
                        random_number = rand() % 10 + 1; 
                }
                else
                {
                        random_number = rand() % 15 + 1;       
                }
                printf("the element to be inserted is %d\n", random_number);
                int type = rand() % 2;
                if (type == 0)
                {
                        printf("Insert Left\n");
                        ret = ioctl(fd, PB2_INSERT_LEFT, &random_number);
                }
        	if (type == 1)
                {
                        printf("Insert Right\n");
                        ret = ioctl(fd, PB2_INSERT_RIGHT, &random_number);
                }
        	if(ret < 0){
            		printf("ERROR! Write failed\n");
            		close(fd);
            		return 0;
        	}
        }

        ret = ioctl(fd, PB2_GET_INFO, (struct obj_info *) &myobj_info);
        if(ret == 0){
                   printf("\n================DeQue INFO=================\n");
        	   printf("DeQue Size: %d\n", myobj_info.deque_size);
                   printf("Max Capacity : %d\n", myobj_info.capacity);
        }
        printf("\n");

	for (int i = 0; i < deque_size; i++)
	{
        	printf("Extracting..\n");
                int type = rand() % 2;
                if (type == 0)
                {
                        printf("Extracting from left\n");
                        ret = ioctl(fd, PB2_EXTRACT_LEFT, (struct result *) &res);
                }
        	if (type == 1)
                {
                        printf("Extracting from Right\n");
                        ret = ioctl(fd, PB2_EXTRACT_RIGHT, (struct result *) &res);
                }
        	if(ret < 0){
            		printf("ERROR! Read failed\n");
            		close(fd);
            		return 0;
        	}
        	printf("Extracted: %d\n", res.result);
	}
	 close(fd);

        return 0;
}

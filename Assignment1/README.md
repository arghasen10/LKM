To run the kernel modules, follow the instructions:
```bash
    make
    sudo insmod deque.ko
    chmod +x userbash.sh
    sudo su
    	# ./userbash.sh
    sudo rmmod deque

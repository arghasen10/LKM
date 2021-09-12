# LKM
Linux Kernel Module or Loadable Kernel Module to play with Linux Kernels.

## DeQue (Doubly Ended Queue)
   First Module is a Simple DeQue implementation in Kernel that takes input from User Space application. For communication it leverages `/proc/` filesystem.
   
To run the kernel modules, follow the following instructions:
```bash
    make
    sudo insmod deque.ko
    chmod +x userbash.sh
    sudo su
    ./userbash.sh
    rmmod lkm_module<A1/A2>
```

# LKM
Linux Kernel Module or Loadable Kernel Module to play with Linux Kernels.

## This Repository also covers the Assignment solutions for CS60038 Advances in Operating Systems Design Course , AUT2021, IITKGP 
### Assignment1: DeQue (Doubly Ended Queue)
   First Module is a Simple DeQue implementation in Kernel that takes input from User Space application. For communication it leverages `/proc/` filesystem.
   
To run the kernel modules, follow the following instructions:
```bash
    cd Assignment1
    make
    sudo insmod deque.ko
    chmod +x userbash.sh
    sudo su
    ./userbash.sh
    rmmod lkm_module<A1/A2>
```
### Assignment2: DeQue (Doubly Ended Queue) with Handling Syscalls using IOCTL in LKM 
   Here we do the same DeQue implementation in Kernel that takes input from User Space application using IOCTL commands.
   
To run the kernel modules, follow the instructions:
```bash
    cd Assignment2
    make
    sudo insmod 20CS92P05.ko
    gcc userspace.c -o user
    ./user
    sudo rmmod 20CS92P05

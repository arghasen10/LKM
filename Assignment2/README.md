To run the kernel modules, follow the instructions:
```bash
    make
    sudo insmod 20CS92P05.ko
    gcc userspace.c -o user
    ./user
    sudo rmmod 20CS92P05

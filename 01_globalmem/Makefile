KVERS = $(shell uname -r)

# Kernel modules
obj-m += globalmem.o
#obj-m += multi_globalmem.o

# Specify flags for the module compilation.
#EXTRA_CFLAGS=-g -O0

build: kernel_modules client_program

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
client_program:
	gcc read.c -o read
	gcc write.c -o write
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
	rm -rf read 
	rm -rf write

obj-m += character-device-module.o

all:
	make -C /lib/modules/5.15.0-70-generic/build M=$(PWD) modules

clean:
	make -C /lib/modules/5.15.0-70-generic/build M=$(PWD) modules

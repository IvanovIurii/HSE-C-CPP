How To:
1. `sudo apt install g++`
2. `sudo apt-get install linux-headers-$(uname -r)`
3. `make`
4. To load module: `sudo insmod ./character-device-module.ko` 
5. To unload module: `sudo rmmod character-device-module` 
6. To WRITE: `echo "a" > /dev/my_device`
7. To READ (will return PONG): `head -c4 /dev/my_device`
8. To read kernel logs: `sudo dmesg`
9. Also can use for testing `test.cpp`: `make CXXFLAGS='-Wall -Werror' test && ./test`

What else can be done:
1. IOCTL
2. Linked list instead of fixed pool of processes (IDs)
3. Better signals handling

Sources:
1. https://blog.bytehackr.in/unlocking-the-power-of-linux-device-drivers
2. https://olegkutkov.me/2018/03/14/simple-linux-character-device-driver/
3. https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html
4. https://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
5. https://gitlab.com/kierangrant/khttpd
6. Linux Device Drivers Book: https://amzn.eu/d/cO4zMck

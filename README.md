# lokios

Simple experimental OS that can be booted with osx-pxe-server.

# Theory of operation

1. lokios.0 is an MBR and PXE-compatible bootloader.  The size of lokios.0 will be larger than a single 512-byte sector.  If we are in an MBR-style boot, then the first sector has been loaded into memory and it must load the rest of lokios.0.  If we are in a PXE-style boot, all of lokios.0 is already in memory thanks to the PXE mechanism.  A small stack is set up from 0xF000-0xFC00 before we branch in and we save some values here in case we end up failing to boot and want to return to BIOS.  lokios.0 will then interact with BIOS to fetch the E820 memory map, enter unreal mode so we can access the first 4G of memory, load lokios.1 into memory at 0x00200000, switch to 64-bit protected mode with a trivial low-memory identity-mapping page table and then jump into lokios.1.  For MBR boots we will load lokios.1 off disk starting in the sector following the end of lokios.0.  For PXE boots, we fetch lokios.1 from the PXE server using PXE calls.

2. lokios.1 is the actual kernel.  It gets loaded into memory at address 0x00200000 and will still be running off the crappy lokios.0 stack.  It switch stacks, records the E820 contents that will be passed up from lokios.0 and initializes page tables to map the first 32M of RAM.  We do some simple ACPI parsing, set up some interrupt stuff and output to the screen and serial port.  C++ exceptions and RTTI also work.

# Getting started

Congratulations, you've cloned lokios onto your docker-enabled desktop!  Now, to start working with it there are a couple more steps required.

First, you need to create the docker development/build environment.  Enter the docker subdirectory and display the build and run commands we'll need to invoke to start a container:

cd docker
head -2 Dockerfile

Execute the build command (inserting your name/username into the build parameters) and then execute the run command.  You will then be presented with a bash prompt from inside the docker container.  Do all your work here; when you are done, simply exit the container.  From outside the container, use 'docker ps -a' to find the name of your container and reenter it when you want to continue working via 'docker start -i container_name'.  Use of tmux while inside the container is recommended for sanity; Ctrl-A is the tmux command prefix.

Now that you are inside the container, cat the README.txt file and follow the directions to clone and build lokios.  Yes, I know you've already cloned it onto your desktop - now you need to clone it into the docker container too.

# Building and running unit tests

make -j5

# Running integration/smoke test under qemu

qemu comes preinstalled in the docker image, so you can just test your changes immediately from the command line.  By specifying an 'iTST' ACPI table entry we can also tell the kernel to exit after initialization steps are complete.

qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.mbr,format=raw -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null

An exit code of 3 indicates that the kernel successful ran and exited.  Any other exit code indicates a problem.

# Typical build/test cycle

This version of the qemu invocation sets up TCP forwarding on localhost port 12345 into guest port 12345.  This can be used to initiate a TCP connection from the docker container to the lokios kernel for testing purposes (say, by using the telnet tool):

make -j && qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.mbr,format=raw -smp 4 -nographic -device isa-debug-exit -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M

To exit qemu: Type Ctrl-A X.  Or, if you are within tmux: Ctrl-A A X.

# lokios

# Getting started

Congratulations, you've cloned lokios onto your docker-enabled desktop!  Now, to start working with it there are a couple more steps required.

First, you need to create the docker development/build environment.  Enter the docker subdirectory and display the build and run commands we'll need to invoke to start a container:

```
cd docker
head -2 Dockerfile
```

Execute the build command (inserting your name/username into the build parameters) and then execute the run command.  You will then be presented with a bash prompt from inside the docker container.  Do all your work here; when you are done, simply exit the container.  From outside the container, use 'docker ps -a' to find the name of your container and reenter it when you want to continue working via 'docker start -i container_name'.  Use of tmux while inside the container is recommended for sanity; Ctrl-A is the tmux command prefix.

Now that you are inside the container, cat the README.txt file and follow the directions to clone and build lokios.  Yes, I know you've already cloned it onto your desktop - now you need to clone it into the docker container too.

# Building and running unit tests

```
make -j5
```

# Typical build/test cycle

qemu comes preinstalled in the docker image, so you can just test your changes immediately from the command line.  This version of the qemu invocation sets up TCP forwarding on localhost port 12345 into guest port 12345.  This can be used to initiate a TCP connection from the docker container to the lokios kernel for testing purposes (say, by using the telnet tool):

```
make -j && qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.elf.mbr,format=raw -smp 4 -nographic -device isa-debug-exit -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M
```

To exit qemu: Type Ctrl-A X.  Or, if you are within tmux: Ctrl-A A X.

# "Integration" tests

By specifying an 'iTST' ACPI table entry when we invoke qemu, we can also tell the kernel to exit after initialization steps are complete.

```
qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.elf.mbr,format=raw -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M
```

An exit code of 3 indicates that the kernel successful ran and exited.  Any other exit code indicates a problem.  This functionality is used by the 'make_all' scripts found under the tools/ directory which can be used along with 'git rebase' to build each commit in a rebase and ensure that the kernel boots far enough for DHCP to work.

# PXE booting inside QEMU

qemu can also be used to test PXE-booting with qemu's built-in PXE server.  It is pretty slow to get link-up (10-20 seconds) though so it's not an ideal test environment.

```
make -j && qemu-system-x86_64 -cpu qemu64,+popcnt -smp 4 -device isa-debug-exit -device virtio-net-pci,netdev=net0,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345,tftp=bin/,bootfile=lokios.0 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M -boot n -nographic
```

You can also specify -curses instead of -nographic for the ugly ANSI VGA emulation.  Use Esc-1, Esc-2 and so on to switch consoles in this mode.

# Preparing and booting from a USB key using Mac OS

1. Download the "Etcher" app:

```
https://etcher.io
```

2. Copy lokios.mbr or lokios.elf.mbr out of the docker image to your local desktop:

```
scp bin/* docker.for.mac.localhost:Desktop/
```

3. Add a .raw suffix to the .mbr file so that Etcher will recognize it as a raw image.
4. Insert your USB key and make sure Etcher detects it.
5. Drag the .mbr.raw file onto Etcher.
6. Burn it.
7. Insert the USB key into the target machine.
8. Boot and hold down the Option key until you see a disk selector menu.
9. Select the removable drive labeled "Windows".  Tip: Hold down Control while doing this to make your selection persistent.

You will now boot into LokiOS on the bare metal.  Note: I had to use a USB 2.0 key to make this work on my old MacBook Air.  I think it does support USB 3.0, but I tried first using a USB 3.0 key and was very frustrated for awhile before I tried a USB 2.0 key and everything just worked.  It seems that the Mac BIOS wouldn't recognize the USB 3.0 key for some reason.

# Preparing and booting from a USB key on Linux

1. Presumably you can 'dd' the lokios.mbr or lokios.elf.mbr to the first sector of a /dev node.
2. Reboot your machine with the USB key inserted and drop into BIOS setup (typically by mashing F10 or F12 when your PC reboots).
3. Select the USB device as the boot device and go.

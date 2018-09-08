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

The easiest way to test your changes is by invoking the hammer.py script.  By default it will launch lokios.elf.mbr in a qemu process and then execute a batch of integration tests and finally exit back to the command line with a status code indicating success or failure.  You can also specify the --standalone option to omit running the integration tests and just leave the kernel running in that console instead - perfect for doing manual testing.  Finally, if you really care to invoke qemu by hand, it's recommended to use the hammer.py --command-line option to get a sane starting point and work from there.

```
make -j && hammer/hammer.py --standalone
```

To exit qemu in --standalone: Type Ctrl-A X.  Or, if you are within tmux: Ctrl-A A X.  Alternatively, you can always send a JSON command to set the '/' state to 'stopped'.

# "Integration" tests

It used to be that you had to specify special ACPI tables and that the kernel would then run some built-in "integration" tests.  There are no longer any special ACPI tables and there are no longer any built-in tests; to run integration tests, invoke the hammer.py script:

```
hammer/hammer.py
```

This uses a standard exit code of 0 to indicate that all integration tests succeeded and a non-0 exit code if there were failures.  This functionality is used by the 'make_all' scripts found under the tools/ directory which can be used along with 'git rebase' to build each commit in a rebase and ensure that the kernel passes all unit and integration tests.

# PXE booting inside QEMU

qemu can also be used to test PXE-booting with qemu's built-in PXE server.  It is pretty slow to get link-up (10-20 seconds) though so it's not an ideal test environment.  This is done by invoking hammer.py with the --pxe option:

```
make -j && hammer/hammer.py --pxe
```

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

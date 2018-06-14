# lokios

Simple experimental OS that can be booted with osx-pxe-server.

# Theory of operation

1. lokios.0 is an MBR and PXE-compatible bootloader.  The size of lokios.0 will be larger than a single 512-byte sector.  If we are in an MBR-style boot, then the first sector has been loaded into memory and it must load the rest of lokios.0.  If we are in a PXE-style boot, all of lokios.0 is already in memory thanks to the PXE mechanism.  A small stack is set up from 0xF000-0xFC00 before we branch in and we save some values here in case we end up failing to boot and want to return to BIOS.  lokios.0 will then interact with BIOS to fetch the E820 memory map, enter unreal mode so we can access the first 4G of memory, load lokios.1 into memory at 0x00200000, switch to 64-bit protected mode with a trivial low-memory identity-mapping page table and then jump into lokios.1.  For MBR boots we will load lokios.1 off disk starting in the sector following the end of lokios.0.  For PXE boots, we fetch lokios.1 from the PXE server using PXE calls.

2. lokios.1 is the actual kernel.  It gets loaded into memory at address 0x00200000 and will still be running off the crappy lokios.0 stack.  It switch stacks, records the E820 contents that will be passed up from lokios.0 and initializes page tables to map the first 32M of RAM.  We do some simple ACPI parsing, set up some interrupt stuff and output to the screen and serial port.  C++ exceptions and RTTI also work.

# Building and running unit tests

make -j5

# Running integration/smoke test under qemu

qemu comes preinstalled in the docker image, so you can just test your changes immediately from the command line.

qemu-system-x86_64 -drive file=bin/lokios.mbr,format=raw -nographic -device isa-debug-exit -smp 2

To exit qemu: Type Ctrl-A A X.

# Typical build/test cycle

make -j && qemu-system-x86_64 -drive file=bin/lokios.mbr,format=raw -smp 2 -nographic -device isa-debug-exit -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap

An exit code of 3 indicates that the kernel successful ran and exited.  Any other exit code indicates a problem.

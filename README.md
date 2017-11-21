# lokios

Simple experimental OS that can be booted with osx-pxe-server.

# Theory of operation

1. lokios.0 fits in a 512-byte boot sector.  If we are in an MBR-style boot, then its job is to load lokios.1 into memory at 0x7E00 and branch to it.  If we are in a PXE-style boot, lokios.1 is already in memory (since lokios.0 and lokios.1 are concatenated) so we can simply branch right in.  A small stack is set up from 0xF000-0xFC00 before we branch in and we save some values here in case we end up failing to boot and want to return to BIOS.

2. lokios.1 is variable-length but should be smaller than 32K and not crash into the stack set up by lokios.0.  The job here is to interact with BIOS to fetch the E820 memory map, enter unreal mode so we can access the first 4G of memory, load lokios.2 into memory at 0x00200000, switch to 64-bit protected mode with a trivial low-memory identity-mapping page table and then jump into lokios.2.  For MBR boots we will load lokios.2 off disk starting in the sector following the end of lokios.1.  For PXE boots, we will have to fetch lokios.2 from the PXE server using PXE calls.  Both of these are TODOs.

3. lokios.2 will be the actual kernel.  It will be loaded into memory at address 0x00200000 and still be running off the crappy lokios.0 stack.  It should switch stacks and record the E820 contents that will be passed up from lokios.1.  Here we need to do things like enable the caches and set up a real page table map.  Maybe do ACPI stuff too.  We should probably mark the first 2M as read-only.

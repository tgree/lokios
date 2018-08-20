#!/bin/bash

# Helper script to iterate through all "edit" commits in
# an interactive rebase and build them testing for build
# and unittest successes.  It will stop on the first commit
# that fails or go until the rebase is complete.
while true; do
    make clean && make -j || break
    qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.elf.mbr,format=raw -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M
    if [ $? -ne 3 ]
    then
        echo "-----MBR integration test failure-----"
        break
    fi

    qemu-system-x86_64 -cpu qemu64,+popcnt -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null -device virtio-net-pci,netdev=net0,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345,tftp=bin/,bootfile=lokios.0 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M -boot n
    if [ $? -ne 3 ]
    then
        echo "-----PXE integration test failure-----"
        break
    fi
    git rebase --continue || break
done

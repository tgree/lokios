#!/bin/bash

# Helper script to iterate through all "edit" commits in
# an interactive rebase and build them testing for build
# and unittest successes.  It will stop on the first commit
# that fails or go until the rebase is complete.
make clean
while true; do
    make -j || break
    qemu-system-x86_64 -cpu qemu64,+popcnt -drive file=bin/lokios.mbr,format=raw -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off,vectors=4 -netdev user,id=net0,hostfwd=tcp::12345-:12345 -object filter-dump,id=dump0,netdev=net0,file=net0dump.pcap -m 64M
    if [ $? -ne 3 ]
    then
        echo "-----Integration test failure-----"
        break
    fi
    git rebase --continue || break
done

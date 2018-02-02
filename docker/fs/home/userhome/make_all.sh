#!/bin/bash

# Helper script to iterate through all "edit" commits in
# an interactive rebase and build them testing for build
# and unittest successes.  It will stop on the first commit
# that fails or go until the rebase is complete.
while true; do
    make clean && make -j || break
    qemu-system-x86_64 -drive file=bin/lokios.mbr,format=raw -nographic -device isa-debug-exit -smp 2 -acpitable sig=iTST,data=/dev/null
    if [ $? -ne 3 ]
    then
        echo "-----Integration test failure-----"
        break
    fi
    git rebase --continue || break
done

#!/bin/bash

# Helper script to iterate through all "edit" commits in
# an interactive rebase and build them testing for build
# and unittest successes.  It will stop on the first commit
# that fails or go until the rebase is complete.
while true; do
    make clean && make -j || break
    if ! hammer/hammer.py; then
        echo "-----Integration test failure-----"
        break
    fi
    git rebase --continue || break
done

#!/bin/bash
set -e

lc3as ./asm-src/$1.asm
lc3sim -s <(echo "
file ./asm-src/${1}.obj
continue
quit") | awk '{if (x) print}; /Loaded/{x=1}'

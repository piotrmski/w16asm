#!/bin/bash
for filename in examples/*.asm; do
    echo Assembling ${filename%.asm}
    dist/w16asm $filename ${filename%.asm}.bin ${filename%.asm}.csv
done

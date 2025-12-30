#!/bin/bash
for filename in examples/*.asm; do
    echo Assembling ${filename%.asm}
    dist/w13asm $filename ${filename%.asm}.bin ${filename%.asm}.csv
done

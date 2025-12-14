#!/bin/bash
for filename in examples/*.asm; do
    dist/w16asm $filename ${filename%.asm}.bin ${filename%.asm}.csv
done

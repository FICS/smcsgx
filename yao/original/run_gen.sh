#!/bin/bash

mpirun -n 4 ./gen 80 4 ./circuits/mil.cir.bin.hash-free inp.txt 127.0.0.1 5000 1

#!/bin/bash


#mpirun  -n 4 ./trd 80 4 ./circuits/mil.cir.bin.hash-free inp.txt 127.0.0.1 9000 1 127.0.0.1 8000  &

#./third 80 4 ./circuits/mil.cir.bin.hash-free inp.txt 127.0.0.1 9000 1 127.0.0.1 8000 4

#cd Downloads/benkreuter-secure-computation-uva-58e2268/system/


#./third 80 4 ./circuits/edt-dist.cir.bin.hash-free inpthird.txt 127.0.0.1 9000 1 127.0.0.1 8000 4 &

#sleep 1


#./evl 80 4 ./circuits/edt-dist.cir.bin.hash-free inp.txt 10.0.1.2 5000 1 127.0.0.1 9000 > oute.txt 

export LD_LIBRARY_PATH=${HOME}/local/lib/:${LD_LIBRARY_PATH}


sleep 1
date

#./gen 80 $3 $1 inp.txt 172.16.1.11 13010 1 172.16.1.11 8000

./evl 80 $3 $1 inp.txt 127.0.0.1 5000 1 127.0.0.1 9000  &
sleep 1
./gen 80 $3 $1 inp.txt 127.0.0.1 5000 1 127.0.0.1 8000

## = single machine
### = phone



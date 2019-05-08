#!/bin/bash


#mpirun  -n 4 ./trd 80 4 ./circuits/mil.cir.bin.hash-free inp.txt 127.0.0.1 9000 1 127.0.0.1 8000  &

#./third 80 4 ./circuits/mil.cir.bin.hash-free inp.txt 127.0.0.1 9000 1 127.0.0.1 8000 4

#cd Downloads/benkreuter-secure-computation-uva-58e2268/system/


export LD_LIBRARY_PATH=${HOME}/pbc-0.5.14/.libs/:${LD_LIBRARY_PATH}


#date

./evl 80 1 ./TestPrograms/arrayinit.wir inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 &
#sleep 1
./gen 80 1 ./TestPrograms/arrayinit.wir inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 

#date
exit



#make

#javac TestNet.java
#java TestNet2 s
date

 ./evl 80 1 ./circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000  &
#sleep 1
./gen 80 1 ./circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 


sleep 5

time         ./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000   &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 

exit

sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5

./evl 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >> evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/edt-dist128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >> gennt.txt



sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt

sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt


sleep 5




 ./evl 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 9000 >>evlnt.txt  &
#sleep 1
./gen 80 1 /srv/users/bmood/circuits/setinter128.cir.bin.hash-free inp.txt 127.0.0.1 5000 1 127.0.0.1 8000 >>gennt.txt



exit


#mpirun  -n $2 ./evl 80 $3 $1 inp.txt 127.0.0.1 12000 1 172.16.0.10 13010 >> oute2.txt &
#sleep 1 
#mpirun  -n $2 ./gen 80 $3 $1 inp.txt 127.0.0.1 12000 1 172.16.1.10 13090


####mpirun  -n $2 ./evl 80 $3 $1 inp.txt 127.0.0.1 5000 1 172.16.0.10 9000 >> oute2.txt &
##sleep 2
##mpirun  -n $2 ./gen 80 $3 $1 inp.txt 172.16.0.3 12000 1 172.16.1.10 18000

## = phone
#### = DO NOT UNCOMMENT
#./third 80 4 ./circuits/mil4.cir.bin.hash-free inpthird.txt 127.0.0.1 19000 1 127.0.0.1 18000 8 &
#sleep 1
#mpirun  -n 4 ./evl 80 8 ./circuits/mil4.cir.bin.hash-free inp.txt 127.0.0.1 15000 1 127.0.0.1 19000  &
#sleep 1
#mpirun  -n 4 ./gen 80 8 ./circuits/mil4.cir.bin.hash-free inp.txt 127.0.0.1 15000 1 127.0.0.1 18000

#./third 80 4  ./circuits/edt-dist2.cir.bin.hash-free inpthird.txt 127.0.0.1 19000 1 127.0.0.1 18000 4 &
#sleep 1
#mpirun  -n 4 ./evl 80 4  ./circuits/edt-dist2.cir.bin.hash-free inp.txt 127.0.0.1 16000 1 127.0.0.1 19000 >> oute3.txt &
#sleep 1
#mpirun  -n 4 ./gen 80 4  ./circuits/edt-dist2.cir.bin.hash-free inp.txt 127.0.0.1 16000 1 127.0.0.1 18000


nodes=2
circuits=2

#cd ..
#cd system
#./third 80 $nodes ./circuits/aes.cir.bin.hash-free inpthird.txt 127.0.0.1 19000 1 127.0.0.1 18000 $circuits  &
#sleep 1
#mpirun  -n $nodes ./evl 80 $circuits  ./circuits/aes.cir.bin.hash-free  inp.txt 127.0.0.1 15000 1 127.0.0.1 19000   &
#sleep 1
#mpirun  -n $nodes ./gen 80 $circuits  ./circuits/aes.cir.bin.hash-free  inp.txt 127.0.0.1 15000 1 127.0.0.1 18000  
#exit

'
cp /srv/users/bmood/partialnompi/keyed64start.cir.bin.hash-free ./circuits/keyed64start.cir.bin.hash-free
cp /srv/users/bmood/partialnompi/keyed64end.cir.bin.hash-free ./circuits/keyed64end.cir.bin.hash-free


./third 80 $nodes ./circuits/keyed64start.cir.bin.hash-free inpthird.txt 127.0.0.1 35010 1 127.0.0.1 35090 $circuits 1  &
#sleep 1
mpirun  -n $nodes ./evl 80 $circuits  ./circuits/keyed64start.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35010 1    &
sleep 1
mpirun  -n $nodes ./gen 80 $circuits  ./circuits/keyed64start.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35090 1   




sleep 4

cp evlpartial.txt evlpartialin.txt
cp genpartial.txt genpartialin.txt




./third 80 $nodes ./circuits/keyed64end.cir.bin.hash-free inpthird.txt 127.0.0.1 35010 1 127.0.0.1 35090 $circuits 0  &
#sleep 1
mpirun  -n $nodes ./evl 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35010 0   &
sleep 1
mpirun  -n $nodes ./gen 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35090 0  


sleep 4

cp evlpartial.txt evlpartialin.txt
cp genpartial.txt genpartialin.txt




./third 80 $nodes ./circuits/keyed64end.cir.bin.hash-free inpthird.txt 127.0.0.1 35010 1 127.0.0.1 35090 $circuits 0  &
#sleep 1
mpirun  -n $nodes ./evl 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35010 0   &
sleep 1
mpirun  -n $nodes ./gen 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35090 0  




exit
'

#cp /srv/users/bmood/partialnompi/documentstart256.cir.bin.hash-free ./circuits/documentstart256.cir.bin.hash-free
#cp /srv/users/bmood/partialnompi/documentset256.cir.bin.hash-free ./circuits/documentset256.cir.bin.hash-free
#cp /srv/users/bmood/partialnompi/documentget256.cir.bin.hash-free ./circuits/documentget256.cir.bin.hash-free



mpd &


sleep 1


./third 80 $nodes ./circuits/keyed64start.cir.bin.hash-free inpthird.txt 127.0.0.1 35010 1 127.0.0.1 35090 $circuits 1  &
#sleep 1
mpirun  -n $nodes ./evl 80 $circuits  ./circuits/keyed64start.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35010 1    &
sleep 1
mpirun  -n $nodes ./gen 80 $circuits  ./circuits/keyed64start.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35090 1   


sleep 4

cp evlpartial.txt evlpartialin.txt
cp genpartial.txt genpartialin.txt




./third 80 $nodes ./circuits/keyed64end.cir.bin.hash-free inpthird.txt 127.0.0.1 35010 1 127.0.0.1 35090 $circuits 0  &
#sleep 1
mpirun  -n $nodes ./evl 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35010 0     &
sleep 1
mpirun  -n $nodes ./gen 80 $circuits  ./circuits/keyed64end.cir.bin.hash-free  inp.txt 127.0.0.1 36000 1 127.0.0.1 35090 0    





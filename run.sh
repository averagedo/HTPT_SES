#!/bin/bash
make complie

for((i=1;i<=2;i++))
do 
  ./main $i &
done

#!/bin/bash
make complie

for((i=1;i<=5;i++))
do 
  ./main $i &
done

#!/bin/bash
make complie

for((i=1;i<=3;i++))
do 
  ./main $i &
done

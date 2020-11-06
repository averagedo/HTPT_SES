#!/bin/bash
make complie

for((i=1;i<=15;i++))
do 
  ./main $i &
done

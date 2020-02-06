#!/bin/bash
num=0
while [ $num -lt $1 ]; do
    echo $num
    ((num++))
    sleep 1
done
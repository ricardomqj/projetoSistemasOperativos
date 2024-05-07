#!/bin/bash

for i in {1..10}
do
   ./client execute 100 -u "echo Test $i" &
   ./client execute 150 -p "cat /etc/hosts | grep localhost" &
done

wait

./client status
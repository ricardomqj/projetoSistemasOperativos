#!/bin/bash

pwd
./orchestrator &

# Cliente 1 envia comandos
./client execute 100 -u "ls -l" &
./client execute 150 -p "cat /etc/passwd | grep root" &

# Cliente 2 envia comandos
./client execute 200 -u "date" &
./client execute 250 -p "df -h | grep dev" &

# Cliente 3 verifica o status
./client status &

wait
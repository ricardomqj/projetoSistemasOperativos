#!/bin/bash
pwd
./orchestrator &

# Executa comandos simples
./client execute 100 -u "echo Hello, World!"
./client execute 150 -u "date"
./client execute 200 -u "uname -a"

# Executa comandos em pipelining
./client execute 300 -p "cat /etc/passwd | grep root"
./client execute 350 -p "ls -la | grep .txt | wc -l"

# Verifica o status
./client status
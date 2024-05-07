CC = gcc
CFLAGS = -Wall -g -Iinclude
#LDFLAGS = 


all: folders orchestrator client

orchestrator: orchestrator

client: client

folders:
	@mkdir -p src obj tmp output_folder

orchestrator: obj/orchestrator.o
	$(CC) $(CFLAGS) obj/orchestrator.o -o orchestrator

obj/orchestrator.o: src/orchestrator.c
	$(CC) $(CFLAGS) -c src/orchestrator.c -o obj/orchestrator.o

client: obj/client.o
	$(CC) $(CFLAGS) obj/client.o -o client

obj/client.o: src/client.c
	$(CC) $(CFLAGS) -c src/client.c -o obj/client.o

clean:
	rm -f obj/* client orchestrator tmp/* output_folder/*


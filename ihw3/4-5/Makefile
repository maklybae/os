CC = gcc

.PHONY: all
all: server client

server: server.c error.c
	${CC} server.c error.c -o server

client: client.c error.c
	${CC} client.c error.c -o client

.PHONY: clean
clean:
	rm -f server client
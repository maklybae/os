CC = gcc

.PHONY: all
all: server client watcher

server: server.c error.c
	${CC} server.c error.c -o server

client: client.c error.c
	${CC} client.c error.c -o client

watcher: watcher.c error.c
	${CC} watcher.c error.c -o watcher

.PHONY: clean
clean:
	rm -f server client watcher
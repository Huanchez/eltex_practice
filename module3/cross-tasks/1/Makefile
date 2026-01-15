CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client

.PHONY: all clean
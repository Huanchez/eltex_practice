CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS=-pthread

all: udp_chat

udp_chat: udp_chat.c
	$(CC) $(CFLAGS) udp_chat.c -o udp_chat $(LDFLAGS)

clean:
	rm -f udp_chat

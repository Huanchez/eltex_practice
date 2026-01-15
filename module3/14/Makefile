CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS=-pthread

all: udp_chat sniffer_raw

udp_chat: udp_chat.c
	$(CC) $(CFLAGS) udp_chat.c -o udp_chat $(LDFLAGS)

sniffer_raw: sniffer_raw.c
	$(CC) $(CFLAGS) sniffer_raw.c -o sniffer_raw

clean:
	rm -f udp_chat sniffer_raw *.o *.bin

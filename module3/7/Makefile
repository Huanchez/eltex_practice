CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lrt

all: ping pong

ping: ping.c
	$(CC) $(CFLAGS) ping.c -o ping $(LDFLAGS)

pong: pong.c
	$(CC) $(CFLAGS) pong.c -o pong $(LDFLAGS)

clean:
	rm -f ping pong

.PHONY: all clean

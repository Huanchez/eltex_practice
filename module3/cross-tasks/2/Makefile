CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2

all: cli

cli: cli.c
	$(CC) $(CFLAGS) cli.c -o cli

clean:
	rm -f cli

.PHONY: all clean

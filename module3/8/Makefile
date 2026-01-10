CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: fabricator user

fabricator: fabricator.c
	$(CC) $(CFLAGS) fabricator.c -o fabricator

user: user.c
	$(CC) $(CFLAGS) user.c -o user

clean:
	rm -f fabricator user

.PHONY: all clean

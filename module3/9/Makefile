CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -pthread

all: sem_posix

sem_posix: sem_posix.c
	$(CC) $(CFLAGS) sem_posix.c -o sem_posix $(LDFLAGS)

clean:
	rm -f sem_posix

.PHONY: all clean

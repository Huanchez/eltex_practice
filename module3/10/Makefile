CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: shm_systemV

shm_systemV: shm_systemV.c
	$(CC) $(CFLAGS) shm_systemV.c -o shm_systemV

clean:
	rm -f shm_systemV

.PHONY: all clean

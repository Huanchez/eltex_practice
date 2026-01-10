CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2
TARGET=shm_posix

all: $(TARGET)

$(TARGET): shm_posix.c
	$(CC) $(CFLAGS) $< -o $@ -pthread

clean:
	rm -f $(TARGET)

.PHONY: all clean

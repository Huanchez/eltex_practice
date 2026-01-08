CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

TARGET = signals

all: $(TARGET)

$(TARGET): signals.c
	$(CC) $(CFLAGS) signals.c -o $(TARGET)

clean:
	rm -f $(TARGET) counter.txt

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

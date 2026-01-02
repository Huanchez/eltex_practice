CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET  := pq_test

OBJS    := main.o priority_queue.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c priority_queue.h
	$(CC) $(CFLAGS) -c main.c -o main.o

priority_queue.o: priority_queue.c priority_queue.h
	$(CC) $(CFLAGS) -c priority_queue.c -o priority_queue.o

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

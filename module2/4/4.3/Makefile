CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET  := phonebook_tree

OBJS    := main.o contacts.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c contacts.h
	$(CC) $(CFLAGS) -c main.c -o main.o

contacts.o: contacts.c contacts.h
	$(CC) $(CFLAGS) -c contacts.c -o contacts.o

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

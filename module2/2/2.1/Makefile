CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET  := phonebook
OBJS    := main.o source.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c header.h
	$(CC) $(CFLAGS) -c main.c

source.o: source.c header.h
	$(CC) $(CFLAGS) -c source.c

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
	
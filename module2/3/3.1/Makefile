CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=
TARGET  := permtool
OBJS    := main.o chmod.o

.PHONY: all clean run debug asan

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.c chmod.h
	$(CC) $(CFLAGS) -c main.c

chmod.o: chmod.c chmod.h
	$(CC) $(CFLAGS) -c chmod.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

debug: CFLAGS += -O0 -g
debug: clean $(TARGET)

asan: CFLAGS += -O0 -g -fsanitize=address,undefined
asan: LDFLAGS += -fsanitize=address,undefined
asan: clean $(TARGET)

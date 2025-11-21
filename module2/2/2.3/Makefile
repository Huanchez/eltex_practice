CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS  :=
TARGET   := calculator
OBJS     := main.o calculator.o

.PHONY: all clean run debug asan

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.c calculator.h
	$(CC) $(CFLAGS) -c main.c

calculator.o: calculator.c calculator.h
	$(CC) $(CFLAGS) -c calculator.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

debug: CFLAGS += -O0 -g
debug: clean $(TARGET)

asan: CFLAGS += -O0 -g -fsanitize=address,undefined
asan: LDFLAGS += -fsanitize=address,undefined
asan: clean $(TARGET)

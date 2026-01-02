CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

TARGET := ip

SRC := main.c ip.c
OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c ip.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean

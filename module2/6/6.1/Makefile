CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2

TARGET  := phonebook
LIBNAME := libcontacts.a

MAIN_OBJ := main.o
LIB_OBJ  := contacts.o

all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJ) -L. -lcontacts

$(LIBNAME): $(LIB_OBJ)
	ar rcs $(LIBNAME) $(LIB_OBJ)

main.o: main.c contacts.h
	$(CC) $(CFLAGS) -c main.c -o main.o

contacts.o: contacts.c contacts.h
	$(CC) $(CFLAGS) -c contacts.c -o contacts.o

clean:
	rm -f $(TARGET) $(MAIN_OBJ) $(LIB_OBJ) $(LIBNAME)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
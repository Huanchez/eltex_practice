CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
PIC     := -fPIC

TARGET  := phonebook
LIBNAME := libcontacts.so

MAIN_OBJ := main.o
LIB_OBJ  := contacts.o

all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJ) -L. -lcontacts -Wl,-rpath,'$$ORIGIN'

$(LIBNAME): $(LIB_OBJ)
	$(CC) -shared -o $(LIBNAME) $(LIB_OBJ)

main.o: main.c contacts.h
	$(CC) $(CFLAGS) -c main.c -o main.o

contacts.o: contacts.c contacts.h
	$(CC) $(CFLAGS) $(PIC) -c contacts.c -o contacts.o

clean:
	rm -f $(TARGET) $(MAIN_OBJ) $(LIB_OBJ) $(LIBNAME)

run: $(TARGET)
	LD_LIBRARY_PATH=. ./$(TARGET)

.PHONY: all clean run

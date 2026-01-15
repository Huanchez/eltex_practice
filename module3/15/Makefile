CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

CLIENT = tcp_client
SERVER = tcp_server

all: $(CLIENT) $(SERVER)

$(CLIENT): tcp_client.c
	$(CC) $(CFLAGS) tcp_client.c -o $(CLIENT)

$(SERVER): tcp_server.c
	$(CC) $(CFLAGS) tcp_server.c -o $(SERVER)

clean:
	rm -f $(CLIENT) $(SERVER)

.PHONY: all clean

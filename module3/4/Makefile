CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: ex4_shell sum_args concat_args maxlen_arg

ex4_shell: ex4_shell.c
	$(CC) $(CFLAGS) $< -o $@

sum_args: sum_args.c
	$(CC) $(CFLAGS) $< -o $@

concat_args: concat_args.c
	$(CC) $(CFLAGS) $< -o $@

maxlen_arg: maxlen_arg.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ex4_shell sum_args concat_args maxlen_arg

.PHONY: all clean

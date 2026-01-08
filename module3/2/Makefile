CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: ex2_shell sum_args concat_args maxlen_arg

ex2_shell: ex2_shell.c
	$(CC) $(CFLAGS) $< -o $@

sum_args: sum_args.c
	$(CC) $(CFLAGS) $< -o $@

concat_args: concat_args.c
	$(CC) $(CFLAGS) $< -o $@

maxlen_arg: maxlen_arg.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ex2_shell sum_args concat_args maxlen_arg

.PHONY: all clean

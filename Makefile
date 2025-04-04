CC = gcc
CFLAGS = -Wall -Wextra -g
TARGETS = multi_process_server

all: $(TARGETS)

multi_process_server: multi_process_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean 
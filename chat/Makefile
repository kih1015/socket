CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGETS = chat_server chat_client

.PHONY: all clean

all: $(TARGETS)

chat_server: chat_server.c
	$(CC) $(CFLAGS) -o $@ $^

chat_client: chat_client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)

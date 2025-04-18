CC = gcc
CFLAGS = -Wall -Wextra
TARGETS = telnet_server telnet_client

all: $(TARGETS)

telnet_server: telnet_server.c
	$(CC) $(CFLAGS) -o $@ $<

telnet_client: telnet_client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) 
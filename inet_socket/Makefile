CC = gcc
CFLAGS = -Wall -Wextra

all: inet_client inet_server

inet_client: inet_client.c
	$(CC) $(CFLAGS) -o $@ $<

inet_server: inet_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f inet_client inet_server 
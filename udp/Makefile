CC = gcc
CFLAGS = -Wall -Wextra

all: udp_client udp_server

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o $@ $<

udp_server: udp_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f udp_client udp_server 
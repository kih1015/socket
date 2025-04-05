CC = gcc
CFLAGS = -Wall -Wextra

all: udp_peer

udp_peer: udp_peer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f udp_peer 
	
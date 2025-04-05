CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses

all: udp_peer

udp_peer: udp_peer.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f udp_peer 
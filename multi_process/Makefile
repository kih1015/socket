CC = gcc
CFLAGS = -Wall -Wextra

all: multi_process_client multi_process_server

multi_process_client: multi_process_client.c
	$(CC) $(CFLAGS) -o $@ $<

multi_process_server: multi_process_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f multi_process_client multi_process_server 
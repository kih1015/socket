CC = gcc
CFLAGS = -Wall -Wextra

all: student_id_client student_id_server

student_id_client: student_id_client.c
	$(CC) $(CFLAGS) -o $@ $<

student_id_server: student_id_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f student_id_client student_id_server 
CC = gcc
CFLAGS = -Wall -Wextra
CLIENT = unix_client
SERVER = unix_server
INET_SERVER = inet_server
INET_CLIENT = inet_client
CLIENT_SRC = client.c
SERVER_SRC = server.c
INET_SERVER_SRC = inet_server.c
INET_CLIENT_SRC = inet_client.c

all: $(CLIENT) $(SERVER) $(INET_SERVER) $(INET_CLIENT)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(INET_SERVER): $(INET_SERVER_SRC)
	$(CC) $(CFLAGS) -o $(INET_SERVER) $(INET_SERVER_SRC)

$(INET_CLIENT): $(INET_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(INET_CLIENT) $(INET_CLIENT_SRC)

clean:
	rm -f $(CLIENT) $(SERVER) $(INET_SERVER) $(INET_CLIENT) 
	
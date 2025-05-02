CC = gcc
CFLAGS = -Wall -Wextra
TARGET = telnet_client
PTHREAD_TARGET = telnet_client_pthread

all: $(TARGET) $(PTHREAD_TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(PTHREAD_TARGET): $(PTHREAD_TARGET).c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

clean:
	rm -f $(TARGET) $(PTHREAD_TARGET) 
CC = gcc
CFLAGS = -Wall -Wextra
TARGET = telnet_client

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) 
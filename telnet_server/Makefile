CC = gcc
CFLAGS = -Wall -Wextra
TARGET = telnet_server

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) 
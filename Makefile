CC = gcc
CFLAGS = -Wall -Wextra
TARGET = hostname_lookup
SRCS = main.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) 
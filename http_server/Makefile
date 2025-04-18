CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 
TARGET = http_server
OBJS = $(TARGET).o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS) 
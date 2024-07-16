CC = gcc
CFLAGS = -g
OBJECTS = main.o list.o hash.o hex_dump.o debug.o bitmap.o
TARGET = testlib

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(TARGET)

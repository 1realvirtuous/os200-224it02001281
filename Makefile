CC = gcc
CFLAGS = -Wall -O2 -Wextra
TARGET = test_224it02001281
SRC = test_224it02001281.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) typescript.log *.patch

.PHONY: all clean

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11
OBJ     := s2html_main.o s2html_event.o s2html_conv.o
TARGET  := s2html_conv.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
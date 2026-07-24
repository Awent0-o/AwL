CC = gcc
CFLAGS = -g -I. -Wall -Wextra
TARGET = awl

SRCS = main.c \
       lexer/lexer.c \
       parser/parser.c \
       semantic/semantic.c \
       codegen/c/codegen.c \
       codegen/python/codepy.c \
       utils/utils.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
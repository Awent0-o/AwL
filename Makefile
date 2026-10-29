CC = gcc
CFLAGS = -g -I. -Wall -Wextra
TARGET = awl

SRCS = main.c \
       lexer/lexer.c \
       parser/expr_parser.c \
       parser/stmt_parser.c \
       parser/func_parser.c \
       codegen/c/utils_codegen.c \
       parser/parser.c \
       codegen/c/expr_codegen.c \
       codegen/c/stmt_codegen.c \
       codegen/c/import_codegen.c \
       codegen/c/codegen.c \
       codegen/python/codepy.c \
       utils/flags.c \
       utils/file_utils.c \
       utils/string_utils.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

cp:
	cp $(TARGET) /usr/bin

clean:
	rm -f $(TARGET)

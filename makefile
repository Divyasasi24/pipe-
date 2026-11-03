CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_DEFAULT_SOURCE -Iinclude -g
SRC_DIR = src
OBJ_DIR = build
TARGET  = pipechat

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) Log/Chat.log.bak

distclean: clean
	: > Log/Chat.log

.PHONY: all run clean distclean
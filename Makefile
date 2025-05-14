CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -I./raylib/src
LDFLAGS = -L./raylib/src -lraylib -lm -ldl -lpthread -lGL

SRC = mines.c
OBJ_DIR = build
BIN_DIR = build
TARGET = $(BIN_DIR)/mines
OBJ = $(OBJ_DIR)/mines.o

RAYLIB_DIR = ./raylib
RAYLIB_LIB = $(RAYLIB_DIR)/src/libraylib.a

all: $(RAYLIB_LIB) $(TARGET)

$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_DIR)/src

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	$(MAKE) -C $(RAYLIB_DIR)/src clean
	rm -rf $(OBJ_DIR) $(BIN_DIR)

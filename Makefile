CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I./include -O3 -flto=auto $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/cpu.c $(SRC_DIR)/memory.c $(SRC_DIR)/opcodes.c $(SRC_DIR)/ppu.c
OBJECTS = $(OBJ_DIR)/main.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/memory.o $(OBJ_DIR)/opcodes.o $(OBJ_DIR)/ppu.o
EXEC = $(BIN_DIR)/C-GB

all: $(EXEC)

$(EXEC): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I./include -O3 -flto=auto $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
BIN_DIR = bin

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/cpu.c $(SRC_DIR)/memory.c $(SRC_DIR)/opcodes.c $(SRC_DIR)/ppu.c $(SRC_DIR)/gb.c
OBJECTS = $(OBJ_DIR)/main.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/memory.o $(OBJ_DIR)/opcodes.o $(OBJ_DIR)/ppu.o $(OBJ_DIR)/gb.o
EXEC = $(BIN_DIR)/C-GB

all: $(EXEC)

$(EXEC): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(INC_DIR)/config.h $(INC_DIR)/cpu.h $(INC_DIR)/memory.h $(INC_DIR)/opcodes.h $(INC_DIR)/ppu.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/cpu.o: $(SRC_DIR)/cpu.c $(INC_DIR)/cpu.h $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/memory.o: $(SRC_DIR)/memory.c $(INC_DIR)/memory.h $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/opcodes.o: $(SRC_DIR)/opcodes.c $(INC_DIR)/opcodes.h $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/ppu.o: $(SRC_DIR)/ppu.c $(INC_DIR)/ppu.h $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/gb.o: $(SRC_DIR)/gb.c $(INC_DIR)/gb.h $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
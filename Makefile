CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I./include -O3 -flto=auto $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
BIN_DIR = bin

# Find all .c files in the source directory and create corresponding .o filepaths
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
EXEC = $(BIN_DIR)/C-GB

all: $(EXEC)

$(EXEC): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/config.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
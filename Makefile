# as16 - the chip16 assembler
# Copyright (C) 2013 tykel
# This program is licensed under the GPLv3; see LICENSE for details.

# Common definitions

CC = gcc
CFLAGS = -O0 -g -Wall -Wno-strict-aliasing -Wno-unused-function -std=c89 -pedantic 
LDFLAGS = 

# Directories

SRC = src
OBJ = build

SOURCES = $(shell find $(SRC) -type f -name '*.c')
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Targets

.PHONY: all clean 

all: as16 

as16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@echo "Removing object files..."
	@rm -f $(OBJECTS) 
	@echo "Removing executable..."
	@rm -f as16

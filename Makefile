# as16 - the chip16 assembler
# Copyright (C) 2013 tykel
# This program is licensed under the GPLv3; see LICENSE for details.

# Common definitions

CC = gcc
CFLAGS = -O2 -Wall -Wno-strict-aliasing -Wno-unused-function -std=c89 -pedantic 
LDFLAGS = 

# Directories

SRC = src
OBJ = build

SOURCES = $(shell find $(SRC) -type f -name '*.c')
OBJECTS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Targets

.PHONY: all clean man install uninstall

all: as16 

as16: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) 
	rm -f as16 2>/dev/null
	rm -f as16.1.gz 2>/dev/null

install: as16 man
ifneq ($(USER),root)
	-@echo You are not root, please run: \'sudo make install\'
else
	cp as16 /usr/bin
	cp as16.1.gz /usr/share/man/man1
	-@echo Updating man-db ...
	-@mandb --no-purge -q
	-@echo done.
endif

uninstall:

uninstall:
ifneq ($(USER),root)
	-@echo You are not root, please run: \'sudo make uninstall\'
else
	-@echo Removing binaries ...
	-@rm /usr/bin/as16 2> /dev/null || true
	-@echo Removing man page ...
	-@rm /usr/share/man/man1/as16.1.gz 2> /dev/null || true
	-@echo Updating man-db ... 
	-@mandb --no-purge -q 
	-@echo done.
endif

man: as16.1.gz
as16.1.gz: as16.1
	@gzip < as16.1 > as16.1.gz


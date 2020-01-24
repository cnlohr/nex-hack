# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
# ------------------------------------------------

# project name (generate executable with this name)
TARGET   = fwtool

CC       = gcc
# compiling flags here
CFLAGS   = -Wall -DNDEBUG -DIOAPI_NO_64 -DFWT_CONSOLE

LINKER   = gcc -o
# linking flags here
LDFLAGS   = -lz -larchive

# change these to set the proper directories where each files shoould be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f

all : $(BINDIR)/$(TARGET) $(BINDIR)/lzpt_writer

$(BINDIR)/lzpt_writer : lzpt_writer.c src/lzpt_io.c src/lz77_inflate.c
	$(LINKER) $@ $^ $(LDFLAGS) $(CFLAGS) -lz

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(LINKER) $@ $(OBJECTS) $(LDFLAGS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)

.PHONEY: remove
remove: clean
	$(rm) $(BINDIR)/$(TARGET)

# Variables declaration
TARGET_NAME = encrypt.out
IDIR = ./tmp_deb/usr/include
LDIR = ./tmp_deb/usr/lib/x86_64-linux-gnu
CC = gcc
CFLAGS = -I$(IDIR)
LIBS = -L$(LDIR) -lmta_crypt -lmta_rand -lpthread

ODIR ?= build

#Source and Object Files- All C source files in current directory
SRCS := $(subst ./,,$(shell find . -maxdepth 1 -name "*.c"))
OBJS := $(addprefix $(ODIR)/,$(patsubst %.c,%.o,$(SRCS)))
OUT := $(ODIR)/$(TARGET_NAME)

$(ODIR)/%.o: %.c
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/$(TARGET_NAME): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Default target
all: $(OUT)

#Clean target
.PHONY: clean
clean:
	rm -rf $(ODIR)



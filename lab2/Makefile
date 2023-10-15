# Compiler
CC = gcc
CFLAGS = -Wall -g -std=c99
LD = gcc 
LDFLAGS = -g
LDLIBS = -lz -lcurl # link with zlib and curl

BUILD_DIRECTORY = obj

# Source files
SRCS = $(filter-out test.c, $(wildcard *.c))

# Object files
OBJS = $(addprefix $(BUILD_DIRECTORY)/, $(SRCS:.c=.o))

# All targets
TARGETS = paster

# Target: all
all: paster

# Target: paster
paster : $(OBJS)
	$(LD) $(LDLIBS) $(CFLAGS) $^ -o $@

# Target: clean
clean:
	rm -rf paster $(OBJS) $(TARGETS) $(BUILD_DIRECTORY)

# Pattern rule for object files
$(BUILD_DIRECTORY)/%.o: %.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
obj:
	mkdir -p $(BUILD_DIRECTORY)

.PHONY: clean all
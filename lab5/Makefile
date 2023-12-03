CC = gcc 
CFLAGS_LIBS = $(shell pkg-config --cflags libxml-2.0 libcurl)
CFLAGS = -Wall $(CFLAGS_LIBS) -std=gnu99 -g
LD = gcc
LDFLAGS = -std=gnu99 -g 
LDLIBS = $(shell pkg-config --libs libxml-2.0 libcurl)
BUILD_DIRECTORY = obj

# Source files
SRCS = $(filter-out test.c, $(wildcard *.c))

# Object files
OBJS = $(addprefix $(BUILD_DIRECTORY)/, $(SRCS:.c=.o))

# All targets
TARGETS = findpng3

# Target: all
all: $(TARGETS)

# Target: paster
findpng3 : $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(CFLAGS)

# Target: clean
clean:
	rm -rf $(TARGETS) $(OBJS) $(TARGETS) $(BUILD_DIRECTORY)

# Pattern rule for object files
$(BUILD_DIRECTORY)/%.o: %.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
obj:
	mkdir -p $(BUILD_DIRECTORY)

.PHONY: clean all
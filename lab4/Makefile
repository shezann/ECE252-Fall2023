CC = gcc 
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)
CFLAGS = -Wall $(CFLAGS_XML2) $(CFLAGS_CURL) -std=gnu99 -g -DDEBUG1_
LD = gcc
LDFLAGS = -std=gnu99 -g 
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) -lz -pthread

BUILD_DIRECTORY = obj

# Source files
SRCS = $(filter-out test.c, $(wildcard *.c))

# Object files
OBJS = $(addprefix $(BUILD_DIRECTORY)/, $(SRCS:.c=.o))

# All targets
TARGETS = paster2

# Target: all
all: findpng2

# Target: paster
findpng2 : $(OBJS)
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
CC = gcc
CFLAGS = -Wall -g -std=c99
LD = gcc 
LDFLAGS = -g
LDLIBS = -lz # link with zlib

BUILD_DIRECTORY = obj

#SRCS = $(wildcard *.c)
SRCS = $(filter-out temp.c, $(wildcard *.c))
OBJS = $(addprefix $(BUILD_DIRECTORY)/, $(notdir $(SRCS:.c=.o)))
DEPS := $(OBJS:.o=.d)

TARGETS = findpng catpng

all : $(TARGETS)

findpng : $(filter-out $(BUILD_DIRECTORY)/catpng.o, $(OBJS))
	$(LD)  -o $@ $^ $(LDLIBS) $(LDFLAGS)

catpng : $(filter-out $(BUILD_DIRECTORY)/findpng.o, $(OBJS))
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(BUILD_DIRECTORY)/%.o: %.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIRECTORY)/*.o $(BUILD_DIRECTORY)/*.d $(TARGETS) $(BUILD_DIRECTORY)

.PHONY: all clean
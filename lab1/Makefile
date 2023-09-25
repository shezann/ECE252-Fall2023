CC = gcc
CFLAGS = -Wall -g -std=c99
LD = gcc 
LDFLAGS = -g
LDLIBS = -lz # link with zlib

BUILD_DIRECTORY = obj

#SRCS = $(wildcard *.c)
SRCS = $(filter-out catpng.c, $(wildcard *.c))
OBJS = $(addprefix $(BUILD_DIRECTORY)/, $(notdir $(SRCS:.c=.o)))
DEPS := $(OBJS:.o=.d)

TARGETS = findpng

all : $(TARGETS)

findpng : $(OBJS)
	$(LD)  -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(BUILD_DIRECTORY)/%.o: %.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIRECTORY)/%.d: %.c
	$(CC) -MM -MF $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIRECTORY)/*.o $(BUILD_DIRECTORY)/*.d $(TARGETS) $(BUILD_DIRECTORY)

.PHONY: all clean

CC := gcc
CFLAGS := -g -MD

srcs := buddy.c main.c
objs := $(patsubst %.c, %.o, $(srcs))
test_elf := test-buddy-system

all: $(test_elf)

$(test_elf): $(objs)
	$(CC) $^ -o $@

-include *.d

clean:
	$(RM) $(objs) $(test_elf) *.d

.PHONY: all clean
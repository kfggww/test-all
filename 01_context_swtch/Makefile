
CROSS_COMPILE=riscv64-linux-gnu-
CC=$(CROSS_COMPILE)gcc

CFLAGS += -g

SRCS = $(wildcard *.c)
ASM_SRCS = $(wildcard *.S)
OBJS = $(patsubst %.c,%.o $(SRCS))
ASM_OBJS = $(patsubst %.S,%.o $(ASM_SRCS))

main: $(OBJS) $(ASM_OBJS)
	$(CC) $^ -o $@

%.o: %.S
	$(CC) -c $< -o $@

.dep:
	rm $@ -rf
	for src in $(SRCS); do $(CC) -MM $$src >> $@; done

include .dep

clean:
	-rm -rf *.o main .dep

.PHONY: clean
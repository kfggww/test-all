
CROSS_COMPILE=riscv64-linux-gnu-
CC=$(CROSS_COMPILE)gcc

CFLAGS += -g
Q = @

SRCS = $(wildcard *.c)
ASM_SRCS = $(wildcard *.S)
OBJS = $(patsubst %.c,%.o, $(SRCS))
ASM_OBJS = $(patsubst %.S,%.o, $(ASM_SRCS))

main: $(OBJS) $(ASM_OBJS)
	$(Q)$(CC) $^ -o $@

%.o: %.S
	$(Q)$(CC) -c $< -o $@

.dep:
	$(Q)rm $@ -rf
	$(Q)for src in $(SRCS); do $(CC) -MM $$src >> $@; done

clean:
	$(Q)-rm -rf *.o main .dep

.PHONY: clean

include .dep

srcs := $(shell find . -name *.c)
exes := $(patsubst %.c, %, $(srcs))

all: $(exes)
	@echo $(exes)

clean:
	@rm -rf $(exes)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean
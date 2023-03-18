CFLAGS := -g

all: debugger test

test: CFLAGS += -static

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
		rm -rf debugger test

.PHONY: all clean
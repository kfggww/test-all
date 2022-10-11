objs := a.out loader

all:

a.out: main.c
	$(CC) -static $< -Ttext=0x40000 -o $@

loader: elfloader.c
	$(CC) -g $< -o $@

clean:
	$(RM) $(objs)

all: $(objs)

.PHONY: all clean
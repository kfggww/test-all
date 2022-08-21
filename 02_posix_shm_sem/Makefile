SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))
EXES := $(patsubst %.o, %, $(OBJS))
CFLAGS += -g

all: $(EXES)

dep: $(DEPS)

%: %.o
	$(CC) $< -lrt -lpthread -o $@

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@

clean:
	rm -rf $(DEPS) $(OBJS) $(EXES)

.PHONY: all dep clean

-include $(DEPS)
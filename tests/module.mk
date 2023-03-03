
d := tests/algs
include $(d)/module.mk

d := tests/event
include $(d)/module.mk

%: %.o
	$(CC) $< $(LDFLAGS) -o $@
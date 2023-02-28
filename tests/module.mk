
d := tests/algs
include $(d)/module.mk

%: %.o
	$(CC) $< $(LDFLAGS) -o $@
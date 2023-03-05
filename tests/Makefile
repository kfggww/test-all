SHELL := /bin/bash

CFLAGS :=
ifeq ($(BUILD_TYPE), Debug)
CFLAGS += -g
endif
CFLAGS += -I$(INSTALL_DIR)/include

LDFLAGS := -Wl,-rpath=$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/lib -lcutest

test_objs :=
test_deps :=
test_exes :=

all:

# test algs
test_algs_srcs := $(wildcard algs/*.c)
test_algs_objs := $(test_algs_srcs:%.c=$(BUILD_DIR)/tests/%.o)
test_algs_deps := $(test_algs_objs:%.o=%.d)
test_algs_exes := $(test_algs_objs:%.o=%)

$(test_algs_exes): LDFLAGS += -lalgs

test_objs += $(test_algs_objs)
test_deps += $(test_algs_deps)
test_exes += $(test_algs_exes)


# test event
test_event_srcs := $(wildcard event/*.c)
test_event_objs := $(test_event_srcs:%.c=$(BUILD_DIR)/tests/%.o)
test_event_deps := $(test_event_objs:%.o=%.d)
test_event_exes := $(test_event_objs:%.o=%)

$(test_event_exes): LDFLAGS += -levent

test_objs += $(test_event_objs)
test_deps += $(test_event_deps)
test_exes += $(test_event_exes)


# test ipc
test_ipc_srcs := $(wildcard ipc/*.c)
test_ipc_objs := $(test_ipc_srcs:%.c=$(BUILD_DIR)/tests/%.o)
test_ipc_deps := $(test_ipc_objs:%.o=%.d)
test_ipc_exes := $(test_ipc_objs:%.o=%)

test_objs += $(test_ipc_objs)
test_deps += $(test_ipc_deps)
test_exes += $(test_ipc_exes)

$(test_exes): %: %.o
	$(CC) $^ $(LDFLAGS) -o $@

# pattern rules
$(BUILD_DIR)/tests/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) -MM -MT $@ > $(@:%.o=%.d)
	$(CC) -c $< $(CFLAGS) -o $@

all: $(test_exes)
	mkdir -p $(INSTALL_DIR)/bin
	cp $^ $(INSTALL_DIR)/bin

clean:
	rm -rf $(test_objs) $(test_deps) $(test_exes)

.PHONY: all clean

-include $(test_deps)
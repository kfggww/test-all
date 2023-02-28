PROJECT := test-all
VERSION := 0.01

BUILD_DIR := build
INSTALL_DIR := install

SHELL := /bin/bash
CC := gcc

CFLAGS :=
LDFLAGS := -L$(INSTALL_DIR)/lib
MAKEFLAGS := --no-print-directory

objs :=
shared_objs :=
interface_hdrs :=
tests :=
deps :=

all: install_tests

install_src:
	mkdir -p $(INSTALL_DIR)/{lib,include}
	cp $(shared_objs) $(INSTALL_DIR)/lib
	cp $(interface_hdrs) $(INSTALL_DIR)/include

install_tests:
	mkdir -p $(INSTALL_DIR)/bin
	cp $^ $(INSTALL_DIR)/bin

include src/module.mk
include tests/module.mk

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -MM -MT $@ > $(@:%.o=%.d)
	$(CC) -c $(CFLAGS) $< -o $@

third-party: cutest

cutest:
	make install $(MAKEFLAGS) INSTALL_DIR=$(shell realpath ./install) -C third-party/cutest

clean:
	rm -rf $(objs) $(shared_objs) $(tests) $(deps)

.PHONY: all clean third-party cutest
-include $(deps)

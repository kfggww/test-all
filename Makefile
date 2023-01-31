PROJECT_DIR := $(shell realpath .)

SHELL := /bin/bash

Q := @
BUILD_DIR := build
INSTALL_DIR := install

SRC_DIR := src
SRC_THIRD_PARTY_DIR := third-party

TEST :=

third_party_projects := $(addprefix $(SRC_THIRD_PARTY_DIR)/, $(shell ls $(SRC_THIRD_PARTY_DIR)))
common_inc := include $(INSTALL_DIR)/include
deps := $(BUILD_DIR)/.deps
test_cases = $(wildcard install/bin/*)

CFLAGS := -g
CFLAGS += $(addprefix -I, $(common_inc))
LDFLAGS := -L$(INSTALL_DIR)/lib -lcutest

obj_execs :=

all:

subproj := $(SRC_DIR)/syscalls
include $(subproj)/module.mk

all: third_party install

third_party:
	$(Q)$(call build_install_projects, $(third_party_projects))

install: $(obj_execs)
	$(Q)mkdir -p $(INSTALL_DIR)/bin
	$(Q)cp $^ $(INSTALL_DIR)/bin

clean:
	$(Q)rm -rf $(obj_execs) $(deps)

test: all
ifeq ($(TEST),)
	$(Q)$(call run_tests, $(test_cases))
else
	$(Q)$(call run_tests, install/bin/$(TEST))
endif

$(BUILD_DIR)/%: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) -c $(CFLAGS) $(LDFLAGS) -MM -MT $@ $< >> $(deps)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

define build_install_projects
for proj in $1; \
do \
	pushd $${proj}; \
	make --no-print-directory; \
	make --no-print-directory INSTALL_DIR=$(PROJECT_DIR)/$(INSTALL_DIR) install; \
	popd; \
done
endef

define run_tests
for test_case in $1; \
do \
	LD_LIBRARY_PATH=$(realpath install/lib) ./$${test_case}; \
done;
endef

-include $(deps)
.PHONY: all clean install third_party

PROJECT := test-all
VERSION := 0.01
BUILD_TYPE := Debug

PROJECT_DIR := $(realpath .)
BUILD_DIR := $(PROJECT_DIR)/build
INSTALL_DIR := $(PROJECT_DIR)/install

SHELL := /bin/bash
CC := gcc

TEST :=

all: third-party build_tests

build_src:
	$(MAKE) -ks -C src

build_tests: build_src
	$(MAKE) -ks -C tests

third-party:
	$(MAKE) -ks -C third-party/cutest install BUILD_DIR=$(BUILD_DIR)/third-party/cutest INSTALL_DIR=$(INSTALL_DIR)

clean:
	$(MAKE) -ks -C src clean
	$(MAKE) -ks -C tests clean
	$(MAKE) -ks -C third-party/cutest clean

run-test:
	python $(PROJECT_DIR)/script/runtests.py $(TEST)

.PHONY: all build_src build_tests third-party clean

export BUILD_TYPE PROJECT_DIR BUILD_DIR INSTALL_DIR CC
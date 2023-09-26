SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := help
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

BUILD := build
SOURCEDIR := src
DISTDIR := dist

BASE_SOURCES := src/array.cpp \
                src/builtin_words.cpp \
                src/byte_buffer.cpp \
                src/code_constructor.cpp \
                src/data_object.cpp \
                src/dictionary.cpp \
                src/error.cpp \
                src/hash_table.cpp \
                src/interpreter.cpp \
                src/location.cpp \
                src/operation_code.cpp \
                src/sorth.cpp \
                src/source_buffer.cpp \
                src/terminal_words.cpp \
                src/tokenize.cpp \
                src/user_words.cpp \
                src/value.cpp


ifeq ($(OS),Windows_NT)
	OS := Windows
else
	OS := $(shell uname -s)
endif
ifeq ($(OS),Darwin)
	ARCH ?= $(shell arch)
else ifeq ($(OS),Linux)
	ARCH ?= $(shell uname -m)
else ifeq ($(OS),Windows)
	ARCH ?= $(shell uname -m)
endif


ifeq ($(OS),Darwin)
	SOURCES = $(BASE_SOURCES) src/posix_io_words.cpp
else ifeq ($(OS),Linux)
	SOURCES = $(BASE_SOURCES) src/posix_io_words.cpp
else ifeq ($(OS),Windows)
	SOURCES = $(BASE_SOURCES) src/win_io_words.cpp
endif


BUILDFLAGS += "-std=c++20"
BUILDFLAGS += "-O3"
# BUILDFLAGS += "--target=$(ARCH)"

default: build build/sorth copy_stdlib ## Default

.PHONY: build
dist: default ## Dist
	install -d $(DISTDIR)
	zip -r $(DISTDIR)/sorth.zip $(BUILD)/*

help:  ## Help
	@grep -E -H '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		sed -n 's/^.*:\(.*\): \(.*\)##\(.*\)/\1%%%\3/p' | \
		column -t -s '%%%'

$(BUILD)/sorth: build $(SOURCES) ## Build binary
	clang++ \
		$(BUILDFLAGS) \
		$(SOURCES) \
		-o ./$@
	strip ./$@

.PHONY: build
 $(BUILD):
	install -d $(BUILD) || true

copy_stdlib: build std.f std/* ## Copy stdlib to build directory
	cp std.f $(BUILD)
	cp -r std $(BUILD)

.PHONY: clean
clean: ## Clean
	rm -rf build || true
	rm -rf dist || true
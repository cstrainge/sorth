SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := help
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

BUILD := build
SOURCEDIR := src
DISTDIR := dist

SOURCES := $(wildcard $(SOURCEDIR)/*.cpp)
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
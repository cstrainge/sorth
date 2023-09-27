SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := help
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

CXX := clang++
CXXFLAGS = -O3 -std=c++20
GCC_CXXFLAGS = -DMESSAGE='"Compiled with GCC"'
CLANG_CXXFLAGS = -DMESSAGE='"Compiled with Clang"'
UNKNOWN_CXXFLAGS = -DMESSAGE='"Compiled with an unknown compiler"'

BUILD := build
SOURCEDIR := src
DISTDIR := dist
TARGET ?= sorth

SOURCES := $(shell find $(SOURCEDIR) -name '*.cpp')

ifneq ($(OS),Windows_NT)
	OS := $(shell uname -s)
endif

# versioning
TAG_COMMIT := $(shell git rev-list --abbrev-commit --tags --max-count=1)
DATE := $(shell git log -1 --format=%cd --date=format:"%Y%m%d")
# `2>/dev/null` suppress errors and `|| true` suppress the error codes.
TAG := $(shell git describe --abbrev=0 --tags ${TAG_COMMIT} 2>/dev/null || true)
ifeq ($(TAG),)
	TAG := dev
endif
COMMIT :=$(shell git rev-parse HEAD | cut -c 1-8)
# here we strip the version prefix
VERSION := $(TAG:v%=%)-$(COMMIT)-$(DATE)

CXXFLAGS += -DVERSION="$(VERSION)" 

# target, optional https://clang.llvm.org/docs/CrossCompilation.html
ifneq ($(CXXTARGET),)
	CXXFLAGS += -target $(CXXTARGET)
endif 

ifeq ($(CXX),g++)
  CXXFLAGS += $(GCC_CXXFLAGS)
else ifeq ($(CXX),clang)
  CXXFLAGS += $(CLANG_CXXFLAGS)
else ifeq ($(CXX),c++) # not sure if this is a good assumption, but it works for CI and local builds on MacOS
  CXXFLAGS += $(CLANG_CXXFLAGS)
else
  CXXFLAGS += $(UNKNOWN_CXXFLAGS)
endif


ifeq ($(OS),Darwin)
	CP_CMD := cp std.f $(BUILD)
	CP_R := cp -r std $(BUILD)
else ifeq ($(OS),Linux)
	CXXFLAGS += -fuse-ld=lld
# optionally use libatomic on arm
ifeq ($(CXXTARGET),arm-unknown-linux-gnu)
	CXXFLAGS += -latomic -mfloat-abi=soft
endif
	CP_CMD := cp std.f $(BUILD)
	CP_R := cp -r std $(BUILD)
else ifeq ($(OS),Windows_NT)
	TARGET := $(TARGET).exe
	CP_CMD := robocopy "." "$(BUILD)" "std.f" || true
	CP_R := robocopy "std" "$(BUILD)\std" /e || true
endif

default: $(BUILD)/$(TARGET) copy_stdlib strip ## Default

help:  ## Help
	@grep -E -H '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		sed -n 's/^.*:\(.*\): \(.*\)##\(.*\)/\1%%%\3/p' | \
		column -t -s '%%%'

$(BUILD)/$(TARGET): $(SOURCES) ## Build binary
	install -d $(BUILD) || true
	$(CXX) \
		$(CXXFLAGS) \
		$(SOURCES) \
		-o ./$(BUILD)/$(TARGET)

.PHONY: strip
strip: $(BUILD)/$(TARGET)
ifeq ($(OS),Darwin)
	strip -x $(BUILD)/$(TARGET)
else ifeq ($(OS),Linux)
ifneq ($(CXXTARGET),arm-unknown-linux-gnu)
	strip -s $(BUILD)/$(TARGET)
endif
else ifeq ($(OS),Windows_NT)
	strip $(BUILD)/$(TARGET)
endif

copy_stdlib: $(BUILD)/std.f $(BUILD)/std/* ## Copy stdlib to build directory

$(BUILD)/std.f: std.f ## Copy std.f to build directory
	$(CP_CMD)

$(BUILD)/std/*: std/* ## Copy std/* to build directory
	$(CP_R)

bundle: ## Bundle binary assets for vsce

vsce: bundle ## Build VSCode extension
	cde strange-forth
	vsce package

.PHONY: clean
clean: ## Clean
	rm -rf build || true
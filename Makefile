SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := help
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

CXX := clang++
CXX_VERSION := $(shell $(CXX) --version)

ifeq ($(OS),Windows_NT)
	CXX_VERSION := $(subst \,\\\\,$(CXX_VERSION))
endif


CXXFLAGS = -O3 -std=c++20
LINKFLAGS = ""
GCC_CXXFLAGS = -DMESSAGE='"Compiled with GCC: $(CXX_VERSION)"'
CLANG_CXXFLAGS = -DMESSAGE='"Compiled with Clang: $(CXX_VERSION)"'
UNKNOWN_CXXFLAGS = -DMESSAGE='"Compiled with an unknown compiler"'

BUILD := build
SOURCEDIR := src
DISTDIR := dist
TARGET ?= sorth

ifneq ($(OS),Windows_NT)
	OS := $(shell uname -s)
endif

# versioning
TAG_COMMIT := $(shell git rev-list --abbrev-commit --tags --max-count=1)
DATE := $(shell git log -1 --format=%cd --date=format:"%Y.%m.%d")
# `2>/dev/null` suppress errors and `|| true` suppress the error codes.
TAG := $(shell git describe --abbrev=0 --tags ${TAG_COMMIT} 2>/dev/null || true)
ifeq ($(TAG),)
	TAG := dev
endif
COMMIT :=$(shell git rev-parse HEAD | cut -c 1-8)
# here we strip the version prefix
VERSION := $(TAG:v%=%)-$(COMMIT)-$(DATE)

CXXFLAGS += -DVERSION=\"$(VERSION)\"

# target, optional https://clang.llvm.org/docs/CrossCompilation.html
ifneq ($(CXXTARGET),)
	CXXFLAGS += -target $(CXXTARGET)
endif

ifeq ($(CXX),g++)
  CXXFLAGS += $(GCC_CXXFLAGS)
else ifeq ($(CXX),clang++)
  CXXFLAGS += $(CLANG_CXXFLAGS)
  CXXFLAGS += -Wno-vla-extension
else ifeq ($(CXX),c++) # not sure if this is a good assumption, but it works for CI and local builds on MacOS
  CXXFLAGS += $(CLANG_CXXFLAGS)
  CXXFLAGS += -Wno-vla-extension
else
  CXXFLAGS += $(UNKNOWN_CXXFLAGS)
endif


ifeq ($(OS),Darwin)
	CP_CMD := cp std.f $(BUILD)
	CP_R := cp -r std $(BUILD)
else ifeq ($(OS),Linux)
	LINKFLAGS += -fuse-ld=lld -ldl -lffi -lm

	ifeq ($(CXXTARGET),x86_64-unknown-linux-gnu)
		LINKFLAGS += -static -stdlib=libc++
	else ifeq ($(CXXTARGET),arm-unknown-linux-gnu)
	# optionally use libatomic on arm
		LINKFLAGS += -latomic -mfloat-abi=soft
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

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

$(BUILD)/$(TARGET): $(OBJS) ## Build binary
	install -d $(BUILD) || true
	$(CXX) \
		$(CXXFLAGS) \
		$(LINKFLAGS) \
		$(OBJS) \
		-o ./$(BUILD)/$(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	install -d $(OBJ_DIR) || true
	$(CXX) $(CXXFLAGS) -c $< -o $@

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

copy_stdlib: $(BUILD)/std.f $(BUILD)/std/*  ## Copy stdlib to build directory.

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
	rm -rf $(BUILD) || true
	rm -rf $(OBJ_DIR) || true
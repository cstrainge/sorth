SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c  
.ONESHELL:
.DEFAULT_GOAL := help

MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

help:  ## Help
	@grep -E -H '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		sed -n 's/^.*:\(.*\): \(.*\)##\(.*\)/\1%%%\3/p' | \
		column -t -s '%%%'

sorth: src/*.cpp ## Build
	clang++ \
		-std=c++20 \
		$^ \
		-O3 \
		-o $@
	strip ./$@

clean: ## Clean
	rm -f sorth
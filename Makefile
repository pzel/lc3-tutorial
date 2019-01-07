CC ?= gcc
.PHONY: build clean test unit

lc:

clean:
	@rm -rf core*

build:
	@find asm-src | grep 'asm$$' | xargs -L1 lc3as

test: clean unit lc
	@./tests.rb

unit: lc.c
	@$(CC) $< -DTEST -o $@ && ./$@

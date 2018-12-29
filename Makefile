CC = tcc
.PHONY: build test

lc:

build:
	find asm-src | grep 'asm$$' | xargs -L1 lc3as

test: lc
	./tests.rb

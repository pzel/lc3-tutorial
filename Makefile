CC := tcc
.PHONY: build clean test unit top

top: test

lc: lc.c binlit.c

clean:
	@rm -rf core*

build:
	@find asm-src | grep 'asm$$' | xargs -L1 lc3as

test: clean unit lc
	@./tests.rb

unit: lc.c binlit.c
	@$(CC) $^ -DTEST -o $@ && ./$@

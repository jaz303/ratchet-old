main: main.c
	gcc -Werror -o $@ $<

clean:
	rm -f main

loc:
	cat *.c *.x | wc -l

.PHONY: clean loc
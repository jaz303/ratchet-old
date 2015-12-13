main: main.c
	gcc -Werror -o $@ $<

clean:
	rm -f main

.PHONY: clean
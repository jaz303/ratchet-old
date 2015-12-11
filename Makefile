main: main.c
	gcc -o $@ $<

clean:
	rm -f main

.PHONY: clean
main: main.cpp *.inc.cpp *.x
	g++ -Werror -o $@ $<

clean:
	rm -f main

loc:
	cat *.cpp *.x | wc -l

.PHONY: clean loc
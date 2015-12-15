main: main.cpp *.inc.cpp *.x
	g++ -Werror -o $@ $<

clean:
	rm -f main

loc:
	cat *.cpp *.x | wc -l

todo:
	@egrep -n '(TODO|FIXME)' *.cpp *.x

.PHONY: clean loc todo
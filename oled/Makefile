CPP = g++ -O3 -Wall -Werror -MD -c

all: fonttable reduce stringtable

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f font

reduce: objs/reduce.o
	g++ -O3 -Wall -Werror $< -o $@

stringtable: objs/stringtable.o
	g++ -O3 -Wall -Werror $< -o $@
	
fonttable: objs/font.o objs/fonttable.o
	g++ -O3 -Wall -Werror objs/font.o objs/fonttable.o -o $@

objs/font.o: font.cpp
	$(CPP) -c $< -o $@

objs/fonttable.o: fonttable.cpp
	$(CPP) -c $< -o $@

objs/reduce.o: reduce.cpp
	$(CPP) -c $< -o $@

objs/stringtable.o: stringtable.cpp
	$(CPP) -c $< -o $@
	

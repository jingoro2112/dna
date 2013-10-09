CPP = g++ -ggdb -O3 -Wall -Werror -MD -c

all: hex2header

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs

hex2header: objs/hex2header.o
	g++ -O3 -Wall -Werror $< -o $@
	
objs/hex2header.o: hex2header.cpp
	$(CPP) -c $< -o $@
	

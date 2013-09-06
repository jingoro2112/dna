CPP = g++ -O3 -Wall -Werror -MD -c

all: splice

-include objs/*.d

clean:  
	@-rm -rf objs splice
	@-mkdir objs

splice: objs/splice.o objs/main.o ../../dnausb/linux/libdnausb.a
	g++ objs/splice.o objs/main.o ../../dnausb/linux/libdnausb.a -o splice -lrt -lusb-1.0

objs/splice.o: ../splice.cpp
	$(CPP) -c $< -o $@

objs/main.o: ../main.cpp
	$(CPP) -c $< -o $@

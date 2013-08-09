CPP = g++ -O3 -Wall -Werror -MD -c

all: loader

-include objs/*.d

clean:  
	@-rm -rf objs loader
	@-mkdir objs

loader: objs/loader.o
	g++ objs/loader.o ../../dnausb/linux/libdnausb.a -lusb-1.0 -o loader -lrt

objs/loader.o: ../loader.cpp
	$(CPP) -c $< -o $@

CPP = g++ -O3 -Wall -Werror -MD -c

all: loader

-include objs/*.d

clean:  
	@-rm -rf objs loader
	@-mkdir objs

loader: objs/loader.o objs/main.o
	g++ objs/loader.o objs/main.o ../../dnausb/linux/libdnausb.a -o loader -lrt

objs/loader.o: ../loader.cpp
	$(CPP) -c $< -o $@

objs/main.o: ../main.cpp
	$(CPP) -c $< -o $@

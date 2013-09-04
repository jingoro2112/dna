CPP = g++ -O3 -Wall -Werror -MD -c

all: loader

-include objs/*.d

clean:  
	@-rm -rf objs loader
	@-mkdir objs

loader: objs/loader.o objs/main.o ../../dnausb/linux/libdnausb.a
	g++ objs/loader.o objs/main.o ../../dnausb/linux/libdnausb.a -o loader -lrt -lusb-1.0

objs/loader.o: ../loader.cpp
	$(CPP) -c $< -o $@

objs/main.o: ../main.cpp
	$(CPP) -c $< -o $@

#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

CPP = g++ -m32 -O3 -Wall -Werror -MD -c

all: splice

-include objs/*.d

clean:  
	@-rm -rf objs splice
	@-mkdir objs

splice: objs/splice.o objs/main.o
	g++ -m32 objs/splice.o objs/main.o ../../dnausb/mac/libdnausb.a -o splice -framework IOKit -framework CoreFoundation -framework CoreServices

objs/splice.o: ../splice.cpp
	$(CPP) -c $< -o $@

objs/main.o: ../main.cpp
	$(CPP) -c $< -o $@

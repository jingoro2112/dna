#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

CPP = g++ -m32 -O3 -Wall -Werror -MD -c

all: loader

-include objs/*.d

clean:  
	@-rm -rf objs loader
	@-mkdir objs

loader: objs/loader.o
	g++ -m32 objs/loader.o ../../dnausb/mac/libdnausb.a -o loader -framework IOKit -framework CoreFoundation -framework CoreServices

objs/loader.o: ../loader.cpp
	$(CPP) -c $< -o $@

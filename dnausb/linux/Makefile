#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

OBJS = \
	objs/dnausb.o \
	objs/dnausb_common.o \
	objs/hid-libusb.o \

CPP = g++ -fPIC -Ihidapi -O3 -Wall -Werror -MD -c
CC = cc -fPIC -Ihidapi -O3 -MD -c

all: libdnausb.a

-include objs/*.d

clean:
	@-rm -rf objs libdnausb.a
	@-mkdir objs
	
libdnausb.a: $(OBJS)
	ar r libdnausb.a $(OBJS)
	ranlib libdnausb.a

objs/dnausb.o: dnausb.cpp
	$(CPP) -c $< -o $@

objs/dnausb_common.o: ../dnausb_common.cpp
	$(CPP) -c $< -o $@

objs/hid-libusb.o: hidapi/hid-libusb.c
	$(CC) -c $< -o $@


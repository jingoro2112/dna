OBJS = \
	objs/dnausb.o \
	objs/hid-libusb.o \

CPP = g++ -fPIC -Ihidapi -O3 -Wall -Werror -MD -c
CC = cc -fPIC -Ihidapi -O3 -MD -c

all: libdnausb.a

-include objs/*.d

clean:
	@-rm -rf objs libdnausb.a
	@-mkdir objs
	
libdnausb.a: $(OBJS)
	ar r libdnausb.a -lusb-1.0 $(OBJS)
	ranlib libdnausb.a

objs/dnausb.o: dnausb.cpp
	$(CPP) -c $< -o $@

objs/hid-libusb.o: hidapi/hid-libusb.c
	$(CC) -c $< -o $@


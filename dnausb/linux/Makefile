OBJS = \
	objs/dnausb.o \
	objs/hid-libusb.o \

CPP = g++ -Ihidapi -O0 -Wall -Werror -MD -c
CC = cc -Ihidapi -O0 -MD -c

all: libdnausb.a loader

-include objs/*.d

clean:
	@-rm -rf objs libdnausb.a
	@-mkdir objs
	
libdnausb.a: $(OBJS)
	ar r libdnausb.a $(OBJS)
	ranlib libdnausb.a

loader: objs/loader.o libdnausb.a
	 g++ objs/loader.o libdnausb.a -lusb-1.0 -lpthread -o loader -lrt

objs/dnausb.o: dnausb.cpp
	$(CPP) -c $< -o $@

objs/loader.o: ../../loader/loader.cpp
	$(CPP) -c $< -o $@

objs/hid-libusb.o: hidapi/hid-libusb.c
	$(CC) -c $< -o $@


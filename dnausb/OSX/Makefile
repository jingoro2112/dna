OBJS = \
	objs/dnausb.o \
	objs/dnausb_common.o \
	objs/hid.o \

#CPP = g++ -m32 -Ihidapi -O3 -Wall -Werror -MD -c
#CC = cc -m32 -Ihidapi -O3 -MD -c
CPP = g++ -Ihidapi -O3 -Wall -Werror -MD -c
CC = cc -Ihidapi -O3 -MD -c

all: libdnausb.a

-include objs/*.d

clean:
	@-rm -rf objs libdnausb.a
	@-mkdir objs
	
libdnausb.a: $(OBJS)
	ar r libdnausb.a $(OBJS)
	ranlib libdnausb.a

objs/dnausb.o: ../linux/dnausb.cpp
	$(CPP) -c $< -o $@

objs/dnausb_common.o: ../dnausb_common.cpp
	$(CPP) -c $< -o $@

objs/hid.o: hidapi/hid.c
	$(CC) -c $< -o $@


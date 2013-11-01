CPP = g++ -I../firmware/dna -ggdb -O0 -Wall -Werror -MD -c

OBJS = \
	objs/font.o \
	objs/eeprom.o \

all: eeprom

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs

eeprom: $(OBJS)
	g++ -O0 -Wall -lz -lpng -Werror $(OBJS) -o $@

objs/font.o: ../util/font.cpp
	$(CPP) -c $< -o $@

objs/fonttable.o: fonttable.cpp
	$(CPP) -c $< -o $@

objs/reduce.o: reduce.cpp
	$(CPP) -c $< -o $@

objs/eeprom.o: eeprom.cpp
	$(CPP) -c $< -o $@
	

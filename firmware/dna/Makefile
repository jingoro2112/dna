#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

OBJS = \
	objs/usbdrvasm.o \
	objs/usbdrv.o \
	objs/dna.o \
	objs/usb.o \
	objs/a2d.o \

MCU = attiny84
#MCU = atmega88
CPU_FREQUENCY = 12000000

CC = avr-gcc -Wall -I../dna -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

all: libdna.a

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f libdna.a

-include objs/*.d

libdna.a: $(OBJS)
	avr-ar rcs libdna.a $(OBJS)

objs/usbdrv.o: ../../usbdrv/usbdrv.c
	$(CC) -c $< -o $@

objs/usbdrvasm.o: ../../usbdrv/usbdrvasm.S
	$(CC) -c -x assembler-with-cpp $< -o $@

objs/dna.o: dna.c
	$(CC) -c $< -o $@

objs/usb.o: usb.c
	$(CC) -c $< -o $@

objs/a2d.o: a2d.c
	$(CC) -c $< -o $@

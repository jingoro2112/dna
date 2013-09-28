#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = blank_app

OBJS = \
	objs/usbdrvasm.o \
	objs/dna.o \
	objs/usb.o \
	objs/main.o \
	objs/galloc.o \

INCLUDES = \
	-I. \
	-I../../dna \

MCU = attiny84
CPU_FREQUENCY = 12000000

CC = avr-gcc -Wall $(INCLUDES) -MD -Os -std=gnu99 -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

all: $(TARGET).hex

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f $(OBJS) $(TARGET).hex $(TARGET).elf $(TARGET).disassembly

$(TARGET).hex: $(OBJS)
	$(CC) -o $(TARGET).elf -Wl,-gc-sections $(OBJS) 
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-objdump -d $(TARGET).elf > $(TARGET).disassembly

objs/usbdrvasm.o: ../../../usbdrv/usbdrvasm.S
	$(CC) -c -x assembler-with-cpp $< -o $@

objs/dna.o: ../../dna/dna.c
	$(CC) -c $< -o $@

objs/usb.o: ../../dna/usb.c
	$(CC) -c $< -o $@

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/galloc.o: ../../dna/galloc.c
	$(CC) -c $< -o $@

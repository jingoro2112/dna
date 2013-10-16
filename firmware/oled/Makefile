#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = oled

OBJS = \
	objs/rna.o \
	objs/main.o \
	objs/i2c_brute.o \
	objs/cfal12832.o \
	objs/24c512.o \
	objs/sram.o \
	objs/galloc.o \
	objs/dstring.o \
	objs/menu.o \
	objs/text.o \
	objs/frame.o \

MCU = attiny84
CPU_FREQUENCY = 8000000

CC = avr-gcc -Wall -I. -I../dna -I../../usbdrv -MD -Os -std=gnu99 -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

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

splice: $(TARGET).hex
	..\..\splice\win32\splice -o oled.hex

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/menu.o: menu.c
	$(CC) -c $< -o $@

objs/text.o: ../dna/text.c
	$(CC) -c $< -o $@

objs/i2c_brute.o: ../dna/i2c_brute.c
	$(CC) -c $< -o $@

objs/dstring.o: ../dna/dstring.c
	$(CC) -c $< -o $@

objs/usb.o: ../dna/usb.c
	$(CC) -c $< -o $@

objs/cfal12832.o: ../dna/cfal12832.c
	$(CC) -c $< -o $@

objs/24c512.o: ../dna/24c512.c
	$(CC) -c $< -o $@

objs/sram.o: ../dna/sram.c
	$(CC) -c $< -o $@

objs/rna.o: ../dna/rna.c
	$(CC) -c $< -o $@

objs/galloc.o: ../dna/galloc.c
	$(CC) -c $< -o $@

objs/frame.o: frame.c
	$(CC) -c $< -o $@

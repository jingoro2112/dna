#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

OBJS = \
	objs/rna.o \
	objs/main.o \

TARGET = bootloader
MCU = atmega88
CPU_FREQUENCY = 8000000
BOOTLOADER_ENTRY = 1C00
#BOOTLOADER_ENTRY = 0

CC = avr-gcc -std=gnu99 -Wall -I. -I../../dna -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

all: $(TARGET).hex

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f $(OBJS) $(TARGET).hex $(TARGET).elf $(TARGET).disassembly
	
$(TARGET).hex: $(OBJS)
	$(CC) -T avr4_custom.x  -Wl,--section-start=.text=$(BOOTLOADER_ENTRY),-gc-sections -o $(TARGET).elf $(OBJS)
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-objdump -d $(TARGET).elf > $(TARGET).disassembly

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/rna.o: ../../dna/rna.c
	$(CC) -c $< -o $@


#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = telemetry

OBJS = \
	objs/main.o \
	objs/rna.o \
	objs/i2c_atmega.o \
	objs/galloc.o \

INCLUDE = \
	-I. \

MCU = atmega88pa
CPU_FREQUENCY = 8000000

CC = avr-gcc -Wall -I. -I../dna -MD -Os -std=gnu99 -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

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
	../../splice/win32/splice -4 $(TARGET).hex

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/i2c_atmega.o: ../dna/i2c_atmega.c
	$(CC) -c $< -o $@

objs/dstring.o: ../dna/dstring.c
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

#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = oled

OBJS = \
	objs/rna.o \
	objs/main.o \

MCU = attiny84
CPU_FREQUENCY = 8000000
PLATFORM = OLED

CC = avr-gcc -Wall -I. -I../../dna -DRNA_POLL_DRIVEN=1 -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -D$(PLATFORM)=1 -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

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

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/rna.o: ../../dna/rna.c
	$(CC) -c $< -o $@

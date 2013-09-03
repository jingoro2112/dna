#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = button

OBJS = \
	objs/main.o \
	objs/rna.o \

BOBJS = \
	objs/rna_bootloader.o

MCU = attiny45
CPU_FREQUENCY = 8000000
PLATFORM = BUTTON

CC = avr-gcc -Wall -I. -I../dna -MD -Os -std=gnu99 -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -D$(PLATFORM)=1 -DF_CPU=$(CPU_FREQUENCY)

all: $(TARGET).hex $(TARGET)_bootloader.hex

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f $(OBJS) $(TARGET).hex $(TARGET).elf $(TARGET).disassembly
	
$(TARGET)_bootloader.hex: $(BOBJS)
	$(CC) -o $(TARGET)_bootloader.elf -Wl,-gc-sections $(OBJS) 
	avr-objcopy -j .text -j .data -O ihex $(TARGET)_bootloader.elf $(TARGET)_bootloader.hex
	avr-objdump -d $(TARGET)_bootloader.elf > $(TARGET)_bootloader.disassembly


$(TARGET).hex: $(OBJS)
	$(CC) -o $(TARGET).elf -Wl,-gc-sections $(OBJS) 
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-objdump -d $(TARGET).elf > $(TARGET).disassembly
#	..\..\loader\loader -i $(TARGET).hex

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/rna.o: ../dna/rna.c
	$(CC) -c $< -o $@

objs/rna_bootloader.o: ../dna/rna.c
	$(CC) -DRNA_POLL_DRIVEN -c $< -o $@

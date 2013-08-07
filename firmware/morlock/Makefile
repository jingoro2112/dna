#* Copyright: (c) 2013 by Curt Hartung
#* This work is released under the Creating Commons 3.0 license
#* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
#* and in the LICENCE.txt file included with this distribution

TARGET = morlock

OBJS = \
	objs/usbdrvasm.o \
	objs/dna.o \
	objs/usb.o \
	objs/main.o \
	objs/a2d.o \

MCU = attiny84
CPU_FREQUENCY = 12000000

CC = avr-gcc -Wall -I../dna -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

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
#	..\..\loader\loader -i $(TARGET).hex

objs/usbdrvasm.o: ../../usbdrv/usbdrvasm.S
	$(CC) -c -x assembler-with-cpp $< -o $@

objs/dna.o: ../dna/dna.c
	$(CC) -c $< -o $@

objs/usb.o: ../dna/usb.c
	$(CC) -c $< -o $@

objs/main.o: main.c
	$(CC) -c $< -o $@

objs/rna_master.o: ../rna/rna_master.c
	$(CC) -c $< -o $@

objs/a2d.o: ../dna/a2d.c
	$(CC) -c $< -o $@

objs/i2c.o: ../dna/i2c.c
	$(CC) -c $< -o $@

objs/24c512.o: ../dna/24c512.c
	$(CC) -c $< -o $@

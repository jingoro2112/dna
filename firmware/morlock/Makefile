TARGET = morlock

OBJS = \
	objs/usbdrvasm.o \
	objs/usbdrv.o \
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
	avr-objdump -d $(TARGET).elf >> $(TARGET).disassembly
#	..\..\loader\loader -i $(TARGET).hex

objs/usbdrv.o: ../../usbdrv/usbdrv.c
	$(CC) -c $< -o $@

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

objs/a2d.o: ../a2d/a2d.c
	$(CC) -c $< -o $@

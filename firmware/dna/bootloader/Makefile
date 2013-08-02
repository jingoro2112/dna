TARGET = bootloader
MCU = attiny84
CPU_FREQUENCY = 12000000
BOOTLOADER_ADDRESS = 18C0

OBJS = \
	objs/usbdrvasm.o \
	objs/usbdrv.o \
	objs/bootloader.o \

CC = avr-gcc -Wall -DRELOCATE -I. -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)
#CC = avr-gcc -Wall -I. -MD -Os -fdata-sections -ffunction-sections -funsigned-char -funsigned-bitfields -fno-inline-small-functions -mmcu=$(MCU) -DF_CPU=$(CPU_FREQUENCY)

all: $(TARGET).hex

-include objs/*.d

clean:
	@-rm -rf objs
	@-mkdir objs
	@-rm -f $(OBJS) $(TARGET).hex $(TARGET).elf $(TARGET).disassembly
	
$(TARGET).hex: $(OBJS)
	$(CC) -T avr25_custom.x  -Wl,--section-start=.text=$(BOOTLOADER_ADDRESS),-gc-sections -o $(TARGET).elf $(OBJS)
#	$(CC) -Wl,-gc-sections -o $(TARGET).elf $(OBJS)
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-objdump -d $(TARGET).elf >> $(TARGET).disassembly

objs/usbdrv.o: ../../../usbdrv/usbdrv.c
	$(CC) -c $< -o $@

objs/usbdrvasm.o: ../../../usbdrv/usbdrvasm.S
	$(CC) -c -x assembler-with-cpp $< -o $@

objs/bootloader.o: bootloader.c
	$(CC) -c $< -o $@
A simple program that sits around blinking the light and waiting for code to be
loaded into it. The bootloader re-writes the interrupt vector table to do a long ijmp so the V-USB software
can function properly on INT0, it also modifies the RESET vector to bring chip reset back to the bootloader,
once entered.


This bootloader compiles to exactly 1792 bytes (28 pages) but only on gcc-avr on linux. For some reason
the win-avr wants to make it a bit bigger.

Not exactly sure why, the same options/makefile are used on both platforms *shrug* well anyways if for some
reason you want to build it, you need to use linux.

Anyways its basically a simple program that sits around blinking the light and waiting for code to be
loaded into it. The bootloader re-writes the interrupt vector table to do a long ijmp so the V-USB software
can function properly on INT0, it also modifies the RESET vector to bring chip reset back to the bootloader,
once entered.

Because of this, page zero is stored in RAM and only committed once the command has been recieved to enter
the application, the 8-bit checksum must match as added safety.

The bootloader refuses to commit a page with a base address higher than 0x18C0, since it is located 64
bytes above this location at 0x18E0; it canont overwrite itself.



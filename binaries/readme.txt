Programming
===========

FUSEs
=====
AVRDUDE command line for writing the FUSE settings to the chip (required once):

avrdude -c <programmer> -P <port,optional> -p <target-mcu> -U lfuse:w:0xAA:m -U hfuse:w:0xBB:m -U efuse:w:0xCC:m -U lock:w:0xDD:m


1) avrispmkii + 168

   avrdude -c avrispmkii -p atmega168 -U lfuse:w:0xE2:m -U hfuse:w:0xDD:m -U efuse:w:0x05:m -U lock:w:0x0F:m
   
2) usbtiny + 168

   avrdude -c usbtiny -P usb -p atmega168 -U lfuse:w:0xE2:m -U hfuse:w:0xDD:m -U efuse:w:0x05:m -U lock:w:0x0F:m

3) usbasp + 168

   avrdude -c usbasp -p atmega168 -v -U lfuse:w:0xE2:m -U hfuse:w:0xDD:m -U efuse:w:0x05:m -U lock:w:0x0F:m

4) avrispmkii + 328p

   avrdude -c avrispmkii -p atmega328p -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0x05:m -U lock:w:0x0F:m
   
5) usbtiny + 328p

   avrdude -c usbtiny -P usb -p atmega328p -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0x05:m -U lock:w:0x0F:m

6) usbasp + 328p

   avrdude -c usbasp -p atmega328p -v -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0x05:m -U lock:w:0x0F:m



Fuse settings to run the ATmega168 [328P] using its internal RC oscillator at 8MHz, BOD: 2.7V, NO bootloader, ...

lock-bits: 0xFF [0xFF]

L-FUSE: 0xE2 [0xE2]
H-FUSE: 0xDD [0xDF]
E-FUSE: 0xFD [0xFD]


If you use avrdude you should use:

lock-bits: 0x0F [0x0F]

L-FUSE: 0xE2 [0xE2]
H-FUSE: 0xDD [0xDF]
E-FUSE: 0x05 [0x05]

"Note that some numerical values refer to fuses containing undefined bits (set to '1' here).
Depending on the target device these fuse bits will be read either as '0' or '1'.
Verification errors will occur if the values are read back with undefined bits set to '0'.
Everything is fine if the values read from the device are either the same as programmed,
or the following values (undefined set to '0'): Extended: 0x05." (1)

AVR FUSE calculator:
====================

(1) http://www.engbedded.com/fusecalc
(2) http://eleccelerator.com/fusecalc/fusecalc.php



Fuse settings to run the ATmega168 using its internal RC oscillator at 8MHz, BOD: 2.7V, NO bootlaoder, ...

L-FUSE: 0xE2
H-FUSE: 0xDD
E-FUSE: 0xFD

If you use avrdude you may also use:

L-FUSE: 0xE2
H-FUSE: 0xDD
E-FUSE: 0x05

"Note that some numerical values refer to fuses containing undefined bits (set to '1' here).
Depending on the target device these fuse bits will be read either as '0' or '1'.
Verification errors will occur if the values are read back with undefined bits set to '0'.
Everything is fine if the values read from the device are either the same as programmed,
or the following values (undefined set to '0'): Extended: 0x05." (1)

AVR FUSE calculator:
====================

(1) http://www.engbedded.com/fusecalc
(2) http://eleccelerator.com/fusecalc/fusecalc.php

~                                                                                                                                                
~                                                                                                                                                
~                                                                                                                                                
~                                                                                                                                                
~                                                                                                                                                
~                                                                                                                                                
~                                  

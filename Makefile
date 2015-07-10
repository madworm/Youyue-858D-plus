# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

BOARD_TAG   = lilypad
ARDUINO_DIR = ${HOME}/bin/arduino-1.6.5
BOARD_SUB	= atmega328
# BOARD_SUB	= atmega168
ISP_PROG	= atmelice_isp
ISP_PORT	= usb
ISP_LOW_FUSE       = 0xe2
ISP_HIGH_FUSE      = 0xdf
ISP_EXT_FUSE       = 0xfd
AVRDUDE		= /usr/bin/avrdude
AVRDUDE_CONF = /etc/avrdude.conf
AVRDUDE_ARD_PROGRAMMER = atmelice_isp
include ./Arduino-Makefile/Arduino.mk

# fuses for atmega168
#
#ISP_LOW_FUSE       = 0xe2
#ISP_HIGH_FUSE      = 0xdd
#ISP_EXT_FUSE       = 0xff

# Hardware configuration settings.
MCU = atmega1284p
F_CPU = 7300000
BAUD = 115200

# Tool aliases.
CC = avr-gcc
STRIP  = avr-strip
OBJCOPY = avr-objcopy

# Compiler configurations.
CDEFS = -mmcu=${MCU} -DF_CPU=${F_CPU} -DBAUD=${BAUD}
CLINKER = -nostartfiles -Wl,--section-start=.text=0x1E000
CWARN =  -Wall
COPT = -std=gnu99 -Os -fno-tree-scev-cprop -mcall-prologues \
       -fno-inline-small-functions -fsigned-char

CFLAGS  = $(CDEFS) $(CLINKER) $(CWARN) $(COPT)

# Include file paths.
INCLUDES = -I . -I ./AES_lib

# Run clean even when all files have been removed.
.PHONY: clean

OFILES = nope.c

all:    flash.hex eeprom.hex
	@echo  Simple bootloader has been compiled and packaged as intel hex.

uart.o : uart.c uart.h
	$(CC) $(CFLAGS) $(INCLUDES) -c uart.c

AES_lib.o : AES_lib.c AES_lib.h 
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib.c

main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c main.c

bootloader.elf: uart.o AES_lib.o main.o
	$(CC) $(CFLAGS) $(INCLUDES) -o bootloader.elf uart.o AES_lib.o main.o

strip: bootloader.elf
	$(STRIP) bootloader.elf -o bootloader.elf

flash.hex: strip
	$(OBJCOPY) -R .eeprom -O ihex bootloader.elf flash.hex

eeprom.hex: strip
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex bootloader.elf eeprom.hex

clean:
	$(RM) -v *.hex *.o *.elf $(MAIN)


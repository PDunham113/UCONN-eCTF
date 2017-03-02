# Hardware configuration settings.
MCU = atmega1284p
F_CPU =7300000
BAUD = 115200

# Tool aliases.
CC = avr-gcc
STRIP  = avr-strip
OBJCOPY = avr-objcopy
PROGRAMMER = dragon_isp

# Compiler configurations.
CDEFS = -mmcu=${MCU} -DF_CPU=${F_CPU} -DBAUD=${BAUD} -mrelax -funsigned-char -funsigned-bitfields -g2
CLINKER = -nostartfiles -Wl,-Map,map.map,--section-start=.text=0x1E000,--defsym=__heap_end=0x800220 -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -Wl,--relax -Wl,--print-gc-sections
#-Wl,--section-start=.text=0x1E000
CWARN =  -Wall
COPT = -std=gnu99 -Os -fno-tree-scev-cprop -mcall-prologues \
       -fno-inline-small-functions -fsigned-char -fdata-sections -fpack-struct -fshort-enums -ffunction-sections

CFLAGS  = $(CDEFS) $(CLINKER) $(CWARN) $(COPT)

# Include file paths.
INCLUDES = -I./AES_lib

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

aes256_enc.o : AES_lib/aes256_enc.c AES_lib/aes256_enc.h
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib/aes256_enc.c

aes_enc.o : AES_lib/aes_enc.c AES_lib/aes_enc.h
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib/aes_enc.c

aes_keyschedule.o : AES_lib/aes_keyschedule.c AES_lib/aes_keyschedule.h
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib/aes_keyschedule.c

aes_sbox.o : AES_lib/aes_sbox.c AES_lib/aes_sbox.h
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib/aes_sbox.c

keysize_descriptor.o : AES_lib/keysize_descriptor.c AES_lib/keysize_descriptor.h
	$(CC) $(CFLAGS) $(INCLUDES) -c AES_lib/keysize_descriptor.c

bootloader.elf: uart.o AES_lib.o main.o aes256_enc.o aes_enc.o aes_keyschedule.o aes_sbox.o keysize_descriptor.o
	$(CC) $(CFLAGS) $(INCLUDES) -o bootloader.elf uart.o AES_lib.o main.o aes256_enc.o aes_enc.o aes_keyschedule.o aes_sbox.o keysize_descriptor.o

strip: bootloader.elf
	$(STRIP) bootloader.elf -o bootloader.elf

flash.hex: strip
	$(OBJCOPY) -R .eeprom -O ihex bootloader.elf flash.hex

eeprom.hex: strip
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex bootloader.elf eeprom.hex

flash: flash.hex eeprom.hex
	avrdude -P usb -p m1284p -c $(PROGRAMMER)  -u -U flash:w:flash.hex:i \
		-U eeprom:w:eeprom.hex:i \
		-U lfuse:w:lfuse.hex:i \
		-U hfuse:w:hfuse.hex:i \
		-U efuse:w:efuse.hex:i \
		-U lock:w:lock.hex:i 

debug: flash.hex eeprom.hex
	# Launch avarice: a tool that creates a debug server for the AVR and Dragon
	avarice -R -g :4242 &
	# Launch the avr debugger avr-gdb. The configuation for this tool is included 
	# in .gdbinit
	avr-gdb

clean:
	$(RM) -v *.hex *.o *.elf $(MAIN)


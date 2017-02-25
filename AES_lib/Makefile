ARCH=atmega163

CC=avr-gcc -g -mmcu=$(ARCH)
AS=avr-gcc -g -c -mmcu=$(ARCH) -x assembler-with-cpp -lm
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-Wall -I. -O2 -mcall-prologues
ASFLAGS=-Wall -I. -O2 

CSRC= \
	hal.c \
	main.c \
	sw.c \
	t0.c \
	aes_enc.c \
	aes_keyschedule.c \
	aes256_enc.c \
	log.c


SSRC= \
	io.S \
	gf256mul.S \
	aes_sbox-asm.S \

OBJ=$(CSRC:.c=.o) $(SSRC:.S=.o)

SLOCFILES = config.h \
	hal.c hal.h io.S main.c main.h t0.c \
	t0.h aes.c aes.h \
	aes_enc.c aes_enc.h \
	aes_keyschedule.c aes_keyschedule.h aes_sbox.h \
	aes256_enc.c aes256_enc.h \
	types.h iomacros.h\
	usr/lib/avr/include/avr/delay.h\

CALLTREEFILES = sw.c hal.c main.c t0.c \
	aes.c \
	aes_enc.c \
	aes_keyschedule.c \
	aes256_enc.c 


all: dpa4 eedata

dpa4: $(OBJ)
	$(CC) $(CFLAGS) -lc -lm -Wl,-Map,dpa4.map -o dpa4 $(OBJ) -lm
	$(SIZE) dpa4
	avr-gcc -g -mmcu=atmega163 -o dpa4.elf hal.o main.o sw.o t0.o aes_enc.o aes_keyschedule.o aes256_enc.o log.o io.o gf256mul.o aes_sbox-asm.o 
	#avr-gcc -fverbose-asm -mmcu=atmega163 -o aes_enc.S -S aes_enc.c 
	$(OBJCOPY) -O binary dpa4 dpa4.bin
	$(OBJCOPY) -O ihex dpa4 dpa4.hex

eedata:
	$(CC) $(ASFLAGS) -c -o eedata -DDAY=0x`date +%d` -DMONTH=0x`date +%m` -DYEAR=0x`date +%y` eedata.S
	$(OBJCOPY) -O binary eedata eedata.bin
	$(OBJCOPY) -O ihex eedata eedata.hex

sloc:
	@echo
	@echo Lines of code
	@sloccount $(SLOCFILES) | grep -A 1000 "SLOC-by-Language" | grep -v "^$$"

calltree:
	@echo
	@echo Calltree
	@/opt/schily/bin/calltree -m -I. -I/opt/avrgcc/avr/include/ -I/usr/lib/avr/include/ -D__AVR_ATmega163__ $(CALLTREEFILES)

calldepth:
	@echo
	@echo Calldepth
	@/opt/schily/bin/calltree -m -I. -I/opt/avrgcc/avr/include/ -I/usr/lib/avr/include/ -D__AVR_ATmega163__ $(CALLTREEFILES) | sed -e "s/[^ ].*$$//" -e "s/    /#/g" -e "s/#/1/" -e "s/#/2/" -e "s/#/3/" -e "s/#/4/" -e "s/#/5/" -e "s/#/6/" -e "s/#/7/" -e "s/#/8/" -e "s/#/9/" -e "s/#/0/" | sort | tail -1

uncalled:
	@echo
	@echo Uncalled
	@/opt/schily/bin/calltree -u -I. -I/opt/avrgcc/avr/include/ -I/usr/lib/avr/include/ -D__AVR_ATmega163__ $(CALLTREEFILES)

statistics: sloc calldepth uncalled

clean: clean-dpa4 clean-eedata
	rm -rf core

clean-dpa4:
	rm -rf $(OBJ) dpa4 dpa4.bin dpa4.hex dpa4.map

clean-eedata:
	rm -rf eedata eedata.bin eedata.hex eedata.lst


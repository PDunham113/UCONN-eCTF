################################################################################
# Automatically-generated file. Do not edit!
################################################################################

RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
AES_lib


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
AES_lib.c \
AES_lib/aes256_enc.c \
AES_lib/aes_enc.c \
AES_lib/aes_keyschedule.c \
AES_lib/aes_sbox.c \
AES_lib/keysize_descriptor.c \
main.c \
uart.c \
eeprom_safe.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
AES_lib.o \
AES_lib/aes256_enc.o \
AES_lib/aes_enc.o \
AES_lib/aes_keyschedule.o \
AES_lib/aes_sbox.o \
AES_lib/keysize_descriptor.o \
main.o \
uart.o \
eeprom_safe.o

OBJS_AS_ARGS +=  \
AES_lib.o \
AES_lib/aes256_enc.o \
AES_lib/aes_enc.o \
AES_lib/aes_keyschedule.o \
AES_lib/aes_sbox.o \
AES_lib/keysize_descriptor.o \
main.o \
uart.o \
eeprom_safe.o

C_DEPS +=  \
AES_lib.d \
AES_lib/aes256_enc.d \
AES_lib/aes_enc.d \
AES_lib/aes_keyschedule.d \
AES_lib/aes_sbox.d \
AES_lib/keysize_descriptor.d \
main.d \
uart.d \
eeprom_safe.d

C_DEPS_AS_ARGS +=  \
AES_lib.d \
AES_lib/aes256_enc.d \
AES_lib/aes_enc.d \
AES_lib/aes_keyschedule.d \
AES_lib/aes_sbox.d \
AES_lib/keysize_descriptor.d \
main.d \
uart.d \
eeprom_safe.d

OUTPUT_FILE_PATH +=ATMega1284P_Boot.elf

OUTPUT_FILE_PATH_AS_ARGS +=ATMega1284P_Boot.elf

ADDITIONAL_DEPENDENCIES:=

#OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+= 

INCLUDES:= -I/usr/lib/avr/include/

# AVR32/GNU C Compiler


./%.o: ./%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : 4.9.2
	avr-gcc -MD $(INCLUDES) -x c -ffunction-sections -funsigned-char -funsigned-bitfields -Os -fno-inline-small-functions -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega1284p -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

AES_lib/%.o: AES_lib/%.c
	@echo Buildimang file: $<
	@echo Invoking: AVR/GNU C Compiler : 4.9.2
	avr-gcc -MD $(INCLUDES) -x c -ffunction-sections -funsigned-char -funsigned-bitfields -Os -fno-inline-small-functions -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega1284p -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : 4.9.2
	avr-gcc $(INCLUDES) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="ATMega1284P_Boot.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mrelax -Wl,-section-start=.text=0x1e000  -mmcu=atmega1284p  
	@echo Finished building target: $@
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "ATMega1284P_Boot.elf" "flash.hex"
	avr-objcopy -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "ATMega1284P_Boot.elf" "eeprom.hex" || exit 0
	avr-objdump -h -S "ATMega1284P_Boot.elf" > "ATMega1284P_Boot.lss"
	avr-objcopy -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "ATMega1284P_Boot.elf" "ATMega1284P_Boot.srec"
	avr-size "ATMega1284P_Boot.elf"
	

flash: flash.hex eeprom.hex
	avrdude -P usb -p m1284p -c dragon_jtag  -u -U flash:w:flash.hex:i \
		-U eeprom:w:eeprom.hex:i \
		-U lfuse:w:lfuse.hex:i \
		-U hfuse:w:hfuse.hex:i \
		-U efuse:w:efuse.hex:i \
		-U lock:w:lock.hex:i 


# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "ATMega1284P_Boot.elf" "ATMega1284P_Boot.a" "ATMega1284P_Boot.hex" "ATMega1284P_Boot.lss" "ATMega1284P_Boot.eep" "ATMega1284P_Boot.map" "ATMega1284P_Boot.srec" "ATMega1284P_Boot.usersignatures"
	
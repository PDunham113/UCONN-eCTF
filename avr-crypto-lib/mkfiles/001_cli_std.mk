CLI_STD =  cli-stub.o cli-basics.o cli-core.o cli-hexdump.o debug.o hexdigit_tab.o \
           dbz_strings.o string-extras-asm.o  $(UART_OBJ) \
           main-test-common.o

UART_I_OBJ =  uart_i-asm.o circularbytebuffer-asm.o
UART_NI_OBJ = uart_ni-asm.o

ifeq ($(UART),NI)
UART_OBJ = $(UART_NI_OBJ)
DEFS += -DUART_NI=1
else
UART_OBJ = $(UART_I_OBJ)
DEFS += -DUART_NI=0
endif


#CLI_STD = cli.o debug.o hexdigit_tab.o \
#          dbz_strings.o string-extras-asm.o uart_i-asm.o circularbytebuffer-asm.o \
#          main-test-common.o


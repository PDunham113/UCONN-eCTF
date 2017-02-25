# Makefile for norx
ALGO_NAME := NORX_C

# comment out the following line for removement of noekeon from the build process
AEAD_CIPHERS += $(ALGO_NAME)

$(ALGO_NAME)_DIR      := norx/
$(ALGO_NAME)_INCDIR   := memxor/
$(ALGO_NAME)_OBJ      := norx32.o memxor.o
$(ALGO_NAME)_TESTBIN  := main-norx-test.o $(CLI_STD)
$(ALGO_NAME)_NESSIE_TEST      := test nessie
$(ALGO_NAME)_PERFORMANCE_TEST := performance


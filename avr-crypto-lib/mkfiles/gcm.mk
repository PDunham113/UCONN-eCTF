# Makefile for GCM
ALGO_NAME := GCM

# comment out the following line for removement of AES from the build process
AEAD_CIPHERS += $(ALGO_NAME)

$(ALGO_NAME)_DIR      := gcm/
$(ALGO_NAME)_INCDIR   := memxor/ aes/ gf256mul/ bcal/
$(ALGO_NAME)_OBJ      := gcm128.o \
                         aes_enc-asm.o aes_sbox-asm.o \
                         aes_keyschedule-asm.o memxor.o
$(ALGO_NAME)_TESTBIN  := main-gcm-test.o $(CLI_STD) $(BCAL_STD)  \
                         bcal_aes128_enconly.o bcal_aes192_enconly.o bcal_aes256_enconly.o  \
                         dump-asm.o dump-decl.o
$(ALGO_NAME)_NESSIE_TEST      := test nessie
$(ALGO_NAME)_PERFORMANCE_TEST := performance


###############################################################################
# Makefile for sgx_version/
#
# Requires Intel SGX SDK installed at $(SGX_SDK) (default: /opt/intel/sgxsdk).
# Build modes:
#   make           — hardware mode, debug build
#   make SGX_MODE=SIM — simulation mode (no SGX hardware needed for SW test)
#   make SGX_DEBUG=0   — release build
###############################################################################

SGX_SDK    ?= /opt/intel/sgxsdk
SGX_MODE   ?= HW           # HW or SIM
SGX_DEBUG  ?= 1            # 1 = debug, 0 = release
SGX_ARCH   ?= x86_64

# ---- Paths ----------------------------------------------------------------
BISEARCH_ROOT := ..
INC_DIR       := $(BISEARCH_ROOT)/include
ORIG_SRC      := $(BISEARCH_ROOT)/src

# lz4 and xdelta3 sources (reuse from original project util dir)
LZ4_SRC    := $(ORIG_SRC)/util/lz4.c
XDELTA_SRC := $(INC_DIR)/xdelta3.h   # header-only usage; actual xd3 .c below
XDELTA_C   := $(wildcard $(BISEARCH_ROOT)/lib/xdelta3/*.c) \
              $(wildcard /usr/local/lib/xdelta3/*.c)

# ---- SGX SDK flags --------------------------------------------------------
include $(SGX_SDK)/buildenv.mk

ifeq ($(SGX_MODE), HW)
    Urts_Library_Name := sgx_urts
else
    Urts_Library_Name := sgx_urts_sim
endif

ifeq ($(SGX_DEBUG), 1)
    SGX_COMMON_CFLAGS := $(SGX_COMMON_CFLAGS) -O0 -g
    SGX_DEBUG_FLAG    := -DSGX_DEBUG_FLAG=1
else
    SGX_COMMON_CFLAGS := $(SGX_COMMON_CFLAGS) -O2
    SGX_DEBUG_FLAG    := -DSGX_DEBUG_FLAG=0
endif

# ---- Compiler / Flags -----------------------------------------------------
CC  := gcc
CXX := g++

App_C_Flags   := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes \
                 -I$(SGX_SDK)/include \
                 -I./include \
                 $(SGX_DEBUG_FLAG)

App_Cpp_Flags := $(App_C_Flags) -std=c++17

App_Link_Flags := -L$(SGX_SDK)/lib64 \
                  -l$(Urts_Library_Name) \
                  -lpthread \
                  -lssl -lcrypto     # OpenSSL for client-side AES-GCM

# ---- Enclave flags --------------------------------------------------------
Enclave_C_Flags := $(ENCLAVE_CFLAGS) \
                   -nostdinc \
                   -fvisibility=hidden \
                   -fpie -fstack-protector \
                   -I$(SGX_SDK)/include \
                   -I$(SGX_SDK)/include/tlibc \
                   -I$(SGX_SDK)/include/libcxx \
                   -I./include \
                   -I./Enclave \
                   -std=c11

Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++17 -nostdinc++

Enclave_Link_Flags := $(ENCLAVE_LDFLAGS) \
                      -Wl,--no-undefined -nostdlib \
                      -Wl,-Bstatic -Wl,-Bsymbolic \
                      -Wl,--export-dynamic \
                      -Wl,-pie -Wl,--start-group \
                          -lsgx_tstdc -lsgx_tcxx \
                          -lsgx_tcrypto -lsgx_tservice \
                      -Wl,--end-group \
                      -L$(SGX_SDK)/lib64

# ---- EDL code generation --------------------------------------------------
Enclave_EDL := Enclave/Enclave.edl
Enclave_u_h := Enclave_u.h
Enclave_u_c := Enclave_u.c
Enclave_t_h := Enclave/Enclave_t.h
Enclave_t_c := Enclave/Enclave_t.c

$(Enclave_u_c) $(Enclave_u_h): $(Enclave_EDL)
	@echo "[EDL] Generating untrusted edge routines..."
	$(SGX_SDK)/bin/x64/sgx_edger8r --untrusted $(Enclave_EDL) \
	    --search-path $(SGX_SDK)/include \
	    --search-path ./include

$(Enclave_t_c) $(Enclave_t_h): $(Enclave_EDL)
	@echo "[EDL] Generating trusted edge routines..."
	$(SGX_SDK)/bin/x64/sgx_edger8r --trusted $(Enclave_EDL) \
	    --search-path $(SGX_SDK)/include \
	    --search-path ./include \
	    --trusted-dir Enclave/

# ---- App objects ----------------------------------------------------------
App_Cpp_Files := App/main.cc \
                 App/App.cpp \
                 App/client_chunker.cc

App_C_Files   := $(Enclave_u_c)

App_Objs := $(App_Cpp_Files:.cc=.o) $(App_C_Files:.c=.o)

%.o: %.cc $(Enclave_u_h)
	$(CXX) $(App_Cpp_Flags) -c $< -o $@

$(Enclave_u_c:.c=.o): $(Enclave_u_c)
	$(CC) $(App_C_Flags) -c $< -o $@

# ---- Enclave objects ------------------------------------------------------
Enclave_Cpp_Files := Enclave/Enclave.cpp \
                     Enclave/enclave_bisearch.cc \
                     Enclave/enclave_datawrite.cc

Enclave_C_Files   := $(Enclave_t_c) \
                     $(LZ4_SRC)

Enclave_Objs := $(Enclave_Cpp_Files:.cc=.o) $(Enclave_C_Files:.c=.o)

Enclave/%.o: Enclave/%.cc $(Enclave_t_h)
	$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@

Enclave/$(Enclave_t_c:.c=.o): $(Enclave_t_c)
	$(CC) $(Enclave_C_Flags) -c $< -o $@

# lz4 as trusted C file
$(ORIG_SRC)/util/lz4.o: $(LZ4_SRC)
	$(CC) $(Enclave_C_Flags) -c $< -o $@

# ---- Targets --------------------------------------------------------------
.PHONY: all clean

APP_NAME     := bisearch_sgx
ENCLAVE_NAME := bisearch_enclave.so
SIGNED_NAME  := bisearch_enclave.signed.so

all: $(APP_NAME) $(SIGNED_NAME)

# Link App
$(APP_NAME): $(App_Objs)
	@echo "[LINK] $@"
	$(CXX) $^ -o $@ $(App_Link_Flags)

# Link Enclave .so (unsigned)
$(ENCLAVE_NAME): $(Enclave_Objs)
	@echo "[LINK] $@"
	$(CXX) $^ -o $@ $(Enclave_Link_Flags)

# Sign Enclave
$(SIGNED_NAME): $(ENCLAVE_NAME) Enclave/Enclave.config.xml
	@echo "[SIGN] $@"
	$(SGX_SDK)/bin/x64/sgx_sign sign \
	    -key Enclave/Enclave_private.pem \
	    -enclave $< \
	    -out $@ \
	    -config Enclave/Enclave.config.xml

# Generate a test signing key (run once on the server)
gen_key:
	openssl genrsa -3 -out Enclave/Enclave_private.pem 3072

clean:
	rm -f $(App_Objs) $(Enclave_Objs) \
	      $(Enclave_u_h) $(Enclave_u_c) \
	      $(Enclave_t_h) $(Enclave_t_c) \
	      $(APP_NAME) $(ENCLAVE_NAME) $(SIGNED_NAME)
	find . -name "*.o" -delete

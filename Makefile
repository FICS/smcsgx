######## SGX SDK Settings ########

# Support for evaluator
# Apr 6, 2016
# daveti

# Support for sender
# Apr 13, 2016
# daveti

# Support for Yao
# Apr 27, 2016
# daveti

# Design change for the hybrid model
# SGX_EVL=YAO_GEN=Bob
# SGX_GEN=YAO_EVL=Alice
# Jul 27, 2016
# daveti

# Roll back to the default input file handling
# for hybrid dijkstra
# SGX_Sender->map
# SGX_Evaluator->path
# Jul 28, 2016
# daveti

SGX_SDK ?= /opt/intel/sgxsdk
# daveti: get ready for the HW
SGX_MODE ?= HW
#SGX_PRERELEASE ?= 1
# daveti: oops, to run RA, we need debug mode to get rid of the Intel signing
SGX_DEBUG ?= 1
#SGX_MODE ?= SIM
SGX_ARCH ?= x64

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g -ggdb -Wall -Wno-unused-variable -DDISABLE_NAGLE
else
        SGX_COMMON_CFLAGS += -O2 -Wall -Wno-unused-variable
endif

ifdef SFE_PARAMS
        SGX_COMMON_CFLAGS += $(SFE_PARAMS)
        $(info "Using parameterized build flags '$(SFE_PARAMS)'")
endif

######## Eval App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

COMMON_FILES := common/smcsgx.cpp common/mysgx_common.cpp common/timing.cpp
YAO_LIBS := -lpbc -lgmp -lcrypto -lyao

Eval_App_Cpp_Files := eval_app/eval_app.cpp eval_app/trans.cpp eval_app/mysgx.cpp eval_app/YaoBase.cpp eval_app/BetterYao.cpp eval_app/myyao.cpp $(COMMON_FILES) 
Eval_App_Include_Paths := -Iinclude -I$(SGX_SDK)/include
Eval_App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(Eval_App_Include_Paths)

Send_App_Cpp_Files := send_app/send_app.cpp send_app/trans.cpp send_app/mysgx.cpp send_app/ecp.cpp send_app/YaoBase.cpp send_app/BetterYao.cpp send_app/myyao.cpp send_app/utils.cpp $(COMMON_FILES)
Send_App_Include_Paths := -Iinclude -Isample_libcrypto -I$(SGX_SDK)/include
Send_App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(Send_App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        Eval_App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG #-fsanitize=address
	Send_App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG #-fsanitize=address
else ifeq ($(SGX_PRERELEASE), 1)
        Eval_App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
	Send_App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        Eval_App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
	Send_App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

Eval_App_Cpp_Flags := $(Eval_App_C_Flags) -std=c++11 -DGEN_CODE -DBEN_IS_NICE_TO_DAVE
Eval_App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L. -lsgx_ukey_exchange -lpthread -Wl,-rpath=$(CURDIR) $(YAO_LIBS)

Send_App_Cpp_Flags := $(Send_App_C_Flags) -std=c++11 -DEVL_CODE -DBEN_IS_NICE_TO_DAVE
Send_App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L. -lsgx_ukey_exchange -lpthread -Wl,-rpath=$(CURDIR)/sample_libcrypto -Wl,-rpath=$(CURDIR) -lsample_libcrypto -Lsample_libcrypto $(YAO_LIBS) -lssl

# Enable ASAN for error run-time checking
ifeq ($(SGX_DEBUG), 1)
Eval_App_Link_Flags += -lasan
Send_App_Link_Flags += -lasan
endif

ifneq ($(SGX_MODE), HW)
	Eval_App_Link_Flags += -lsgx_uae_service_sim
	Send_App_Link_Flags += -lsgx_uae_service_sim
else
	Eval_App_Link_Flags += -lsgx_uae_service
	Send_App_Link_Flags += -lsgx_uae_service
endif

Eval_App_Cpp_Objects := $(Eval_App_Cpp_Files:.cpp=.o)
Eval_App_Name := evaluator

Send_App_Cpp_Objects := $(Send_App_Cpp_Files:.cpp=.o)
Send_App_Name := sender 

######## Enclave Settings ########

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif
Crypto_Library_Name := sgx_tcrypto

Eval_Enclave_Cpp_Files := eval_enclave/eval_enclave.cpp eval_enclave/mysfe.cpp eval_enclave/util.cpp eval_enclave/mysfe_naive.cpp eval_enclave/hybrid.cpp
Eval_Enclave_Include_Paths := -Iinclude -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport

Eval_Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Eval_Enclave_Include_Paths)
Eval_Enclave_Cpp_Flags := $(Eval_Enclave_C_Flags) -std=c++03 -nostdinc++ -DBEN_IS_NICE_TO_DAVE
Eval_Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -lsgx_tkey_exchange -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=eval_enclave/eval_enclave.lds 

Eval_Enclave_Cpp_Objects := $(Eval_Enclave_Cpp_Files:.cpp=.o)

Eval_Enclave_Name := eval_enclave.so
Eval_Enclave_Hash_Name := eval_enclave_hash.hex
Eval_Enclave_Signature_Name := eval_enclave_signature.hex
Eval_Signed_Enclave_Name := eval_enclave.signed.so
Eval_Enclave_Config_File := eval_enclave/eval_enclave.config.xml


Send_Enclave_Cpp_Files := send_enclave/send_enclave.cpp
Send_Enclave_Include_Paths := -Iinclude -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport

Send_Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Send_Enclave_Include_Paths)
Send_Enclave_Cpp_Flags := $(Send_Enclave_C_Flags) -std=c++03 -nostdinc++ -DBEN_IS_NICE_TO_DAVE
Send_Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
        -Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
        -Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -lsgx_tkey_exchange -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
        -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
        -Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
        -Wl,--defsym,__ImageBase=0 \
        -Wl,--version-script=send_enclave/send_enclave.lds

Send_Enclave_Cpp_Objects := $(Send_Enclave_Cpp_Files:.cpp=.o)

Send_Enclave_Name := send_enclave.so
Send_Signed_Enclave_Name := send_enclave.signed.so
Send_Enclave_Config_File := send_enclave/send_enclave.config.xml


################# YAO settings #######################

YAO_Cpp_Objects := yao/Bytes.o yao/Aes.o yao/key_expansion.o yao/interpreter.o yao/Prng.o yao/Circuit.o yao/Algebra.o yao/NetIO.o yao/Env.o yao/GarbledCct.o
YAO_Include_Paths := -Iyao
YAO_C_Flags := -fPIC -Wno-attributes
YAO_Cpp_Flags := $(YAO_C_Flags) -I$(HOME)/local/include -L$(HOME)/local/lib -Wno-deprecated -D__STDC_LIMIT_MACROS -DFREE_XOR -DRAND_SEED -DGRR -DNDEBUG -std=c++0x -g #-fsanitize=address
YAO_Link_Flags :=  -shared -lpbc -lgmp -lcrypto
YAO_Lib_Name := libyao.so



ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: $(YAO_Lib_Name)
	@echo "YAO is required for both parties in the hybrid model"
all: $(Eval_App_Name) $(Eval_Enclave_Name)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Eval_Enclave_Name) first with your signing key before you run the $(Eval_App_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Eval_Enclave_Name) -out <$(Eval_Signed_Enclave_Name)> -config $(Eval_Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool. See User's Guide for more details."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
all: $(Send_App_Name) $(Send_Enclave_Name)
        @echo "The project has been built in release hardware mode."
        @echo "Please sign the $(Send_Enclave_Name) first with your signing key before you run the $(Send_App_Name) to launch and access the enclave."
        @echo "To sign the enclave use the command:"
        @echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Send_Enclave_Name) -out <$(Send_Signed_Enclave_Name)> -config $(Send_Enclave_Config_File)"
        @echo "You can also sign the enclave using an external signing tool. See User's Guide for more details."
        @echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
all: $(YAO_Lib_Name)
all: $(Eval_App_Name) $(Eval_Signed_Enclave_Name)
all: $(Send_App_Name) $(Send_Signed_Enclave_Name)
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(Eval_App_Name) 	
	@echo "RUN  =>  $(Eval_App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
	@$(CURDIR)/$(Send_App_Name)
	@echo "RUN  =>  $(Send_App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

######## App Objects ########

eval_app/eval_enclave_u.c: $(SGX_EDGER8R) eval_enclave/eval_enclave.edl
	@cd eval_app && $(SGX_EDGER8R) --untrusted ../eval_enclave/eval_enclave.edl --search-path ../eval_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

eval_app/eval_enclave_u.o: eval_app/eval_enclave_u.c
	@$(CC) $(Eval_App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

eval_app/%.o: eval_app/%.cpp
	@$(CXX) $(Eval_App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Eval_App_Name): eval_app/eval_enclave_u.o $(Eval_App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Eval_App_Link_Flags)
	@echo "LINK =>  $@"


send_app/send_enclave_u.c: $(SGX_EDGER8R) send_enclave/send_enclave.edl
	@cd send_app && $(SGX_EDGER8R) --untrusted ../send_enclave/send_enclave.edl --search-path ../send_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

send_app/send_enclave_u.o: send_app/send_enclave_u.c
	@$(CC) $(Send_App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

send_app/%.o: send_app/%.cpp
	@$(CXX) $(Send_App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Send_App_Name): send_app/send_enclave_u.o $(Send_App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Send_App_Link_Flags)
	@echo "LINK =>  $@"

common/%.o: common/%.cpp
	@$(CXX) $(Send_App_Cpp_Flags) -c $< -o $@
	@echo "CXX COM  <=  $<"

######## Enclave Objects ########

eval_enclave/eval_enclave_t.c: $(SGX_EDGER8R) eval_enclave/eval_enclave.edl
	@cd eval_enclave && $(SGX_EDGER8R) --trusted ../eval_enclave/eval_enclave.edl --search-path ../eval_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

eval_enclave/eval_enclave_t.o: eval_enclave/eval_enclave_t.c
	@$(CC) $(Eval_Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

eval_enclave/%.o: eval_enclave/%.cpp
	@$(CXX) $(Eval_Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Eval_Enclave_Name): eval_enclave/eval_enclave_t.o $(Eval_Enclave_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Eval_Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Eval_Signed_Enclave_Name): $(Eval_Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key eval_enclave/eval_enclave_private.pem -enclave $(Eval_Enclave_Name) -out $@ -config $(Eval_Enclave_Config_File)
	@echo "SIGN =>  $@"
	@$(SGX_ENCLAVE_SIGNER) gendata -enclave $(Eval_Enclave_Name) -config $(Eval_Enclave_Config_File) -out $(Eval_Enclave_Hash_Name)
	@echo "GEN HASH =>  $(Eval_Enclave_Hash_Name)"
	@openssl dgst -sha256 -out $(Eval_Enclave_Signature_Name) -sign eval_enclave/eval_enclave_private.pem -keyform PEM $(Eval_Enclave_Hash_Name)
	@echo "GEN SIGNATURE => $(Eval_Enclave_Signature_Name)"


send_enclave/send_enclave_t.c: $(SGX_EDGER8R) send_enclave/send_enclave.edl
	@cd send_enclave && $(SGX_EDGER8R) --trusted ../send_enclave/send_enclave.edl --search-path ../send_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

send_enclave/send_enclave_t.o: send_enclave/send_enclave_t.c
	@$(CC) $(Send_Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

send_enclave/%.o: send_enclave/%.cpp
	@$(CXX) $(Send_Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Send_Enclave_Name): send_enclave/send_enclave_t.o $(Send_Enclave_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Send_Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Send_Signed_Enclave_Name): $(Send_Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key send_enclave/send_enclave_private.pem -enclave $(Send_Enclave_Name) -out $@ -config $(Send_Enclave_Config_File)
	@echo "SIGN =>  $@"


######## YAO Objects ########

yao/GarbledCct.o : yao/GarbledCct.cpp
	@$(CXX) -msse2 $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Env.o : yao/Env.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/NetIO.o : yao/NetIO.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Algebra.o: yao/Algebra.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Circuit.o : yao/Circuit.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Prng.o: yao/Prng.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/interpreter.o: yao/interpreter.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Aes.o: yao/Aes.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

#Aes.o: Bytes.h Aes.cpp
#	$(CXX) -O3 -c -maes -msse4.2 Aes.cpp -DAESNI

yao/key_expansion.o: yao/key_expansion.s
	@$(CXX) -O3 $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

yao/Bytes.o : yao/Bytes.cpp
	@$(CXX) $(YAO_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"
	
$(YAO_Lib_Name): $(YAO_Cpp_Objects)
	@$(CXX) $^ -o $@ $(YAO_Link_Flags)
	@echo "LINK =>  $@"



.PHONY: clean clean-sgx deploy

# Deploy script for making static builds of run configurations
# Clean all SGX code before rebuild to pick up changed header files
deploy:
	$(MAKE) clean-sgx
	$(MAKE) all
	./deploy.sh
	
clean:
	@rm -f $(Eval_App_Name) $(Eval_Enclave_Name) $(Eval_Signed_Enclave_Name) $(Eval_App_Cpp_Objects) eval_app/eval_enclave_u.* $(Eval_Enclave_Cpp_Objects) eval_enclave/eval_enclave_t.*
	@rm -f $(Send_App_Name) $(Send_Enclave_Name) $(Send_Signed_Enclave_Name) $(Send_App_Cpp_Objects) send_app/send_enclave_u.* $(Send_Enclave_Cpp_Objects) send_enclave/send_enclave_t.*
	@rm -f $(YAO_Lib_Name) $(YAO_Cpp_Objects)
	@rm -f *.hex

# same as clean, but only cleans SGX related files
clean-sgx:
	@rm -f $(Eval_App_Name) $(Eval_Enclave_Name) $(Eval_Signed_Enclave_Name) $(Eval_App_Cpp_Objects) eval_app/eval_enclave_u.* $(Eval_Enclave_Cpp_Objects) eval_enclave/eval_enclave_t.*
	@rm -f $(Send_App_Name) $(Send_Enclave_Name) $(Send_Signed_Enclave_Name) $(Send_App_Cpp_Objects) send_app/send_enclave_u.* $(Send_Enclave_Cpp_Objects) send_enclave/send_enclave_t.*
	@rm -f *.hex

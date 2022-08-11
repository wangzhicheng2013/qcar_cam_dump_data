ARTIFACT = qnx_arcam_test

#Build architecture/variant string, possible values: x86, armv7le, etc...
PLATFORM ?= aarch64le

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Compiler definitions

CC = qcc -Vgcc_nto$(PLATFORM)
CXX = qcc -lang-c++ -Vgcc_nto$(PLATFORM)
LD = $(CXX)

#select QCarCamVersion
#QCARCAM_VERSION := 462
QCARCAM_VERSION := 477

#User defined include/preprocessor flags and libraries

#INCLUDES += -I/path/to/my/lib/include
INCLUDES += -I./inc

#LIBS += -L/path/to/my/lib/$(PLATFORM)/usr/lib -lmylib
#LIBS += -L../mylib/$(OUTPUT_DIR) -lmylib
#LIBS += -L../ChangAn_Sdk/lib/hvx -lcdsprpc -lfastrpc_pmem -llibstd -lsmmu_client
LIBS += -L./lib/runtime -lcdsprpc -lfastrpc_pmem -lsmmu_client -pthread

ifeq ($(QCARCAM_VERSION), 462)
#echo "build QCARCAM_VERSION_4_6_2"
INCLUDES += -I./inc/qcarcam_462
LIBS += -L./lib/qcarcam_462 -lais_client -lsocket -lpmemext -lOSAbstraction -lmmap_peer -llibstd -lslog2 -lpmem_client 
LIBS += -lscreen -lfreetype
CCFLAGS_all += -DQCARCAM_VERSION_4_6_2
else
ifeq ($(QCARCAM_VERSION), 477)
#echo "build QCARCAM_VERSION_4_7_7"
INCLUDES += -I./inc/qcarcam_477
LIBS += -L./lib/qcarcam_477 -lais_client -lsocket -lpmemext -lOSAbstraction -lmmap_peer -llibstd -lslog2 -lpmem_client
LIBS += -lscreen -lfreetype
CCFLAGS_all += -DQCARCAM_VERSION_4_7_7
endif
endif

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -finput-charset=UTF-8 -fexec-charset=GBK -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS = $(call rwildcard, src, c cpp)

#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Compiling rule
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<
$(OUTPUT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

#Linking rule
$(TARGET):$(OBJS)
	$(LD) -o $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

#Rules section for default compilation and linking
all: $(TARGET)

clean:
	rm -fr $(OUTPUT_DIR)

rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)
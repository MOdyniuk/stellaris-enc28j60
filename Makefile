# Configurable variables

# Compiler setup
CC_PREFIX=arm-none-eabi-
CXX=$(CC_PREFIX)g++
CC=$(CC_PREFIX)gcc
LD=$(CC_PREFIX)ld
OBJCOPY=$(CC_PREFIX)objcopy
SIZE=$(CC_PREFIX)size

# Build directory
BUILD_DIR=build/

STELLARIS=$(HOME)/opt/stellaris-launchpad
UIP_DIR=uip-1.0

# Flash setup
LM4FLASH=$(STELLARIS)/lm4flash

# Base name of the binary targets (.elf, .bin)
TARGET_NAME=enc28j60

# Target CPU configuration
CPU=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
LINKER_SCRIPT=LM4F.ld
# Used to give some useful statistics after linking
FLASH_SIZE=262144
RAM_SIZE=32768

# Common C/C++ compiler flags
COMMON_FLAGS=$(CPU) -ffunction-sections -fdata-sections -Os 

# C/C++ pre-processor defines and include paths
DEFS=
DEFINES=$(DEFS) -DTARGET_IS_BLIZZARD_RA2 -DPART_LM4F120H5QR -DUART_BUFFERED
INCLUDES=-I$(STELLARIS) -Iuip-1.0 -I. -I uip-1.0/uip

# C++ compiler specific flags
CXXFLAGS=-fno-rtti -fno-exceptions 

# C compiler specific flags
CFLAGS=--std=gnu99

# Linker flags
LDFLAGS=$(CPU) -nodefaultlibs -T$(LINKER_SCRIPT) -Wl,--gc-sections

# Libraries to link against
LIBS=-lm -lc

UIP_SRCS=$(UIP_DIR)/uip/uip.c \
	 $(UIP_DIR)/uip/uip_timer.c \
	 $(UIP_DIR)/uip/psock.c \
	 $(UIP_DIR)/uip/uip_arp.c \
	 $(UIP_DIR)/apps/dhcpc/dhcpc.c

# C++ source code files
CXX_SRCS=main.cpp enc28j60.cpp enc28j60_stellaris.cpp

# C source code
C_SRCS=dummyfuncs.c \
	startup_gcc.c \
	$(STELLARIS)/utils/uartstdio.c \
	$(UIP_SRCS) \
	httpd.c

# End of configuriable variables

SHELL=bash
SRCS=$(CXX_SRCS) $(C_SRCS)
OBJS=$(addprefix $(BUILD_DIR), $(notdir $(CXX_SRCS:.cpp=.o)) $(C_SRCS:.c=.o))
DEPS=$(OBJS:.o=.d)

TARGET_ELF=$(BUILD_DIR)$(TARGET_NAME).elf
TARGET_BIN=$(BUILD_DIR)$(TARGET_NAME).bin

GCC_DEP_GEN=-MD -MP -MF $(BUILD_DIR)$(@F:.o=.d)

_CXXFLAGS=$(COMMON_FLAGS) $(CXXFLAGS) $(GCC_DEP_GEN) $(DEFINES) $(INCLUDES)
_CFLAGS=$(COMMON_FLAGS) $(CFLAGS) $(GCC_DEP_GEN) $(DEFINES) $(INCLUDES)

BUILD_DIRS=$(sort $(dir $(OBJS)))

define print-memory-usage
FLASH_USAGE=$$($(SIZE) $@ | tail -1 | cut -f 1 | tr -d ' '); \
DATA_USAGE=$$($(SIZE) $@ | tail -1 | cut -f 2 | tr -d ' '); \
BSS_USAGE=$$($(SIZE) $@ | tail -1 | cut -f 3 | tr -d ' '); \
let "FLASH_P=$$FLASH_USAGE*100 / $(FLASH_SIZE)"; \
let "RAM_USAGE=$$DATA_USAGE+$$BSS_USAGE"; \
let "RAM_P=$$RAM_USAGE*100 / $(RAM_SIZE)"; \
printf "Flash usage     : %6dB / %6dB = %2d%%\n" "$$FLASH_USAGE" "$(FLASH_SIZE)" "$$FLASH_P"; \
printf "Static RAM usage: %6dB / %6dB = %2d%%\n" "$$RAM_USAGE" "$(RAM_SIZE)" "$$RAM_P"
endef

.PHONY: print-config print-config-verbose $(BUILD_DIR) doxygen

all: print-config $(BUILD_DIR) $(TARGET_BIN)

doxygen: $(SRCS)
	@doxygen	

load: $(TARGET_BIN)
	@echo "Flashing $(TARGET_BIN) to target"
	$(LM4FLASH) $(TARGET_BIN)

print-config:
	@echo "SETTINGS:"
	@echo "---------"
	@echo "Compiler prefix: $(CC_PREFIX)"
	@echo "Commong flags  : $(COMMON_FLAGS)"
	@echo "Defines        : $(DEFINES)"
	@echo "C   flags      : $(CFLAGS)"
	@echo "C++ flags      : $(CXXFLAGS)"
	@echo

print-config-verbose: print-config
	@echo $(OBJS)
	@echo $(BUILD_DIRS)

$(OBJS): Makefile

$(BUILD_DIR): 
	@mkdir -p $(BUILD_DIRS)

$(TARGET_ELF): $(OBJS) $(LINKER_SCRIPT)
	@echo "Linking $@"
	$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
	@echo 
	@$(print-memory-usage)
	@echo

$(TARGET_BIN): $(TARGET_ELF)
	@echo "$(TARGET_ELF) -> $(TARGET_BIN)"
	@$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)

clean:
	@echo "Removing '$(BUILD_DIR)'"
	@rm -rf $(BUILD_DIR)

$(BUILD_DIR)%.o: %.cpp
	@echo "C++ compiling '$<'"
	@echo "        -> $@"
	@$(CXX) -c $(_CXXFLAGS) $< -o $@
	@echo

$(BUILD_DIR)%.o: %.c
	@echo "C   compiling '$<'"
	@echo "        -> $@"
	@$(CC) -c $(_CFLAGS) $< -o $@ 
	@echo

-include $(DEPS)

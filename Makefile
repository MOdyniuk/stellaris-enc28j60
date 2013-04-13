
CXX=arm-none-eabi-g++
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CPU=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp

CFLAGS=$(CPU) -ffunction-sections -fdata-sections

STELLARIS=$(HOME)/opt/stellaris-launchpad
UIP_DIR=uip-1.0

INCLUDES=-I$(STELLARIS) -Iuip-1.0 -I. -I uip-1.0/uip
DEFINES=-DTARGET_IS_BLIZZARD_RA2 -DPART_LM4F120H5QR -DUART_BUFFERED 
CXXFLAGS=$(CFLAGS) -fno-rtti -fno-exceptions 
BUILD_DIR=build/
LDFLAGS=-nodefaultlibs -TLM4F.ld 
LIBS=-lm -lc -lgcc

VPATH += $(UIP_DIR)/uip
VPATH += $(UIP_DIR)/apps/dhcpc

UIP_SRCS=$(UIP_DIR)/uip/uip.c \
	 $(UIP_DIR)/uip/uip_timer.c \
	 $(UIP_DIR)/uip/psock.c \
	 $(UIP_DIR)/uip/uip_arp.c \
	 $(UIP_DIR)/uip/uip_timer.c \
	 $(UIP_DIR)/apps/dhcpc/dhcpc.c

CXX_SRCS=main.cpp enc28j60.cpp
C_SRCS=dummyfuncs.c $(UIP_SRCS) httpd.c
#_CXX_SRCS=$(wildcard $(patsubst %, %/*.cpp, .))

SRCS=$(CXX_SRCS) $(C_SRCS)
OBJS=$(addprefix $(BUILD_DIR), $(notdir $(CXX_SRCS:.cpp=.o)) $(notdir $(C_SRCS:.c=.o)))
DEPS=$(OBJS:.o=.d)

TARGET_NAME=enc28j60
TARGET_ELF=$(BUILD_DIR)$(TARGET_NAME).elf
TARGET_BIN=$(BUILD_DIR)$(TARGET_NAME).bin

GCC_DEP_GEN=-MD -MP -MF $(BUILD_DIR)$(@F:.o=.d)

_CXXFLAGS=$(CXXFLAGS) $(GCC_DEP_GEN) $(DEFINES) $(INCLUDES)
_CFLAGS=$(CFLAGS) $(GCC_DEP_GEN) $(DEFINES) $(INCLUDES)

.PHONY: $(BUILD_DIR)

all: $(BUILD_DIR) $(TARGET_BIN)

$(BUILD_DIR): 
	echo $(OBJS)
	@mkdir -p $(BUILD_DIR)

$(TARGET_ELF): $(OBJS)
	$(CXX) $(LDFLAGS) $? -o $@ $(LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)

clean:
	@rm -f $(OBJS) $(TARGET_ELF) $(TARGET_BIN)

$(BUILD_DIR)%.o: %.cpp
	$(CXX) -c $(_CXXFLAGS) $< -o $@

$(BUILD_DIR)%.o: %.c
	$(CC) -c $(_CFLAGS) $< -o $@ 


-include $(DEPS)

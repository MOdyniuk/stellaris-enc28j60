CXX=arm-none-eabi-g++
CC=arm-none-eabi-gcc
STELLARIS=$(HOME)/opt/stellaris-launchpad
INCLUDES=-I$(STELLARIS) -Iuip-1.0 -I. -I uip-1.0/uip
CXXFLAGS=-DTARGET_IS_BLIZZARD_RA2 -DPART_LM4F120H5QR -DUART_BUFFERED $(INCLUDES) 
_CXXFLAGS=$(CXXFLAGS) -MD -MP -MF $(BUILD_DIR)$(@F:.o=.d)
BUILD_DIR=build/

CXX_SRCS=main.cpp enc28j60.cpp
_CXX_SRCS=$(wildcard $(patsubst %, %/*.cpp, .))
SRCS=$(CXX_SRCS)
OBJS=$(addprefix $(BUILD_DIR), $(CXX_SRCS:.cpp=.o))
DEPS=$(OBJS:.o=.d)

.PHONY: $(BUILD_DIR)

target: $(BUILD_DIR) $(OBJS)
	echo $(OBJS)

$(BUILD_DIR): 
	echo $(_CXX_SRCS)
	echo $(OBJS)
	@mkdir -p $(BUILD_DIR)

depend: 

clean:
	@rm $(OBJS)

$(BUILD_DIR)%.o: %.cpp
	$(CXX) -c $(_CXXFLAGS) $< -o $@

-include $(DEPS)

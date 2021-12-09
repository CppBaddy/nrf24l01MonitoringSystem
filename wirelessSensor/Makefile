SENSOR_ID=38

CC=avr-g++

DEVICE=84

CPU_CLOCK=4000000

EFUSE=0xff
HFUSE=0xdf
LFUSE=0x62
#LFUSE=0x22

CFLAGS=-g -std=c11 -Os -Wall -mcall-prologues -mmcu=attiny$(DEVICE) -DF_CPU=$(CPU_CLOCK)
## Use short (8-bit) data types
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -I/usr/lib/avr/include
CFLAGS +=  -DSENSOR_ID=$(SENSOR_ID)

LDFLAGS = -Wl,-Map,$(TARGET).map
## Optional, but often ends up with smaller code
LDFLAGS += -Wl,--gc-sections

OBJ2HEX=avr-objcopy

SIZE=avr-size

UISP=avrdude

TARGET=wirelessSensor

C_FILES = $(wildcard *.c)

OBJS = $(C_FILES:.c=.o)

all : clean build program

#program : flash eeprom

program : flash

fuse :
	$(UISP) -p t$(DEVICE) -c USBasp -v -U hfuse:w:${HFUSE}:m -U lfuse:w:${LFUSE}:m 

flash : $(TARGET).hex
	$(UISP) -p t$(DEVICE) -c USBasp -v -U flash:w:$(TARGET).hex:i

eeprom : $(TARGET).eep
	$(UISP) -p t$(DEVICE) -c USBasp -v -U eeprom:w:$(TARGET).eep:i

build : $(TARGET).hex $(TARGET).eep
	ls -l $(TARGET).*

%.hex : %.elf
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

%.eep : %.elf
	$(OBJ2HEX) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@


%.elf : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) -t $@

%.o : %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	@rm -f *.hex *.eep *.elf *.o

help :
	@echo "make [help | clean | build | eeprom | flash | program | fuse]"

#!/bin/bash

MCU=msp430g2231
PROG=lp_prototype
CXX_OPTIONS="-fno-exceptions -fno-rtti -Wl,--gc-sections -ffunction-sections -nodefaultlibs -lc -lgcc"

msp430-gcc -Os -Wall -g -mmcu=$MCU -o $PROG-$MCU.elf \
  lp_prototype.c \
  lp_prototype_interrupts.c

if [ -f $PROG-$MCU.elf ];
then
  sudo mspdebug rf2500 "prog $PROG-$MCU.elf"
fi

msp430-objdump -S $PROG-$MCU.elf > $PROG-$MCU.S

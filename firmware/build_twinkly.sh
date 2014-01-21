#!/bin/bash

MCU=msp430g2231
PROG=twinkly
CXX_OPTIONS="-fno-exceptions -fno-rtti -Wl,--gc-sections -ffunction-sections -nodefaultlibs -lc -lgcc"

msp430-gcc -Os -Wall -g -mmcu=$MCU -o $PROG-$MCU.elf \
  twinkly.c \
  twinkly_interrupts.c

if [ -f $PROG-$MCU.elf ];
then
  sudo mspdebug rf2500 "prog $PROG-$MCU.elf"
  echo
fi

msp430-objdump -S $PROG-$MCU.elf > $PROG-$MCU.S

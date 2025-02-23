#!/bin/bash

# need megatinycore installed for this

if [ "$(uname -o)" == "Msys" ]; then
	AVR_SIZE="C:/Program Files (x86)/Atmel/Studio/7.0/toolchain/avr8/avr8-gnu-toolchain/bin/avr-size.exe"
else
	AVR_SIZE="${HOME}/atmel_setup/avr8-gnu-toolchain-linux_x86_64/bin/avr-size"
fi

# Replace this with the path to your .elf file
ELF_FILE=$1

if [ "$ELF_FILE" == "" ]; then
  echo "Please specify a file: $0 <file> [max flash size]"
  exit 1
fi

# Constants for program storage and dynamic memory size
PROGRAM_STORAGE=32768
DYNAMIC_MEMORY=2048

# Run avr-size and parse the output
if [ "$(uname -o)" == "Msys" ]; then
	OUTPUT=$("$AVR_SIZE" -A $ELF_FILE)
else
	OUTPUT=$($AVR_SIZE -A $ELF_FILE)
fi

# Extract sizes of .text, .data, .rodata, and .bss sections
TEXT_SIZE=$(echo "$OUTPUT" | grep -E '\.text' | awk '{print $2}')
DATA_SIZE=$(echo "$OUTPUT" | grep -E '\.data' | awk '{print $2}')
RODATA_SIZE=$(echo "$OUTPUT" | grep -E '\.rodata' | awk '{print $2}')
BSS_SIZE=$(echo "$OUTPUT" | grep -E '\.bss' | awk '{print $2}')

# Calculate used program storage and dynamic memory
PROGRAM_STORAGE_USED=$((TEXT_SIZE + DATA_SIZE + RODATA_SIZE))
DYNAMIC_MEMORY_USED=$((DATA_SIZE + BSS_SIZE))

# Calculate percentages
PROGRAM_STORAGE_PERCENT=$(awk -v used="$PROGRAM_STORAGE_USED" -v total="$PROGRAM_STORAGE" 'BEGIN { printf("%.2f", used / total * 100) }')
DYNAMIC_MEMORY_PERCENT=$(awk -v used="$DYNAMIC_MEMORY_USED" -v total="$DYNAMIC_MEMORY" 'BEGIN { printf("%.2f", used / total * 100) }')

# if a max flash size was passed then compare it against the amount used
if [ ! -z "$2" ] && [ "$(printf '%d' $2)" -lt "$(printf '%d' $PROGRAM_STORAGE_USED)" ]; then
	echo "---------"
	echo "Failure! Program space: 0x$(printf '%x' $PROGRAM_STORAGE_USED) larger than allowed: 0x$(printf '%x' $2)"
	echo "Reduce program size within 0x$(printf '%x' $2), or reduce flash storage space then adjust BOOTEND in the Makefile"
	exit 1
fi

# Display the results
echo "Success! Program uses $PROGRAM_STORAGE_USED/$PROGRAM_STORAGE or $(printf '0x%x' $PROGRAM_STORAGE_USED)/$(printf '0x%x' $PROGRAM_STORAGE) bytes of flash space ($PROGRAM_STORAGE_PERCENT%)"
echo "Global variables use $DYNAMIC_MEMORY_USED/$DYNAMIC_MEMORY or $(printf '0x%x' $DYNAMIC_MEMORY_USED)/$(printf '0x%x' $DYNAMIC_MEMORY) bytes of SRAM ($DYNAMIC_MEMORY_PERCENT%)"



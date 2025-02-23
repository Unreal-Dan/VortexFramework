#!/bin/bash

FILE_PATH="$HOME/.arduino15/packages/adafruit/hardware/samd/1.7.16/cores/arduino/USB/CDC.cpp"

# Check if the file exists
if [ ! -f "$FILE_PATH" ]; then
    echo "Error: File does not exist."
    exit 1
fi
# Read the specific line and check its content
CURRENT_LINE=$(sed -n '258p' "$FILE_PATH")

if [ "$CURRENT_LINE" != "	delay(10);" ]; then
  if [ "$CURRENT_LINE" != "	\/\/delay(10);" ]; then
    echo "No changes made: line 258 does not match the expected content."
    exit 1
  fi
  echo "No changes made: line 258 has already been modified."
else
  # Replace the content of line 258
  sed -i '258s/.*/	\/\/delay(10);/' "$FILE_PATH"
  echo "Line 258, a delay(10), has been commented out in: [$FILE_PATH]"
fi

# =============================================================================================

FILE_PATH2="$HOME/.arduino15/packages/adafruit/hardware/samd/1.7.16/cores/arduino/USB/USBAPI.h"
# Check if the file exists
if [ ! -f "$FILE_PATH2" ]; then
    echo "Error: File does not exist."
    exit 1
fi
# Read the specific line and check its content
CURRENT_LINE=$(sed -n '182p' "$FILE_PATH2")
if [ "$CURRENT_LINE" != "private:" ]; then
  if [ "$CURRENT_LINE" != "\/\/private:" ]; then
    echo "No changes made: line 182 does not match the expected content."
    exit 1
  fi
  echo "No changes made: line 182 has already been modified."
else
  # replace the content of line
  sed -i '182s/.*/\/\/private:/' "$FILE_PATH2"
  echo "Line 182, private: has been commented out in: [$FILE_PATH2]"
fi

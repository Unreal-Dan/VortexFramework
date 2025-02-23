#include "Storage.h"

#include <string.h>
#include <stdlib.h>

#include "../VortexConfig.h"
#include "../Memory/Memory.h"
#include "../Serial/ByteStream.h"
#include "../Log/Log.h"

#ifdef VORTEX_EMBEDDED
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#endif

#ifdef VORTEX_LIB
#include "../VortexLib/VortexLib.h"
#endif

#ifndef VORTEX_EMBEDDED
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <stdio.h>
#endif
#endif

#define DEFAULT_STORAGE_FILENAME "FlashStorage.flash"

#ifdef VORTEX_LIB
std::string Storage::m_storageFilename;
#define STORAGE_FILENAME m_storageFilename.c_str()
#else
#define STORAGE_FILENAME DEFAULT_STORAGE_FILENAME
#endif

// The first half of the data goes into the eeprom and then the rest goes into
// flash, the EEPROM is 256 and storage size is 512 so the flash storage is 256
#define FLASH_STORAGE_SIZE (STORAGE_SIZE)
// The position of the flash storage is right before the end of the flash, as long
// as the program leaves 256 bytes of space at the end of flash then this will fit
#define FLASH_STORAGE_SPACE ((volatile uint8_t *)(0x10000 - FLASH_STORAGE_SIZE))
// The header is max 15 bytes big, this is decided by the MAX_MODE_SIZE being
// 76, since there's only 256 bytes in the eeprom that leaves exactly 27 bytes
// leftover after packing 3x modes. Out of the 27 bytes 12 is the ByteStream
// header leaving 15 for the max header size. Of the 15 only 7 bytes are really
// being used and the rest are extra bytes reserved for future use
#define HEADER_SIZE 15
// The full header size includes the 12 bytes for the serialbuffer header
#define STORAGE_HEADER_SIZE (HEADER_SIZE + 12)

uint32_t Storage::m_lastSaveSize = 0;

Storage::Storage()
{
}

bool Storage::init()
{
#ifdef VORTEX_LIB
  if (!m_storageFilename.length() && Vortex::storageEnabled()) {
    m_storageFilename = DEFAULT_STORAGE_FILENAME;
  }
#endif
  return true;
}

void Storage::cleanup()
{
}

#define FLASH_PAGE_SIZE 128


// store a serial buffer to storage
bool Storage::write(uint8_t slot, ByteStream &buffer)
{
#ifdef VORTEX_LIB
  if (!Vortex::storageEnabled()) {
    // success so the system thinks it all worked
    return true;
  }
#endif
  // check size
  uint16_t size = buffer.rawSize();
  if (buffer.rawSize() > MAX_MODE_SIZE) {
    ERROR_LOG("Buffer too big for storage space");
    return false;
  }
  if (slot >= NUM_MODE_SLOTS) {
    return false;
  }
  // just in case
  buffer.recalcCRC();
#ifdef VORTEX_EMBEDDED
  const uint8_t *buf = (const uint8_t *)buffer.rawData();
  // the header is slot 0 and it gets 17 bytes in the start of the eeprom then
  // the next 3 modes are 76 bytes each taking up 228 bytes for a total of 245
  if (slot < 4) {
    uint8_t eepSlot = 0;
    if (slot > 0) {
      eepSlot = STORAGE_HEADER_SIZE + (MAX_MODE_SIZE * (slot - 1));
    }
    // header eeprom (12 bytes of 256)
    // 3 modes eeprom (76 x 3 bytes of 244)
    uint8_t slotSize = (slot == 0) ? STORAGE_HEADER_SIZE : MAX_MODE_SIZE;
    for (uint8_t i = 0; i < slotSize; ++i) {
      uint8_t b = (i < size) ? buf[i] : 0x00;
      eepromWriteByte(eepSlot + i, b);
    }
  } else {
    // Then flash storage is 0x200 or 512 bytes which allows for 6 x 76 byte modes
    // the base address of the slot we are writing
    uint16_t slotAddr = (uint16_t)FLASH_STORAGE_SPACE + (MAX_MODE_SIZE * (slot - 4));
    // The storage slot may lay across a page boundary which means potentially writing
    // two pages instead of just one. In order to update only part of a page, the page
    // buffer must be filled with both the previous content along with the new data.
    // For example, imagine 2 pages of data: |xxxxxxSSSS|SSSxxxxxxx| the x's are other
    // data that must be preserved, and the S's denote the storage slot being written.
    // This would take place over two iterations of the loop, each writing out one page
    // by read-then-writing-back the x's and writing out the new S's. This is necessary
    // because the page buffer must be filled to perform a page write, at least I think
    while (size > 0) {
      uint16_t pageStart = slotAddr & ~(FLASH_PAGE_SIZE - 1);
      uint16_t offset = slotAddr % FLASH_PAGE_SIZE;
      uint16_t space = FLASH_PAGE_SIZE - offset;
      uint16_t writeSize = (size < space) ? size : space;

      for (uint8_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
        uint8_t value;
        if (i >= offset && i < offset + writeSize) {
          // if this is within the slot then write out the new data
          value = buf[i - offset];
        } else {
          // otherwise just write-back the same value to fill the pagebuffer
          value = *(volatile uint8_t *)(pageStart + i);
        }
        *(volatile uint8_t *)(pageStart + i) = value;
      }

      // Erase and write the flash page
      _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
      while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm));

      // continue to the next page
      slotAddr += writeSize;
      buf += writeSize;
      size -= writeSize;
    }
  }
#elif defined(_WIN32)
  HANDLE hFile = CreateFile(STORAGE_FILENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    // error
    return false;
  }
  DWORD written = 0;
  DWORD offset = slot * MAX_MODE_SIZE;
  SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
  uint8_t modeBuffer[MAX_MODE_SIZE] = {0};
  // copy the mode data into a temp buffer
  memcpy(modeBuffer, buffer.rawData(), size);
  // then copy the full size of the temp buffer in
  if (!WriteFile(hFile, modeBuffer, MAX_MODE_SIZE, &written, NULL)) {
    // error
    return false;
  }
  CloseHandle(hFile);
#else
  FILE *f = fopen(STORAGE_FILENAME, "w");
  if (!f) {
    return false;
  }
  long offset = slot * MAX_MODE_SIZE;
  fseek(f, offset, SEEK_SET);
  uint8_t modeBuffer[MAX_MODE_SIZE] = {0};
  // copy the mode data into a temp buffer
  memcpy(modeBuffer, buffer.rawData(), size);
  if (!fwrite(modeBuffer, sizeof(char), MAX_MODE_SIZE, f)) {
    return false;
  }
  fclose(f);
#endif // VORTEX_EMBEDDED
  DEBUG_LOGF("Wrote %u bytes to storage (max: %u)", m_lastSaveSize, STORAGE_SIZE);
  return true;
}

// read a serial buffer from storage
bool Storage::read(uint8_t slot, ByteStream &buffer)
{
#ifdef VORTEX_LIB
  if (!Vortex::storageEnabled()) {
    // return false here, but true in write because we don't want to return
    // an empty buffer after returning true
    return false;
  }
#endif
  uint32_t size = MAX_MODE_SIZE;
  if (size > STORAGE_SIZE || size < sizeof(ByteStream::RawBuffer) + 4 || slot >= NUM_MODE_SLOTS) {
    return false;
  }
  if (!buffer.init(size)) {
    return false;
  }
#ifdef VORTEX_EMBEDDED
  uint8_t *buf = (uint8_t *)buffer.rawData();
  volatile uint8_t *src;
  if (slot == 0) { // slot 0 is header eeprom
    src = (volatile uint8_t *)MAPPED_EEPROM_START;
    size = STORAGE_HEADER_SIZE;
  } else if (slot < 4) { // slots 1-3 are eeprom
    src = (volatile uint8_t *)MAPPED_EEPROM_START + STORAGE_HEADER_SIZE + (MAX_MODE_SIZE * (slot - 1));
  } else { // slots 4-9 are flash
    src = (volatile uint8_t *)FLASH_STORAGE_SPACE + (MAX_MODE_SIZE * (slot - 4));
  }
  for (uint8_t i = 0; i < size; ++i) {
    buf[i] = src[i];
  }
#elif defined(_WIN32)
  HANDLE hFile = CreateFile(STORAGE_FILENAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    // error
    return false;
  }
  DWORD bytesRead = 0;
  DWORD offset = slot * MAX_MODE_SIZE;
  SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
  if (!ReadFile(hFile, buffer.rawData(), MAX_MODE_SIZE, &bytesRead, NULL)) {
    // error
    return false;
  }
  CloseHandle(hFile);
#else
  FILE *f = fopen(STORAGE_FILENAME, "r");
  if (!f) {
    return false;
  }
  long offset = slot * MAX_MODE_SIZE;
  fseek(f, offset, SEEK_SET);
  if (!fread(buffer.rawData(), sizeof(char), MAX_MODE_SIZE, f)) {
    return false;
  }
  fclose(f);
#endif
  // ensure the internal buffer is sane after reading it out, this
  // prevents segfaults if the internal size reports larger than capacity
  buffer.sanity();
  // check crc immediately since we read into raw data copying the
  // array could be dangerous
  if (!buffer.checkCRC()) {
    buffer.clear();
    ERROR_LOG("Could not verify buffer");
    return false;
  }
  m_lastSaveSize = size;
  DEBUG_LOGF("Loaded savedata (Size: %u)", buffer.size());
  return true;
}

uint32_t Storage::lastSaveSize()
{
  return m_lastSaveSize;
}

#ifdef VORTEX_EMBEDDED
// write out the eeprom byte
void Storage::eepromWriteByte(uint16_t index, uint8_t in)
{
  if (eepromReadByte(index) == in) {
    return;
  }
  // The first two pages of the data goes into the eeprom and then the last page goes
  // into the USERROW which is located at 0x1300
  uint16_t adr = MAPPED_EEPROM_START + (index);
  __asm__ __volatile__(
    "ldi r30, 0x00"     "\n\t"
    "ldi r31, 0x10"     "\n\t"
    "ldd r18, Z+2"      "\n\t"
    "andi r18, 3"       "\n\t"
    "brne .-6"          "\n\t"
    "st X, %0"          "\n\t"
    "ldi %0, 0x9D"      "\n\t"
    "out 0x34, %0"      "\n\t"
    "ldi %0, 0x03"      "\n\t"
    "st Z, %0"          "\n\t"
    :"+d"(in)
    : "x"(adr)
    : "r30", "r31", "r18");

  while (!(NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm));
}

uint8_t Storage::eepromReadByte(uint16_t index)
{
  return *(volatile uint8_t *)(MAPPED_EEPROM_START + index);
}
#endif

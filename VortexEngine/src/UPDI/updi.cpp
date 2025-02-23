#include "updi.h"

#include "../VortexConfig.h"
#include "../Time/TimeControl.h"
#include "../Log/Log.h"
#include "../Leds/Leds.h"
#include "../Serial/ByteStream.h"
#include "../Patterns/Pattern.h"
#include "../Modes/Mode.h"

#ifdef VORTEX_EMBEDDED

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "soc/uart_periph.h"
#include "esp_rom_gpio.h"
#include "hal/uart_types.h"
#include "soc/gpio_struct.h"
#include "soc/soc.h"
#include "soc/io_mux_reg.h"
#include "esp_timer.h"
#include "driver/timer.h"
#include <Arduino.h>

#define HIGH 0x1
#define LOW 0x0

#define FLASH_PAGE_SIZE 128
#define EEPROM_PAGE_SIZE 64

// Define GPIO pin for UPDI communication
#define UPDI_PIN GPIO_NUM_10

// Timing macros (adjust as needed for your specific setup)
#define BIT_TIME_US 30
#define UPDI_BREAK_PAUSE_US    2000
#define UPDI_BREAK_DELAY_US    14000

// Direct GPIO manipulation macros using ESP32 registers
#define GPIO_SET_HIGH(gpio_num) (GPIO.out_w1ts.val = (1U << gpio_num))
#define GPIO_SET_LOW(gpio_num) (GPIO.out_w1tc.val = (1U << gpio_num))
#define GPIO_READ(gpio_num) ((GPIO.in.val >> gpio_num) & 0x1)

// Configure GPIO for output
#define GPIO_SET_OUTPUT(gpio_num) (GPIO.enable_w1ts.val = (1U << gpio_num))

// Configure GPIO for input
// the key here is setting the gpio as GPIO_FLOATING to prevent contention
#define GPIO_SET_INPUT(gpio_num) (GPIO.enable_w1tc.val = (1U << gpio_num)); gpio_set_pull_mode((gpio_num_t)gpio_num, GPIO_FLOATING);
#endif

// the max size of a single mode
#define DUO_MODE_SIZE 76
// the size of the actual storage header data
#define DUO_HEADER_SIZE 15
// the full size of the storage header, data + metadata (first 27 bytes of eeprom)
#define DUO_HEADER_FULL_SIZE (DUO_HEADER_SIZE + 12)
// the base address of eeprom and where the save header starts
// followed by 3 modes each 76 makes 255 bytes of eeprom
#define DUO_EEPROM_BASE 0x1400
// then the next 6 modes are stored in flash from 0xfe00 to 0x10000
// which is 512 bytes of space that can be adjusted
#define DUO_FLASH_STORAGE_BASE 0xFe00
// the actual firmware is 0x8000 to 0x10000
#define DUO_FIRMWARE_BASE 0x8000

// the old duo header was embedded in the entire mode save
// so we have to load the whole modes
#define LEGACY_DUO_HEADER_SIZE_1 5
#define LEGACY_DUO_HEADER_SIZE_2 255

// The Duo has a special Modes flag (all of upper bits) which means a new
// firmware was flashed and it should turn on and write out a new save header
// This is setting the global flags to represent:
//
//   0 = button lock enabled
//   0 = one click mode enabled
//   0 = advanced menus enabled
//   0 = keychain mode enabled
//   1111 = Startup Mode Index 15 (impossible)
//
// When the Duo turns on and see this it will rewrite it's own save header
#define DUO_MODES_FLAG_NEW_FIRMWARE  0xF0

// the storage type of the duo, detected by reading the save header size
// older versions of duo use legacy storage types, must account for them
UPDI::StorageType UPDI::m_storageType = MODERN_STORAGE;

DuoHeader UPDI::m_lastSaveHeader;

// Compile-time check on the size of the DuoHeader against definition
static_assert(DUO_HEADER_SIZE == sizeof(DuoHeader), "Incorrect DuoHeader size");

bool UPDI::init()
{
#ifdef VORTEX_EMBEDDED
  m_storageType = MODERN_STORAGE;
  memset(&m_lastSaveHeader, 0, sizeof(m_lastSaveHeader));
#endif
  return true;
}

void UPDI::cleanup()
{
}

uint8_t UPDI::isConnected()
{
#ifdef VORTEX_EMBEDDED
  sendDoubleBreak();
  stcs(Control_A, 0x6);
  return cpu_mode<0xEF>();
#else
  return true;
#endif
}

bool UPDI::readHeader(ByteStream &header)
{
#ifdef VORTEX_EMBEDDED
  if (!header.init(DUO_HEADER_SIZE)) {
    return false;
  }
  if (!enterProgrammingMode()) {
    return false;
  }
  uint8_t *ptr = (uint8_t *)header.rawData();
  uint16_t addr = DUO_EEPROM_BASE;
  stptr_p((const uint8_t *)&addr, 2);
  for (uint16_t i = 0; i < 4; ++i) {
    ptr[i] = ldinc_b();
  }
  const uint32_t size = *(uint32_t *)ptr;
  if (!size) {
    return false;
  }
  // more than 30 is old old duo where header is combined with save
  if (size > 30) {
    return readHeaderLegacy2(header);
  }
  // less than 6 is old duo where header is separate but smaller (no build number)
  if (size < 6) {
    return readHeaderLegacy1(header);
  }
  // modern duo header is 27 total and separate from modes
  stptr_p((const uint8_t *)&addr, 2);
  for (uint16_t i = 0; i < header.rawSize(); ++i) {
    ptr[i] = ldinc_b();
  }
  header.sanity();
  if (!header.checkCRC()) {
    header.clear();
    ERROR_LOG("ERROR Header CRC Invalid!");
    return false;
  }
  // copy into the last save header
  copyLastSaveHeader(header);
  // Modern Duo with segmented header and build number in header
  m_storageType = MODERN_STORAGE;
#endif
  return true;
}

#ifdef VORTEX_EMBEDDED
// kinda old duos like 1.2.0 to 1.3.0
bool UPDI::readHeaderLegacy1(ByteStream &header)
{
  // around 1.2.0 the duo storage was segmented to fix the storage issue
  // so the header is a separate buffer that's still at the start but only
  // takes up about 17 bytes (12 + 5) of the start of the eeprom because
  // it is contained in it's own ByteStream. Later on the size was increased
  // by 10 bytes and 1 more of those bytes were used to store the build number
  if (!header.init(LEGACY_DUO_HEADER_SIZE_1)) {
    return false;
  }
  if (!enterProgrammingMode()) {
    return false;
  }
  uint8_t *ptr = (uint8_t *)header.rawData();
  uint16_t addr = DUO_EEPROM_BASE;
  stptr_p((const uint8_t *)&addr, 2);
  for (uint16_t i = 0; i < header.rawSize(); ++i) {
    ptr[i] = ldinc_b();
  }
  header.sanity();
  if (!header.checkCRC()) {
    header.clear();
    ERROR_LOG("ERROR Header CRC Invalid!");
    reset();
    return false;
  }
  // copy into the last save header
  copyLastSaveHeader(header);
  // LEGACY DUO! Old Storage format with segmented header and no build number
  m_storageType = LEGACY_STORAGE_1;
  return true;
}

void UPDI::copyLastSaveHeader(const ByteStream &srcData)
{
  // copy into the last save header
  memset(&m_lastSaveHeader, 0, sizeof(m_lastSaveHeader));
  // the amount to copy into the last save header
  uint32_t copysize = sizeof(m_lastSaveHeader);
  if (srcData.size() < sizeof(m_lastSaveHeader)) {
    copysize = srcData.size();
  }
  // copy in the data
  memcpy(&m_lastSaveHeader, srcData.data(), copysize);
}

// really old duos like 1.0.0
bool UPDI::readHeaderLegacy2(ByteStream &header)
{
  // Different size for old old duos, the whole eeprom was the only storage
  // and it was thought the userrow provided another 128 but in reality that
  // didn't actually work and only 255 bytes were available causing a bad
  // storage bug that erased modes. So this reads the entire storage even
  // though we only need the header at the start
  if (!header.init(LEGACY_DUO_HEADER_SIZE_2)) {
    return false;
  }
  //if (!enterProgrammingMode()) {
  //  return false;
  //}
  uint8_t *ptr = (uint8_t *)header.rawData();
  uint16_t addr = DUO_EEPROM_BASE;
  stptr_p((const uint8_t *)&addr, 2);
  for (uint16_t i = 0; i < header.rawSize(); ++i) {
    ptr[i] = ldinc_b();
  }
  header.sanity();
  if (!header.checkCRC()) {
    header.clear();
    ERROR_LOG("ERROR Header CRC Invalid!");
    reset();
    return false;
  }
  // copy into the last save header
  copyLastSaveHeader(header);
  // LEGACY DUO! Old Storage format with combined header and no build number
  m_storageType = LEGACY_STORAGE_2;
  return true;
}

#endif

bool UPDI::readMode(uint8_t idx, ByteStream &modeBuffer)
{
#ifdef VORTEX_EMBEDDED
  if (m_storageType == LEGACY_STORAGE_2) {
    // nope I'm not going to write code to read the old duos they can
    // just send the mode c2c to the deck if they want to save it then
    // they can update their duo and push the mode back to it
    return false;
  }
  // initialize mode buffer
  if (!modeBuffer.init(DUO_MODE_SIZE)) {
    modeBuffer.clear();
    return false;
  }
  if (!enterProgrammingMode()) {
    modeBuffer.clear();
    return false;
  }
  // DUO_MODE_SIZE is the max duo mode size (the slot size)
  uint8_t *ptr = (uint8_t *)modeBuffer.rawData();
  uint16_t numBytes = modeBuffer.rawSize();
  uint16_t base;
  // there are 3 modes in the eeprom after the header
  if (idx < 3) {
    // DUO_EEPROM_BASE is eeprom base
    // DUO_HEADER_FULL_SIZE is size of duo header
    // DUO_MODE_SIZE is size of each duo mode
    base = DUO_EEPROM_BASE + (DUO_HEADER_FULL_SIZE) + (idx * DUO_MODE_SIZE);
    // legacy storage 1 the header is a bit smaller so the first mode is a bit
    // sooner in the eeprom but that's the only difference
    if (m_storageType == LEGACY_STORAGE_1) {
      base -= 10;
    }
  } else {
    // DUO_FLASH_STORAGE_BASE is the end of flash, 0x200 before
    base = DUO_FLASH_STORAGE_BASE + ((idx - 3) * DUO_MODE_SIZE);
  }
  stptr_p((const uint8_t *)&base, 2);
  //rep(numBytes - 1);
  for (uint16_t i = 0; i < modeBuffer.rawSize(); ++i) {
    ptr[i] = ldinc_b();
  }
  modeBuffer.sanity();
  if (!modeBuffer.checkCRC()) {
    modeBuffer.clear();
    ERROR_LOG("ERROR Header CRC Invalid!");
    return false;
  }
#endif
  return true;
}

bool UPDI::writeHeader(ByteStream &headerBuffer)
{
#ifdef VORTEX_EMBEDDED
  if (m_storageType != MODERN_STORAGE) {
    // nope!
    return false;
  }
  if (!enterProgrammingMode()) {
    return false;
  }
  // DUO_EEPROM_BASE is eeprom base
  // DUO_HEADER_FULL_SIZE is size of duo header
  // DUO_MODE_SIZE is size of each duo mode
  uint16_t base = DUO_EEPROM_BASE;
  // the size of the mode being written out
  uint16_t size = headerBuffer.rawSize();
  uint8_t *ptr = (uint8_t *)headerBuffer.rawData();

  // read out the page so the |xxxxxxxxxxx part
  ByteStream pageBuffer(EEPROM_PAGE_SIZE);
  uint8_t *pagePtr = (uint8_t *)pageBuffer.data();
  uint16_t end = base + DUO_HEADER_FULL_SIZE;
  stptr_p((const uint8_t *)&end, 2);
  for (uint16_t i = DUO_HEADER_FULL_SIZE; i < EEPROM_PAGE_SIZE; ++i) {
    pagePtr[i] = ldinc_b();
  }
  nvmWait();

  // overlay the actual data, so the SSSSS part of |xxxxxxSSSS
  memcpy(pagePtr, ptr, size);

  for (uint16_t i = 0; i < EEPROM_PAGE_SIZE; ++i) {
    sts_b(base + i, pagePtr[i]);
  }
  nvmCmd(NVM_ERWP);
  nvmWait();
#endif
  return true;
}

bool UPDI::writeMode(uint8_t idx, ByteStream &modeBuffer)
{
#ifdef VORTEX_EMBEDDED
  if (m_storageType != MODERN_STORAGE) {
    // nope!
    return false;
  }
  modeBuffer.sanity();
  // TODO: idk if this is really necessary here anymore? I forget why it's here
  if (!modeBuffer.checkCRC()) {
    ERROR_LOG("ERROR Mode CRC Invalid!");
    return false;
  }
  // check against the max mode size so we don't write beyond limits
  if (modeBuffer.rawSize() > DUO_MODE_SIZE) {
    ERROR_LOG("ERROR Mode CRC Invalid!");
    return false;
  }
  // there are 3 modes in the eeprom after the header
  if (idx < 3) {
    return writeModeEeprom(idx, modeBuffer);
  }
  return writeModeFlash(idx, modeBuffer);
#else
  return true;
#endif
}

#ifdef VORTEX_EMBEDDED
bool UPDI::writeModeEeprom(uint8_t idx, ByteStream &modeBuffer)
{
  if (!enterProgrammingMode()) {
    return false;
  }
  // DUO_EEPROM_BASE is eeprom base
  // DUO_HEADER_FULL_SIZE is size of duo header
  // DUO_MODE_SIZE is size of each duo mode
  uint16_t base = DUO_EEPROM_BASE + DUO_HEADER_FULL_SIZE + (idx * DUO_MODE_SIZE);
  // the size of the mode being written out
  uint16_t size = modeBuffer.rawSize();
  uint8_t *ptr = (uint8_t *)modeBuffer.rawData();
  // The storage slot may lay across a page boundary which means potentially writing
  // two pages instead of just one. In order to update only part of a page, the page
  // buffer must be filled with both the previous content along with the new data.
  // For example, imagine 2 pages of data: |xxxxxxSSSS|SSSxxxxxxx| the x's are other
  // data that must be preserved, and the S's denote the storage slot being written.
  // This would take place over two iterations of the loop, each writing out one page
  // by read-then-writing-back the x's and writing out the new S's. This is necessary
  // because the page buffer must be filled to perform a page write, at least I think
  while (size > 0) {
    uint16_t pageStart = base & ~(EEPROM_PAGE_SIZE - 1);
    uint16_t offset = base % EEPROM_PAGE_SIZE;
    uint16_t space = EEPROM_PAGE_SIZE - offset;
    uint16_t writeSize = (size < space) ? size : space;

    // read out the page so the |xxxxxxxxxxx part
    ByteStream pageBuffer(EEPROM_PAGE_SIZE);
    uint8_t *pagePtr = (uint8_t *)pageBuffer.data();
    stptr_p((const uint8_t *)&pageStart, 2);
    for (uint16_t i = 0; i < EEPROM_PAGE_SIZE; ++i) {
      pagePtr[i] = ldinc_b();
    }
    nvmWait();

    // overlay the actual data, so the SSSSS part of |xxxxxxSSSS
    memcpy(pagePtr + offset, ptr, writeSize);

    // write back the page
    //nvmCmd(NVM_PBC);
    //stptr_p((const uint8_t *)&pageStart, 2);
    //stcs(Control_A, 0x0E);
    //rep(pageSize - 1);
    //stinc_b_noget(buf[0]);
    //for (uint8_t i = 1; i < pageSize; ++i) {
    //  sendByte(buf[i]);
    //}
    //sts_b(pageStart, pagePtr[0]);
    for (uint16_t i = 0; i < EEPROM_PAGE_SIZE; ++i) {
      //stinc_b(pagePtr[i]);
      sts_b(pageStart + i, pagePtr[i]);
    }
    //stcs(Control_A, 0x06);
    nvmCmd(NVM_ERWP);
    nvmWait();

    //for (uint8_t i = 0; i < EEPROM_PAGE_SIZE; ++i) {
    //  uint8_t value;
    //  if (i >= offset && i < offset + writeSize) {
    //    // if this is within the slot then write out the new data
    //    value = ptr[i - offset];
    //  } else {
    //    // otherwise just write-back the same value to fill the pagebuffer
    //    uint16_t addr = pageStart + i;
    //    stptr_p((const uint8_t *)&addr, 2);
    //    value = ld_b();
    //  }
    //  sts_b(pageStart + i, value);
    //}

    //bool cont = false;
    //for (uint8_t i = 0; i < EEPROM_PAGE_SIZE; ++i) {
    //  // if outside the write area
    //  if (i < offset || i >= (offset + writeSize)) {
    //    // if continuously reading just read and continue
    //    if (cont) {
    //      ptr[i] = ldinc_b();
    //      continue;
    //    }
    //    // otherwise start reading here
    //    uint16_t addr = pageStart + i;
    //    stptr_p((const uint8_t *)&addr, 2);
    //    ptr[i] = ldinc_b();
    //    cont = true;
    //  } else {
    //    // no longer continuously reading
    //    cont = false;
    //  }
    //}


    //// first read out the rest of the page
    //stptr_p((const uint8_t *)&base, 2);
    ////rep(numBytes - 1);
    //for (uint16_t i = 0; i < modeBuffer.rawSize(); ++i) {
    //  ptr[i] = ldinc_b();
    //}

    //nvmCmd(NVM_ERWP);
    //nvmWait();
    //nvmCmd(NVM_PBC);
    //stptr_w(pageStart + offset);
    //stcs(Control_A, 0x0E);
    //rep(writeSize - 1);
    //stinc_b_noget(ptr[0]);
    //for (uint8_t i = 1; i < writeSize; ++i) {
    //  sendByte(ptr[i]);
    //}
    //stcs(Control_A, 0x06);
    //nvmCmd(NVM_WP);
    //nvmWait();

    // continue to the next page
    base += writeSize;
    ptr += writeSize;
    size -= writeSize;
  }
  return true;
}

bool UPDI::writeModeFlash(uint8_t idx, ByteStream &modeBuffer)
{
  if (!enterProgrammingMode()) {
    return false;
  }
  // there are 3 modes in the eeprom after the header
  // DUO_FLASH_STORAGE_BASE is the end of flash, 0x200 before
  uint16_t base = DUO_FLASH_STORAGE_BASE + ((idx - 3) * DUO_MODE_SIZE);
  uint16_t size = modeBuffer.rawSize();
  uint8_t *ptr = (uint8_t *)modeBuffer.rawData();
  // The storage slot may lay across a page boundary which means potentially writing
  // two pages instead of just one. In order to update only part of a page, the page
  // buffer must be filled with both the previous content along with the new data.
  // For example, imagine 2 pages of data: |xxxxxxSSSS|SSSxxxxxxx| the x's are other
  // data that must be preserved, and the S's denote the storage slot being written.
  // This would take place over two iterations of the loop, each writing out one page
  // by read-then-writing-back the x's and writing out the new S's. This is necessary
  // because the page buffer must be filled to perform a page write, at least I think
  while (size > 0) {
    uint16_t pageStart = base & ~(FLASH_PAGE_SIZE - 1);
    uint16_t offset = base % FLASH_PAGE_SIZE;
    uint16_t space = FLASH_PAGE_SIZE - offset;
    uint16_t writeSize = (size < space) ? size : space;

    // read out the page
    ByteStream pageBuffer(FLASH_PAGE_SIZE);
    uint8_t *pagePtr = (uint8_t *)pageBuffer.data();
//#define NUM_SLICES 8
//#define PAGE_SLICE (FLASH_PAGE_SIZE / NUM_SLICES)
//    for (uint16_t j = 0; j < NUM_SLICES; ++j) {
//      uint16_t pos = pageStart = j;
//      stptr_p((const uint8_t *)&pos, 2);
//      for (uint16_t i = 0; i < PAGE_SLICE; ++i) {
//        ptr[(j * PAGE_SLICE) + i] = ldinc_b();
//      }
//      // wait idk
//      nvmWait();
//    }

    stptr_p((const uint8_t *)&pageStart, 2);
    for (uint16_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
      pagePtr[i] = ldinc_b();
    }
    //if (!readFlashPage(pageStart, pageBuffer)) {
    //  return false;
    //}
    // copy in the thing
    memcpy(pagePtr + offset, ptr, writeSize);

    // write back the page
    //nvmCmd(NVM_PBC);
    //stptr_p((const uint8_t *)&pageStart, 2);
    //stcs(Control_A, 0x0E);
    //rep(pageSize - 1);
    //stinc_b_noget(buf[0]);
    //for (uint8_t i = 1; i < pageSize; ++i) {
    //  sendByte(buf[i]);
    //}
    //sts_b(pageStart, pagePtr[0]);
    for (uint16_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
      //stinc_b(pagePtr[i]);
      sts_b(pageStart + i, pagePtr[i]);
    }
    //stcs(Control_A, 0x06);
    nvmCmd(NVM_ERWP);
    nvmWait();

    //if (!writeFlashPage(pageStart, pageBuffer)) {
    //  return false;
    //}

    //for (uint8_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
    //  uint8_t value;
    //  if (i >= offset && i < offset + writeSize) {
    //    // if this is within the slot then write out the new data
    //    value = ptr[i - offset];
    //  } else {
    //    // otherwise just write-back the same value to fill the pagebuffer
    //    uint16_t addr = pageStart + i;
    //    stptr_p((const uint8_t *)&addr, 2);
    //    value = ld_b();
    //  }
    //  sts_b(pageStart + i, value);
    //}

    //bool cont = false;
    //for (uint8_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
    //  // if outside the write area
    //  if (i < offset || i >= (offset + writeSize)) {
    //    // if continuously reading just read and continue
    //    if (cont) {
    //      ptr[i] = ldinc_b();
    //      continue;
    //    }
    //    // otherwise start reading here
    //    uint16_t addr = pageStart + i;
    //    stptr_p((const uint8_t *)&addr, 2);
    //    ptr[i] = ldinc_b();
    //    cont = true;
    //  } else {
    //    // no longer continuously reading
    //    cont = false;
    //  }
    //}



    //nvmCmd(NVM_ERWP);
    //nvmWait();
    //nvmCmd(NVM_PBC);
    //stptr_w(pageStart + offset);
    //stcs(Control_A, 0x0E);
    //rep(writeSize - 1);
    //stinc_b_noget(ptr[0]);
    //for (uint8_t i = 1; i < writeSize; ++i) {
    //  sendByte(ptr[i]);
    //}
    //stcs(Control_A, 0x06);
    //nvmCmd(NVM_WP);
    //nvmWait();

    // continue to the next page
    base += writeSize;
    ptr += writeSize;
    size -= writeSize;
  }
  return true;
}

bool UPDI::writePage(uint16_t addr, const uint8_t *buf, uint16_t pageSize)
{
    stptr_w(addr);
    stcs(Control_A, 0x0E);
    rep(FLASH_PAGE_SIZE - 1);
    stinc_b_noget(buf[0]);
    for (uint16_t i = 1; i < FLASH_PAGE_SIZE; ++i) {
      sendByte(buf[i]);
    }
    stcs(Control_A, 0x06);
    nvmCmd(NVM_ERWP);
    nvmWait();

  //nvmCmd(NVM_PBC);
  //stptr_p((const uint8_t *)&addr, 2);
  ////stcs(Control_A, 0x0E);
  ////rep(pageSize - 1);
  ////stinc_b_noget(buf[0]);
  ////for (uint8_t i = 1; i < pageSize; ++i) {
  ////  sendByte(buf[i]);
  ////}
  //for (uint8_t i = 0; i < pageSize; ++i) {
  //  //stinc_b_noget(buf[i]);
  //  sts_b(addr + i, buf[i]);
  //}
  ////stcs(Control_A, 0x06);
  //nvmCmd(NVM_ERWP);
  //nvmWait();
  return true;
}

bool UPDI::readPage(uint8_t addr, uint8_t *buf, uint16_t pageSize)
{
  stptr_p((const uint8_t *)&addr, 2);
  for (uint16_t i = 0; i < pageSize; ++i) {
    buf[i] = ldinc_b();
  }
  //stcs(Control_A, 0x0E);
  //rep(pageSize - 1);
  //buf[0] = ldinc_b();
  //for (uint16_t i = 0; i < pageSize; ++i) {
  //  stptr_w(addr + i);
  //  buf[i] = ld_b();
  //}
  //stcs(Control_A, 0x06);
  return true;
}

bool UPDI::writeFlashPage(uint16_t addr, const uint8_t *buf)
{
  return writePage(addr, buf, FLASH_PAGE_SIZE);
}

bool UPDI::readFlashPage(uint8_t addr, uint8_t *buf)
{
  return readPage(addr, buf, FLASH_PAGE_SIZE);
}

bool UPDI::writeEepromPage(uint16_t addr, const uint8_t *buf)
{
  return writePage(addr, buf, EEPROM_PAGE_SIZE);
}

bool UPDI::readEepromPage(uint8_t addr, uint8_t *buf)
{
  return readPage(addr, buf, EEPROM_PAGE_SIZE);
}

#endif

bool UPDI::writeFirmware(uint32_t position, ByteStream &firmwareBuffer)
{
#ifdef VORTEX_EMBEDDED
  firmwareBuffer.sanity();
  if (!firmwareBuffer.checkCRC()) {
    ERROR_LOG("ERROR Header CRC Invalid!");
    reset();
    return false;
  }
  if (!enterProgrammingMode()) {
    return false;
  }
  // DUO_MODE_SIZE is the max duo mode size (the slot size)
  uint8_t *ptr = (uint8_t *)firmwareBuffer.data();
  // there are 3 modes in the eeprom after the header
  // DUO_FLASH_STORAGE_BASE is the end of flash, 0x200 before
  uint16_t base = DUO_FIRMWARE_BASE + position;
  uint16_t size = firmwareBuffer.size();
  while (size > 0) {
    uint16_t writeSize = (size < FLASH_PAGE_SIZE) ? size : FLASH_PAGE_SIZE;
    //for (uint8_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
    //  uint8_t value = (i < writeSize) ? ptr[i] : 0;
    //  sts_b(base + i, value);
    //}
    stptr_w(base);
    stcs(Control_A, 0x0E);
    rep(FLASH_PAGE_SIZE - 1);
    stinc_b_noget(ptr[0]);
    for (uint16_t i = 1; i < FLASH_PAGE_SIZE; ++i) {
      sendByte((i < writeSize) ? ptr[i] : 0);
      //sts_b(base + i, (i < writeSize) ? ptr[i] : 0);
    }
    stcs(Control_A, 0x06);
    nvmCmd(NVM_ERWP);
    nvmWait();
    // continue to the next page
    base += writeSize;
    ptr += writeSize;
    size -= writeSize;
  }
#endif
  return true;
}

// This is a modern feature for Duos right after flashing firmware which will
// tell the duo to turn on and write out it's new save header which contains
// the new version number, this allows the chromalink firmware update process
// to be immediately reconnected without user interaction. Normally the Duo
// save header would still contain the old save data, the firmware update
// cannot write a new save header the duo must do that itself
bool UPDI::setFlagNewFirmware()
{
#ifdef VORTEX_EMBEDDED
  // first read out the existing duo header so we can patch it
  ByteStream duoHeader;
  if (!readHeader(duoHeader) || duoHeader.size() < 2) {
    return false;
  }
  // only use the header trick if it's a modern duo
  if (m_storageType != MODERN_STORAGE) {
    // can't do it
    return true;
  }
  // modify the global flags of the saveheader to indicate a new firmware
  DuoHeader *pHeader = (DuoHeader *)duoHeader.data();
  pHeader->globalFlags = DUO_MODES_FLAG_NEW_FIRMWARE;
  // force recalculate the CRC after changing the global flags
  duoHeader.recalcCRC(true);
  // write back the header with this new global flags
  if (!writeHeader(duoHeader)) {
    return false;
  }
#endif
  return true;
}


bool UPDI::eraseMemory()
{
  uint8_t status = 0;
#ifdef VORTEX_EMBEDDED
  sendDoubleBreak();
  sendEraseKey();
  status = ldcs(ASI_Key_Status);
  if (status != 0x8) {
    ERROR_LOGF("Erasing mem, bad key status: 0x%02x...", status);
  }
  reset();
#endif
  return status == 0x8;
}

bool UPDI::reset()
{
#ifdef VORTEX_EMBEDDED
  resetOn();
  return resetOff();
#else
  return true;
#endif
}

bool UPDI::disable()
{
#ifdef VORTEX_EMBEDDED
  // UPDIDIS bit to CTRLB
  stcs(Control_B, 0x2);
#endif
  return true;
}

#ifdef VORTEX_EMBEDDED
#define MAX_TRIES 1000

bool UPDI::enterProgrammingMode()
{
  uint8_t mode;
  uint32_t tries = 0;
  while (tries++ < MAX_TRIES) {
    sendDoubleBreak();
    stcs(Control_A, 0x6);
    mode = cpu_mode<0xEF>();
    if (mode != 0x82 && mode != 0x21 && mode != 0xA2 && mode != 0x08) {
      ERROR_LOGF("Bad CPU Mode 0x%02x... error: 0x%02x", mode, status);
      sendBreak();
      uint8_t status = ldcs(Status_B);
      continue;
    }
    if (mode != 0x08) {
      sendProgKey();
      uint8_t status = ldcs(ASI_Key_Status);
      if (status != 0x10) {
        ERROR_LOGF("Bad prog key status: 0x%02x", status);
        reset();
        //sendBreak();
        //uint8_t status = ldcs(Status_B);
        continue;
      }
      reset();
    }
    break;
  }
  if (tries >= MAX_TRIES) {
    Leds::holdAll(RGB_RED);
    return false;
  }
  mode = cpu_mode();
  while (mode != 0x8) {
    mode = cpu_mode();
  }
  return true;
}

void UPDI::resetOn()
{
  stcs(ASI_Reset_Request, 0x59);
}

bool UPDI::resetOff()
{
  stcs(ASI_Reset_Request, 0x0);
  uint8_t timeout = 0;
  while (cpu_mode<0x0E>() == 0 && timeout < 10) {
    Time::delayMicroseconds(100);
    timeout++;
  }
  return timeout < 2;
}

void UPDI::sendByte(uint8_t byte)
{
  uint8_t parity = __builtin_parity(byte);
  GPIO_SET_OUTPUT(UPDI_PIN);
  // Send start bit (low)
  GPIO_SET_LOW(UPDI_PIN);
  Time::delayMicroseconds(BIT_TIME_US);
  // Send data bits (LSB first)
  for (uint8_t i = 0; i < 8; i++) {
    if (byte & 0x01) {
      GPIO_SET_HIGH(UPDI_PIN);
    } else {
      GPIO_SET_LOW(UPDI_PIN);
    }
    Time::delayMicroseconds(BIT_TIME_US);
    byte >>= 1;
  }
  // Send parity bit (even parity)
  if (parity) {
    GPIO_SET_HIGH(UPDI_PIN);
  } else {
    GPIO_SET_LOW(UPDI_PIN);
  }
  Time::delayMicroseconds(BIT_TIME_US);
  // Send stop bit (high)
  GPIO_SET_HIGH(UPDI_PIN);
  Time::delayMicroseconds(BIT_TIME_US * 2);
  GPIO_SET_INPUT(UPDI_PIN);
}

uint8_t UPDI::receiveByte()
{
  uint8_t byte = 0;
  uint32_t counter = 0;
#define TIMEOUT_TT 40000
  while (GPIO_READ(UPDI_PIN) == 1 && counter++ < TIMEOUT_TT);
  if (counter >= TIMEOUT_TT) {
    ERROR_LOG("Timed out waiting for start bit");
    return 0;
  }
  Time::delayMicroseconds(BIT_TIME_US / 2); // Half bit time for sampling
  // Read data bits (LSB first)
  for (uint8_t i = 0; i < 8; i++) {
    Time::delayMicroseconds(BIT_TIME_US);
    byte >>= 1;
    if (GPIO_READ(UPDI_PIN)) {
      byte |= 0x80;
    }
  }
  // Read and discard parity bit
  Time::delayMicroseconds(BIT_TIME_US);
  // Read stop bit
  Time::delayMicroseconds(BIT_TIME_US);
  if (GPIO_READ(UPDI_PIN) == 0) {
    //ERROR_LOG("Stop bit error");
  }
  Time::delayMicroseconds(BIT_TIME_US * 2);
  return byte;
}

void UPDI::sendBreak()
{
  GPIO_SET_OUTPUT(UPDI_PIN);
  GPIO_SET_LOW(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_DELAY_US);
  GPIO_SET_HIGH(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_DELAY_US);
}

void UPDI::sendDoubleBreak()
{
  GPIO_SET_OUTPUT(UPDI_PIN);

  // First break signal
  GPIO_SET_LOW(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_DELAY_US);
  GPIO_SET_HIGH(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_PAUSE_US);

  // Second break signal
  GPIO_SET_LOW(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_DELAY_US);
  GPIO_SET_HIGH(UPDI_PIN);
  Time::delayMicroseconds(UPDI_BREAK_DELAY_US);
}

void UPDI::sendKey(const char *key)
{
  sendByte(0x55);
  sendByte(0xE0);
  for (int8_t i = 7; i >= 0; --i) {
    sendByte(key[i]);
  }
}

uint8_t UPDI::ldcs(cs_reg reg)
{
  sendByte(0x55);  // SYNCH
  sendByte(0x80 + reg);
  return receiveByte();
}

void UPDI::stcs(cs_reg r, uint8_t data)
{
  sendByte(0x55);
  sendByte(0xC0 + r);
  sendByte(data);
}

void UPDI::stptr_p(const uint8_t *addr_p, uint8_t n)
{
  sendByte(0x55);
  sendByte(0x68 + --n);
  sendByte(*(addr_p++));
  if (n >= 1) {
    sendByte(*(addr_p++));
  }
  if (n >= 2) {
    sendByte(*addr_p);
  }
  receiveByte();
}

void UPDI::stptr_l(uint32_t address)
{
  sendByte(0x55);
  sendByte(0x6A);
  sendByte(address & 0xFF);
  sendByte((address >> 8) & 0xFF);
  sendByte((address >> 16) & 0xFF);
  receiveByte();
}

void UPDI::stptr_w(uint16_t address)
{
  sendByte(0x55);
  sendByte(0x69);
  sendByte(address & 0xFF);
  sendByte(address >> 8);
  receiveByte();
}

void UPDI::stptr_inc_16(uint8_t *data, uint16_t len)
{
  sendByte(0x55);
  sendByte(0x65);
  receiveByte();
  uint16_t n = 2;
  while (n < len)
  {
    sendByte(data[n]);
    sendByte(data[n + 1]);
    receiveByte();
    n += 2;
  }
}

void UPDI::rep(uint8_t repeats)
{
  sendByte(0x55);
  sendByte(0xA0);
  sendByte(repeats);
}

uint8_t UPDI::ldinc_b()
{
  sendByte(0x55);
  sendByte(0x24);
  return receiveByte();
}

void UPDI::sts_b(uint16_t address, uint8_t data)
{
  sendByte(0x55);
  sendByte(0x44);
  sendByte(address & 0xFF);
  sendByte(address >> 8);
  receiveByte();
  sendByte(data);
  receiveByte();
}

uint8_t UPDI::lds_b(uint16_t address)
{
  sendByte(0x55);
  sendByte(0x04);
  sendByte(address & 0xFF);
  sendByte(address >> 8);
  return receiveByte();
}

uint8_t UPDI::ld_b()
{
  sendByte(0x55);
  sendByte(0x20);
  return receiveByte();
}

void UPDI::stinc_b(uint8_t data)
{
  sendByte(0x55);
  sendByte(0x100);
  sendByte(data);
  receiveByte();
}

void UPDI::stinc_b_noget(uint8_t data)
{
  sendByte(0x55);
  sendByte(0x64);
  sendByte(data);
}

void UPDI::stinc_w_noget(uint16_t data)
{
  sendByte(0x55);
  sendByte(0x65);
  sendByte(data & 0xFF);
  sendByte((data >> 8) & 0xFF);
}


#endif


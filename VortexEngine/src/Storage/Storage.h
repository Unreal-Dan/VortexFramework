#ifndef STORAGE_H
#define STORAGE_H

#include <inttypes.h>

#ifdef VORTEX_LIB
#include <string>
#endif

class ByteStream;

class Storage
{
  Storage();
public:

  // init storage
  static bool init();
  static void cleanup();

  // store a serial buffer to storage
  static bool write(uint8_t slot, ByteStream &buffer);
  // read a serial buffer from storage
  static bool read(uint8_t slot, ByteStream &buffer);

  // the last save size (use STORAGE_SIZE For total space)
  static uint32_t lastSaveSize();

#ifdef VORTEX_LIB
  static void setStorageFilename(const std::string &name) { m_storageFilename = name; }
  static std::string getStorageFilename() { return m_storageFilename; }
#endif

private:
  // write out a single byte to the eeprom
  static void eepromWriteByte(uint16_t index, uint8_t in);
  static uint8_t eepromReadByte(uint16_t index);

#ifdef VORTEX_LIB
  static std::string m_storageFilename;
#endif

  // the size of the last save
  static uint32_t m_lastSaveSize;
};

#endif

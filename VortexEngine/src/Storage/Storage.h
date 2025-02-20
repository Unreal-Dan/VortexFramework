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

  // set the global storage page, the chromadeck has 8 pages of 16 slots each
  static void setStoragePage(uint8_t page);

  // store a serial buffer to storage
  static bool write(uint16_t slot, ByteStream &buffer);
  // read a serial buffer from storage
  static bool read(uint16_t slot, ByteStream &buffer);

  // the last save size (use STORAGE_SIZE For total space)
  static uint32_t lastSaveSize();

#ifdef VORTEX_LIB
  static void setStorageFilename(const std::string &name) { m_storageFilename = name; }
  static std::string getStorageFilename() { return m_storageFilename; }
#endif

private:
#ifdef VORTEX_LIB
  static std::string m_storageFilename;
#endif

  // the size of the last save
  static uint32_t m_lastSaveSize;

  // the curren storage page
  static uint8_t m_storagePage;
};

#endif

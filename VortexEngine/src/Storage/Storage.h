#ifndef STORAGE_H
#define STORAGE_H

#include <inttypes.h>

#ifdef VORTEX_LIB
#include <string>
#endif

class ByteStream;
class VortexEngine;

class Storage
{
public:
  Storage(VortexEngine &engine);
  ~Storage();

  // init storage
  bool init();
  void cleanup();

  // store a serial buffer to storage
  bool write(uint16_t slot, ByteStream &buffer);
  // read a serial buffer from storage
  bool read(uint16_t slot, ByteStream &buffer);

  // the last save size (use STORAGE_SIZE For total space)
  uint32_t lastSaveSize();

#ifdef VORTEX_LIB
  void setStorageFilename(const std::string &name) { m_storageFilename = name; }
  std::string getStorageFilename() { return m_storageFilename; }
#endif

private:
  // reference to engine
  VortexEngine &m_engine;

#ifdef VORTEX_LIB
  std::string m_storageFilename;
#endif

  // the size of the last save
  uint32_t m_lastSaveSize;
};

#endif

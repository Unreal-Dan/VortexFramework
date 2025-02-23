#ifndef BYTE_STREAM_H
#define BYTE_STREAM_H

#include <inttypes.h>
#include "../Log/Log.h"

// disable warning about using flexible array at end of structure
// otherwise visual studio dumps a million warnings about the buf[]
// flexible array member at the end of the inner data structure.
// Yes I know it's scary Visual Studio, please shut up now.
#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

class FlashClass;

class ByteStream
{
  friend class Storage;

public:
  ByteStream(uint32_t size = 0, const uint8_t *buf = nullptr);
  ~ByteStream();

  // copy and assignment operators
  ByteStream(const ByteStream &other);
  void operator=(const ByteStream &other);

  // move operator since std::move not always available
  void move(ByteStream *target);

  // used to initialize the full RawBuffer with a chunk of data,
  // this includes the size, flags, crc, and data
  bool rawInit(const uint8_t *rawdata, uint32_t size);

  // reset the buffer to a specific capcity
  bool init(uint32_t capacity = 0, const uint8_t *buf = nullptr);

  // clear the buffer
  void clear();

  // shrink capacity down to size, to free unused space
  bool shrink();

  // append another buffer
  bool append(const ByteStream &other);

  // extend the storage without changing the size of the data
  bool extend(uint32_t size);

  // trim the storage and erase some bytes from the end
  void trim(uint32_t bytes);

  // badly written in-place compression and decompression
  // which just uses a table of values then compresses the
  // data into indexes in the table packed into a buffer.
  // Unfortunately sometimes compression does not yield a
  // smaller buffer size, incases like this compress returns
  // true but the data isn't compressed. Similarly, if that
  // data is passed to decompress it will return true.
  bool compress();
  bool decompress();

  // re-calculate the built-in CRC on the bytestream if needed
  // if the crc is up to date this will do nothing. Optionally
  // force a re-calculation even if the dirty flag is missing.
  // Will return the new CRC value.
  uint32_t recalcCRC(bool force = false);

  // helper func to ensure the internal buffer is sane, use this
  // after reading into the rawdata of the buffer
  void sanity();
  // check the CRC without re-calculating, note, if the CRC is
  // dirty then this will simply return false
  bool checkCRC() const;
  // check whether the data in the buffer has changed since the
  // crc has been re-calculated, if it is dirty, call recalcCRC
  bool isCRCDirty() const;
  // mark the CRC as dirty (manually modified the buffer or something)
  void setCRCDirty();

  // serialize a byte into the buffer
  bool serialize8(uint8_t byte);
  bool serialize16(uint16_t bytes);
  bool serialize32(uint32_t bytes);

  // reset the unserializer index
  void resetUnserializer();
  // move the unserializer index manually
  void moveUnserializer(uint32_t idx);
  // check if the unserializer is at the end
  bool unserializerAtEnd() const;

  // unserialize data out of the buffer
  bool unserialize8(uint8_t *byte);
  bool unserialize16(uint16_t *bytes);
  bool unserialize32(uint32_t *bytes);

  // unserialize data and erase it from the buffer
  bool consume8(uint8_t *byte = nullptr);
  bool consume16(uint16_t *bytes = nullptr);
  bool consume32(uint32_t *bytes = nullptr);

  // unserialize and consume a whole chunk
  bool consume(uint32_t size, void *bytes = nullptr);

  uint8_t peek8() const;
  uint16_t peek16() const;
  uint32_t peek32() const;

  // overload [] for array access (no bounds check lol)
  uint8_t &operator[](uint32_t index) { return m_pData->buf[index]; }
  // overload (uint8_t *) for cast to buffer
  operator uint8_t *() const { return m_pData ? m_pData->buf : nullptr; }
  operator const uint8_t *() const { return m_pData ? m_pData->buf : nullptr; }

  // return the members
  const uint8_t *data() const { return m_pData ? m_pData->buf : nullptr; }
  void *rawData() const { return m_pData; }
  uint32_t rawSize() const { return m_pData ? m_pData->size + sizeof(RawBuffer) : 0; }
  uint32_t size() const { return m_pData ? m_pData->size : 0; }
  uint32_t capacity() const { return m_capacity; }
  bool is_compressed() const;
  uint32_t CRC() const { return m_pData ? m_pData->crc32 : 0; }
  
  uint8_t *frontUnserializer() const { return m_pData ? m_pData->buf + m_position : nullptr; }

private:
  // fetch pointers that walk the buffer to serialize/unserialize
  //uint8_t *frontUnserializer() const { return m_pData ? m_pData->buf + m_position : nullptr; }
  uint8_t *frontSerializer() const { return m_pData ? m_pData->buf + m_pData->size : nullptr; }
  // check if the buffer is large enough for the amount
  bool largeEnough(uint32_t amount) const;
  // helper to get width of bits that makes up value
  uint32_t getWidth(uint32_t value) const;

  // the structure of raw data that's written to storage
  struct RawBuffer
  {
    RawBuffer() : size(0), flags(0), crc32(5381) {}
    // hash the raw buffer into crc
    uint32_t hash() const
    {
      uint32_t hash = 5381;
      for (uint32_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + buf[i];
      }
      return hash;
    }
    // accumulate the data into the hash
    void accumulate(uint32_t val)
    {
      crc32 = ((crc32 << 5) + crc32) + val;
    }
    // veryify the crc
    bool verify() const
    {
      // if the buffer is empty then 'verify' should just return true
      if (!size) {
        return true;
      }
      if (hash() != crc32) {
        DEBUG_LOGF("CRC mismatch: %x should be %x", hash(), crc32);
        return false;
      }
      return true;
    }
    // re-calculate the crc
    void recalcCRC()
    {
      crc32 = hash();
    }
    // the size of the buf of data
    uint32_t size;
    // any special flags about the data (compression etc)
    uint32_t flags;
    // the crc32 for the data
    uint32_t crc32;
    // the start of the buf of data
    uint8_t buf[];
  };

  // The raw buffer of data along with size and flags
  RawBuffer *m_pData;
  // the index in the raw buffer for unserialization
  uint16_t m_position;
  // the actual size of the buffer raw buffer
  uint16_t m_capacity;
};

#endif

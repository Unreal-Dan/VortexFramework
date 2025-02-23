#include "ByteStream.h"

#include "../Serial/BitStream.h"
#include "../Memory/Memory.h"
#include "../Log/Log.h"

#include <string.h>

#include "Compression.h"

// flags for saving buffer to disk
#define BUFFER_FLAG_COMRPESSED (1<<0) // buffer is compressed
#define BUFFER_FLAG_DIRTY      (1<<1) // buffer crc is dirty

ByteStream::ByteStream(uint32_t size, const uint8_t *buf) :
  m_pData(),
  m_position(0),
  m_capacity(0)
{
  init(size, buf);
}

ByteStream::~ByteStream()
{
  clear();
}

ByteStream::ByteStream(const ByteStream &other) :
  ByteStream()
{
  init(other.capacity(), other.data());
  m_pData->flags = other.m_pData->flags;
  m_pData->crc32 = other.m_pData->crc32;
  m_pData->size = other.m_pData->size;
}

void ByteStream::operator=(const ByteStream &other)
{
  init(other.capacity(), other.data());
  m_pData->flags = other.m_pData->flags;
  m_pData->crc32 = other.m_pData->crc32;
  m_pData->size = other.m_pData->size;
}

void ByteStream::move(ByteStream *target)
{
  if (!target) {
    return;
  }
  target->clear();
  target->m_pData = m_pData;
  target->m_position = m_position;
  target->m_capacity = m_capacity;
  m_pData = nullptr;
  m_position = 0;
  m_capacity = 0;
}

bool ByteStream::rawInit(const uint8_t *rawdata, uint32_t size)
{
  if (!rawdata || size < sizeof(RawBuffer)) {
    DEBUG_LOGF("Cannot rawInit: %p %u", rawdata, size);
    return false;
  }
  // round up to nearest 4
  m_capacity = (size + 4) - (size % 4);
  m_pData = (RawBuffer *)vcalloc(1, m_capacity + sizeof(RawBuffer));
  if (!m_pData) {
    m_capacity = 0;
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  // copy in the actual data from the serial buffer, this will fill the
  // members: size, flags, and crc32 of m_pData as well as the buf
  m_pData->size = ((RawBuffer *)rawdata)->size;
  m_pData->flags = ((RawBuffer *)rawdata)->flags;
  m_pData->crc32 = ((RawBuffer *)rawdata)->crc32;
  // ensure the internal size doesn't exceed outer size
  sanity();
  memcpy(m_pData->buf, ((RawBuffer *)rawdata)->buf, m_pData->size);
  return true;
}

// reset the buffer
bool ByteStream::init(uint32_t size, const uint8_t *buf)
{
  clear();
  if (size) {
    // round up to nearest 4
    m_capacity = (size + 4) - (size % 4);
    m_pData = (RawBuffer *)vcalloc(1, m_capacity + sizeof(RawBuffer));
    if (!m_pData) {
      m_capacity = 0;
      ERROR_OUT_OF_MEMORY();
      return false;
    }
    m_pData->size = 0;
    m_pData->flags = 0;
    m_pData->crc32 = 0;
    memset(m_pData->buf, 0, m_capacity);
  }
  if (buf && m_pData) {
    memcpy(m_pData->buf, buf, size);
    m_pData->size = size;
    m_pData->recalcCRC();
  }
  return true;
}

void ByteStream::clear()
{
  if (m_pData) {
    vfree(m_pData);
    m_pData = nullptr;
  }
  m_capacity = 0;
  m_position = 0;
}

bool ByteStream::shrink()
{
  if (!m_pData) {
    return false;
  }
  if (m_pData->size == m_capacity) {
    return true;
  }
  m_capacity = m_pData->size;
  RawBuffer *temp = (RawBuffer *)vrealloc(m_pData, m_pData->size + sizeof(RawBuffer));
  if (!temp) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  m_pData = temp;
  return true;
}

// append another buffer
bool ByteStream::append(const ByteStream &other)
{
  if (!other.m_pData) {
    // nothing to append
    return true;
  }
  if (!m_pData || other.size() > (m_capacity - size())) {
    if (!extend(other.size())) {
      return false;
    }
  }
  memcpy(frontSerializer(), other.data(), other.size());
  m_pData->size += other.size();
  return true;
}

// trim some amount of bytes off the end of the stream
void ByteStream::trim(uint32_t bytes)
{
  if (!m_pData) {
    return;
  }
  // just adjust the data size no need to reallocate, this
  // will put the extra bytes into 'extra space' and shrink()
  // could be called to reallocate the buffer without them
  if (m_pData->size <= bytes) {
    m_pData->size = 0;
  } else {
    m_pData->size -= bytes;
  }
  // move the unserializer back if necessary
  if (m_position >= m_pData->size) {
    m_position = m_pData->size;
  }
}

// extend the storage without changing the size of the data
bool ByteStream::extend(uint32_t size)
{
  if (!size) {
    // ???
    return true;
  }
  // if the buffer is empty just initialize a new buffer
  if (!m_capacity) {
    return init(size);
  }
  // new size is current plus extension amount
  uint32_t buffer_size = m_capacity + size + 3;
  // round size up to nearest 4 and add the size of the base raw buffer
  buffer_size -= buffer_size % 4;
  // the size of the actual new block of memory
  uint32_t new_size = buffer_size + sizeof(RawBuffer);
  RawBuffer *temp = (RawBuffer *)vrealloc(m_pData, new_size);
  if (!temp) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  // calc the region that was newly allocated by realloc
  uint8_t *new_mem = ((uint8_t *)temp) + sizeof(RawBuffer) + m_capacity;
  size_t new_mem_size = buffer_size - m_capacity;
  // zero out the new memory
  memset(new_mem, 0, new_mem_size);
  // update the pointer and size of the serial buffer
  m_pData = temp;
  m_capacity = buffer_size;
  return true;
}

bool ByteStream::compress()
{
  // only compress if we have valid data
  if (!m_pData) {
    DEBUG_LOG("No data to compress");
    return false;
  }
  // recalculate the CRC even if we don't compress, this ensures the
  // CRC is always up to date as long as compress() is called
  recalcCRC();
  // don't actually perform any compression if VORTEX SLIM is enabled
#if VORTEX_SLIM == 0
  // check to see if the buffer is already compressed
  if (is_compressed()) {
    // already compressed
    return true;
  }
  // use LZ4_compressBound for the output buffer size, it may be larger
  // but it will allow for faster compression then we can shrink it after
  ByteStream compressedBuffer(compress_size(m_pData->size));
  // compress the data
  int compressedSize = compress_buffer(m_pData->buf, compressedBuffer.m_pData->buf,
    m_pData->size, compressedBuffer.m_capacity);
  // check for compression error
  if (compressedSize < 0) {
    DEBUG_LOGF("Failed to compress, error: %d", compressedSize);
    return false;
  }
  // check for useless compression
  if (!compressedSize || (uint32_t)compressedSize >= m_pData->size) {
    // cannot compress, or compressed no smaller than original
    return true;
  }
  // update the buffer size
  compressedBuffer.m_pData->size = compressedSize;
  // buffer is now compressed
  compressedBuffer.m_pData->flags = (m_pData->flags | BUFFER_FLAG_COMRPESSED);
  // recalc the crc on the data buffer
  compressedBuffer.m_pData->recalcCRC();
  // shrink buffer capacity to match size
  compressedBuffer.shrink();
  // move into self
  compressedBuffer.move(this);
#endif
  return true;
}

bool ByteStream::decompress()
{
  // ensure the internal buffer is sane after reading it out, this
  // prevents segfaults if the internal size reports larger than capacity
  sanity();
  // only decompress if we have valid data
  if (!m_pData || !m_pData->verify()) {
    DEBUG_LOG("Cannot verify crc, not decompressing");
    return false;
  }
  // don't actually perform any compression if VORTEX SLIM is enabled
#if VORTEX_SLIM == 0
  if (!is_compressed()) {
    // already decompressed
    return true;
  }
  // first multiple will be 2x
  uint32_t multiple = 1;
  int decompressedSize = 0;
  ByteStream decompressedBuffer;
  do {
    // double the amount each time
    multiple *= 2;
    // create buffer to receive decompressed data
    decompressedBuffer.init(m_pData->size * multiple);
    // decompress the data
    decompressedSize = decompress_buffer(m_pData->buf, decompressedBuffer.m_pData->buf,
      m_pData->size, decompressedBuffer.m_capacity);
  } while (decompressedSize < 0 && multiple < 512);
  // still error
  if (decompressedSize < 0) {
    ERROR_LOGF("Failed to decompress: %d", decompressedSize);
    return false;
  }
  // size changed
  decompressedBuffer.m_pData->size = decompressedSize;
  // data is no longer compressed
  decompressedBuffer.m_pData->flags = (m_pData->flags & ~BUFFER_FLAG_COMRPESSED);
  // recalc crc of buffer
  decompressedBuffer.m_pData->recalcCRC();
  DEBUG_LOGF("Decompressed %u to %u bytes", m_pData->size, decompressedBuffer.m_pData->size);
  // shrink buffer capacity to match size
  decompressedBuffer.shrink();
  // move into self
  decompressedBuffer.move(this);
#endif
  return true;
}

uint32_t ByteStream::recalcCRC(bool force)
{
  if (!m_pData || !m_pData->size) {
    return 0;
  }
  // is the crc dirty?
  // or is the recalc being forced anyway?
  if (!force && !(m_pData->flags & BUFFER_FLAG_DIRTY)) {
    return 0;
  }
  // re-calculate the CRC on the buffer
  m_pData->recalcCRC();
  // unset dirty flag
  m_pData->flags &= ~BUFFER_FLAG_DIRTY;
  // return the newly calculated crc
  return m_pData->crc32;
}

void ByteStream::sanity()
{
  // to ensure size never exceeds the buffer capacity
  if (m_pData && m_pData->size > m_capacity) {
    m_pData->size = m_capacity;
  }
}

bool ByteStream::checkCRC() const
{
  // don't check if dirty
  if (!m_pData || isCRCDirty()) {
    return false;
  }
  return m_pData->verify();
}

bool ByteStream::isCRCDirty() const
{
  return (m_pData && (m_pData->flags & BUFFER_FLAG_DIRTY) != 0);
}

void ByteStream::setCRCDirty()
{
  if (!m_pData) {
    return;
  }
  // set the dirty flag
  m_pData->flags |= BUFFER_FLAG_DIRTY;
}

bool ByteStream::serialize8(uint8_t byte)
{
  //DEBUG_LOGF("Serialize8(): %u", byte);
  if (!m_pData || (m_pData->size + sizeof(uint8_t)) > m_capacity) {
    if (!extend(sizeof(uint32_t))) {
      return false;
    }
  }
  memcpy(m_pData->buf + m_pData->size, &byte, sizeof(uint8_t));
  // walk forward
  m_pData->size += sizeof(uint8_t);
  // dirty the crc
  setCRCDirty();
  return true;
}

bool ByteStream::serialize16(uint16_t bytes)
{
  //DEBUG_LOGF("Serialize16(): %u", bytes);
  if (!m_pData || (m_pData->size + sizeof(uint16_t)) > m_capacity) {
    if (!extend(sizeof(uint32_t))) {
      return false;
    }
  }
  memcpy(m_pData->buf + m_pData->size, &bytes, sizeof(uint16_t));
  // walk forward
  m_pData->size += sizeof(uint16_t);
  // dirty the crc
  setCRCDirty();
  return true;
}

bool ByteStream::serialize32(uint32_t bytes)
{
  //DEBUG_LOGF("Serialize32(): %u", bytes);
  if (!m_pData || (m_pData->size + sizeof(uint32_t)) > m_capacity) {
    if (!extend(sizeof(uint32_t))) {
      return false;
    }
  }
  memcpy(m_pData->buf + m_pData->size, &bytes, sizeof(uint32_t));
  // walk forward
  m_pData->size += sizeof(uint32_t);
  // dirty the crc
  setCRCDirty();
  return true;
}

// reset the unserializer index so that unserialization will
// begin from the start of the buffer
void ByteStream::resetUnserializer()
{
  moveUnserializer(0);
}

// move the unserializer index manually
void ByteStream::moveUnserializer(uint32_t idx)
{
  if (!m_pData) {
    return;
  }
  if (idx >= m_pData->size) {
    idx = 0;
  }
  m_position = idx;
}

bool ByteStream::unserializerAtEnd() const
{
  // the serializer always points to the end of the buffer,
  // if the unserializer points there too then there's nothing
  // more for the unserializer to read
  return m_pData && m_position == m_pData->size;
}

// unserialize data and walk the buffer that many bytes
bool ByteStream::unserialize8(uint8_t *byte)
{
  if (!m_pData || m_position >= m_pData->size || (m_pData->size - m_position) < sizeof(uint8_t)) {
    return false;
  }
  memcpy(byte, m_pData->buf + m_position, sizeof(uint8_t));
  //DEBUG_LOGF("Unserialize8(): %u", *byte);
  m_position += sizeof(uint8_t);
  return true;
}

bool ByteStream::unserialize16(uint16_t *bytes)
{
  if (!m_pData || m_position >= m_pData->size || (m_pData->size - m_position) < sizeof(uint16_t)) {
    return false;
  }
  memcpy(bytes, m_pData->buf + m_position, sizeof(uint16_t));
  //DEBUG_LOGF("Unserialize16(): %u", *bytes);
  m_position += sizeof(uint16_t);
  return true;
}

bool ByteStream::unserialize32(uint32_t *bytes)
{
  if (!m_pData || m_position >= m_pData->size || (m_pData->size - m_position) < sizeof(uint32_t)) {
    return false;
  }
  memcpy(bytes, m_pData->buf + m_position, sizeof(uint32_t));
  //DEBUG_LOGF("Unserialize32(): %u", *bytes);
  m_position += sizeof(uint32_t);
  return true;
}

// unserialize data and erase it from the buffer
bool ByteStream::consume8(uint8_t *bytes)
{
  if (!m_pData || m_pData->size < sizeof(uint8_t)) {
    return false;
  }
  if (bytes) {
    // copy out the data
    memcpy(bytes, m_pData->buf, sizeof(uint8_t));
  }
  // adjust the size
  m_pData->size -= sizeof(uint8_t);
  // shift the data
  memmove(m_pData->buf, m_pData->buf + sizeof(uint8_t), m_pData->size);
  // dirty the crc
  setCRCDirty();
  return true;
}

bool ByteStream::consume16(uint16_t *bytes)
{
  if (!m_pData || m_pData->size < sizeof(uint16_t)) {
    return false;
  }
  if (bytes) {
    // copy out the data
    memcpy(bytes, m_pData->buf, sizeof(uint16_t));
  }
  // adjust the size
  m_pData->size -= sizeof(uint16_t);
  // shift the data
  memmove(m_pData->buf, m_pData->buf + sizeof(uint16_t), m_pData->size);
  // dirty the crc
  setCRCDirty();
  return true;
}

bool ByteStream::consume32(uint32_t *bytes)
{
  if (!m_pData || m_pData->size < sizeof(uint32_t)) {
    return false;
  }
  if (bytes) {
    // copy out the data
    memcpy(bytes, m_pData->buf, sizeof(uint32_t));
  }
  // adjust the size
  m_pData->size -= sizeof(uint32_t);
  // shift the data
  memmove(m_pData->buf, m_pData->buf + sizeof(uint32_t), m_pData->size);
  // dirty the crc
  setCRCDirty();
  return true;
}

bool ByteStream::consume(uint32_t size, void *bytes)
{
  if (!m_pData || m_position >= m_pData->size || (m_pData->size - m_position) < size) {
    return false;
  }
  if (bytes) {
    // copy out the data
    memcpy(bytes, m_pData->buf, size);
  }
  // adjust the size
  m_pData->size -= size;
  // shift the data
  memmove(m_pData->buf, m_pData->buf + size, m_pData->size);
  // dirty the crc
  setCRCDirty();
  return true;
}

uint8_t ByteStream::peek8() const
{
  if (!m_pData) {
    return 0;
  }
  return *(uint8_t *)frontUnserializer();
}

uint16_t ByteStream::peek16() const
{
  if (!m_pData) {
    return 0;
  }
  return *(uint16_t *)frontUnserializer();
}

uint32_t ByteStream::peek32() const
{
  if (!m_pData) {
    return 0;
  }
  return *(uint32_t *)frontUnserializer();
}

bool ByteStream::is_compressed() const
{
  if (!m_pData) {
    return false;
  }
  return (m_pData->flags & BUFFER_FLAG_COMRPESSED) != 0;
}

bool ByteStream::largeEnough(uint32_t amount) const
{
  if (!m_pData) {
    return false;
  }
  return ((m_pData->size + amount) <= m_capacity);
}

// get width of bits that makes up value
// ex: 3 -> 2 bits wide (11)
//     9 -> 4 bits wide (1001)
uint32_t ByteStream::getWidth(uint32_t value) const
{
  if (!value) {
    return 0;
  }
  for (uint32_t i = 0; i < 31; ++i) {
    if (value < (uint32_t)(1 << i)) {
      return i;
    }
  }
  return 0;
}
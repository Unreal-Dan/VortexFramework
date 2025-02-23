#include "Serial.h"

#include "../Serial/ByteStream.h"
#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Log/Log.h"

#include "../VortexEngine.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#include <stdio.h>
#endif

SerialComs::SerialComs(VortexEngine &engine) :
  m_engine(engine),
  m_serialConnected(false),
  m_lastCheck(0)
{
}

SerialComs::~SerialComs()
{
}

// init serial
bool SerialComs::init()
{
  // Try connecting serial ?
  //checkSerial();
  return true;
}

void SerialComs::cleanup()
{
}

bool SerialComs::isConnected()
{
  return m_serialConnected;
}

// check for any serial connection or messages
bool SerialComs::checkSerial()
{
#if VORTEX_SLIM == 0
  if (isConnected()) {
    // already connected
    return true;
  }
  uint32_t now = m_engine.time().getCurtime();
  // don't check for serial too fast
  if (m_lastCheck && (now - m_lastCheck) < MAX_SERIAL_CHECK_INTERVAL) {
    // can't check yet too soon
    return false;
  }
  m_lastCheck = now;
#ifdef VORTEX_LIB
  if (!m_engine.vortexLib().vcallbacks()->serialCheck()) {
    return false;
  }
  m_engine.vortexLib().vcallbacks()->serialBegin(SERIAL_BAUD_RATE);
#else
  // This will check if the serial communication is open
  if (!Serial.available()) {
    // serial is not connected
    return false;
  }
  // Begin serial communications
  Serial.begin(SERIAL_BAUD_RATE);
#endif
#endif
  // serial is now connected
  m_serialConnected = true;
  return true;
}

void SerialComs::write(const char *msg, ...)
{
#if VORTEX_SLIM == 0
  if (!isConnected()) {
    return;
  }
  va_list list;
  va_start(list, msg);
  uint8_t buf[2048] = {0};
  int len = vsnprintf((char *)buf, sizeof(buf), msg, list);
#ifdef VORTEX_LIB
  m_engine.vortexLib().vcallbacks()->serialWrite(buf, len);
#else
  Serial.write(buf, len);
  Serial.flush();
#endif
  va_end(list);
#endif
}

void SerialComs::write(ByteStream &byteStream)
{
#if VORTEX_SLIM == 0
  if (!isConnected()) {
    return;
  }
  byteStream.recalcCRC();
  uint32_t size = byteStream.rawSize();
#ifdef VORTEX_LIB
  m_engine.vortexLib().vcallbacks()->serialWrite((const uint8_t *)&size, sizeof(size));
  m_engine.vortexLib().vcallbacks()->serialWrite((const uint8_t *)byteStream.rawData(), byteStream.rawSize());
#else
  Serial.write((const uint8_t *)&size, sizeof(size));
  Serial.write((const uint8_t *)byteStream.rawData(), byteStream.rawSize());
  Serial.flush();
#endif
#endif
}

void SerialComs::read(ByteStream &byteStream)
{
#if VORTEX_SLIM == 0
  if (!isConnected()) {
    return;
  }
#ifdef VORTEX_LIB
  uint32_t amt = m_engine.vortexLib().vcallbacks()->serialAvail();
#else
  uint32_t amt = Serial.available();
#endif
  if (!amt) {
    return;
  }
  do {
    uint8_t byte = 0;
#ifdef VORTEX_LIB
    if (!m_engine.vortexLib().vcallbacks()->serialRead((char *)&byte, 1)) {
      return;
    }
#else
    byte = Serial.read();
#endif
    byteStream.serialize8(byte);
  } while (--amt > 0);
#endif
}

bool SerialComs::dataReady()
{
#if VORTEX_SLIM == 0
  if (!isConnected()) {
    return false;
  }
#ifdef VORTEX_LIB
  return (m_engine.vortexLib().vcallbacks()->serialAvail() > 0);
#else
  return (Serial.available() > 0);
#endif
#else
  return false;
#endif
}

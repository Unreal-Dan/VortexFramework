#ifndef SERIAL_H
#define SERIAL_H

#include <inttypes.h>

class ByteStream;
class VortexEngine;

// Really wish I could name this Serial but arduino ruined that for me
class SerialComs
{
public:
  SerialComs(VortexEngine &engine);
  ~SerialComs();

  // init serial
  bool init();
  void cleanup();

  // whether serial is initialized
  bool isConnected();

  // check for any serial connection or messages
  bool checkSerial();

  // write a message to serial
  void write(const char *msg, ...);

  // write raw data
  void write(ByteStream &byteStream);

  // read a message from serial
  void read(ByteStream &byteStream);

  // data in the socket ready to read
  bool dataReady();

private:
  // reference to engine parent
  VortexEngine &m_engine;
  // whether serial communications are initialized
  bool m_serialConnected;
  uint32_t m_lastCheck;
};

#endif

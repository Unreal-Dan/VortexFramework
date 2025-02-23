#ifndef SERIAL_H
#define SERIAL_H

#include <inttypes.h>

class ByteStream;

// Really wish I could name this Serial but arduino ruined that for me
class SerialComs
{
public:

  // init serial
  static bool init();
  static void cleanup();

  // whether serial is initialized
  static bool isConnected();

  // why do I need this
  static bool isConnectedReal();

  // check for any serial connection or messages
  static bool checkSerial();

  // write a message to serial
  static void write(const char *msg, ...);

  // write raw data
  static void write(ByteStream &byteStream);

  // read a message from serial
  static void read(ByteStream &byteStream);

  // data in the socket ready to read
  static bool dataReady();

private:
  // whether serial communications are initialized
  static bool m_serialConnected;
  static uint32_t m_lastCheck;
  static uint32_t m_lastConnected;
};

#endif

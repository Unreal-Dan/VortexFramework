#ifndef IR_SENDER_H
#define IR_SENDER_H

#include "../Serial/ByteStream.h"
#include "../Serial/BitStream.h"

#include "IRConfig.h"

#if IR_ENABLE_SENDER == 1

class VortexEngine;
class Mode;

class IRSender
{
public:
  IRSender(VortexEngine &engine);
  ~IRSender();

  bool init();
  void cleanup();

  // initialize the IR sender with a mode to send
  bool loadMode(const Mode *targetMode);
  bool send();

  bool isSending() { return m_isSending; }

  uint32_t percentDone() { return (uint32_t)(((float)m_writeCounter / (float)m_size) * 100.0); }

private:
  // sender functions
  void beginSend();
  // send a full 8 bits in a tight loop
  void sendByte(uint8_t data);
  // send a mark/space by turning PWM on/off
  void sendMark(uint16_t time);
  void sendSpace(uint16_t time);

  // reference to engine
  VortexEngine &m_engine;

  // the serial buffer for the data
  ByteStream m_serialBuf;
  // a bit walker for the serial data
  BitStream m_bitStream;
  bool m_isSending;
  uint32_t m_lastSendTime;

  // some runtime meta info
  uint32_t m_size;
  // the number of blocks that will be sent
  uint8_t m_numBlocks;
  // the amount in the final block
  uint8_t m_remainder;

  // configuration options for the sender
  uint32_t m_blockSize;

  // write total
  uint32_t m_writeCounter;
};

#endif

#endif

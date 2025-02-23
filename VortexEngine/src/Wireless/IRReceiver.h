#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <inttypes.h>

#include "../Serial/BitStream.h"

#include "IRConfig.h"

#if IR_ENABLE_RECEIVER == 1

class VortexEngine;
class ByteStream;
class Mode;

class IRReceiver
{
public:
  IRReceiver(VortexEngine &engine);
  ~IRReceiver();

  // init and cleanup the receiver
  bool init();
  void cleanup();

  // check whether a full IR message is ready to read
  bool dataReady();
  // whether actively receiving
  bool isReceiving();
  // the percent of data received
  uint8_t percentReceived();
  uint16_t bytesReceived() { return m_irData.bytepos(); }

  // receive the IR message into a target mode
  bool receiveMode(Mode *pMode);

  // turn the receiver on/off
  bool beginReceiving();
  bool endReceiving();
  // checks if there is new data since the last reset or check
  bool onNewData();
  // reset IR receiver buffer
  void resetIRState();

private:
  // reference to engine
  VortexEngine &m_engine;

  // reading functions
  // PCI handler for when IR receiver pin changes states
  bool read(ByteStream &data);
  void recvPCIHandler();
  void handleIRTiming(uint32_t diff);

  // ===================
  //  private data:

  // BitStream object that IR data is fed to bit by bit
  BitStream m_irData;

  // Receive state used for state machine in PCIhandler
  enum RecvState : uint8_t
  {
    WAITING_HEADER_MARK,
    WAITING_HEADER_SPACE,
    READING_DATA_MARK,
    READING_DATA_SPACE
  };

  // state information used by the PCIHandler
  RecvState m_recvState;
  // used to track pin changes
  uint32_t m_prevTime;
  uint8_t m_pinState;

  // used to compare if received data has changed since last checking
  uint32_t m_previousBytes;

#ifdef VORTEX_LIB
  friend class Vortex;
#endif
};

#endif

#endif

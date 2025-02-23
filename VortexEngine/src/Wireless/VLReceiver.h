#ifndef VL_RECEIVER_H
#define VL_RECEIVER_H

#include <inttypes.h>

#include "../Serial/BitStream.h"

#include "VLConfig.h"

#if VL_ENABLE_RECEIVER == 1

#ifdef VORTEX_EMBEDDED
#include <Arduino.h>
#endif

class ByteStream;
class Mode;

class VLReceiver
{
  VLReceiver();

public:
  // init and cleanup the receiver
  static bool init();
  static void cleanup();

  // check whether a full VL message is ready to read
  static bool dataReady();
  // whether actively receiving
  static bool isReceiving();
  // the percent of data received
  static uint8_t percentReceived();
  static uint16_t bytesReceived() { return m_vlData.bytepos(); }

  // receive the VL message into a target mode
  static bool receiveMode(Mode *pMode);

  // turn the receiver on/off
  static bool beginReceiving();
  static bool endReceiving();
  // checks if there is new data since the last reset or check
  static bool onNewData();
  // reset VL receiver buffer
  static void resetVLState();

  static void recvPCIHandler();

private:

  // reading functions
  // PCI handler for when VL receiver pin changes states
  static bool read(ByteStream &data);
  static void handleVLTiming(uint32_t diff);

  // ===================
  //  private data:

  // BitStream object that VL data is fed to bit by bit
  static BitStream m_vlData;

  // Receive state used for state machine in PCIhandler
  enum RecvState : uint8_t
  {
    WAITING_HEADER_MARK,
    WAITING_HEADER_SPACE,
    READING_DATA_MARK,
    READING_DATA_SPACE
  };

  // state information used by the PCIHandler
  static RecvState m_recvState;
  // used to track pin changes
  static uint32_t m_prevTime;
  static uint8_t m_pinState;

  // used to compare if received data has changed since last checking
  static uint32_t m_previousBytes;

#ifdef VORTEX_EMBEDDED
  static void adcCheckTimerCallback(void *arg);
#endif

#ifdef VORTEX_LIB
  friend class Vortex;
#endif
};

#endif

#endif

#include "VLReceiver.h"
#include "IRConfig.h"

#if VL_ENABLE_RECEIVER == 1

#include "../VortexEngine.h"

#include "../Serial/ByteStream.h"
#include "../Serial/BitStream.h"
#include "../Time/TimeControl.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

VLReceiver::VLReceiver(VortexEngine &engine) :
  m_engine(engine),
  m_vlData(),
  m_recvState(WAITING_HEADER_MARK),
  m_prevTime(0),
  m_pinState(0),
  m_previousBytes(0)
{
}

VLReceiver::~VLReceiver()
{
}

bool VLReceiver::init()
{
  return m_vlData.init(VL_RECV_BUF_SIZE);
}

void VLReceiver::cleanup()
{
}

bool VLReceiver::dataReady()
{
  // is the receiver actually receiving data?
  if (!isReceiving()) {
    return false;
  }
  uint8_t blocks = m_vlData.data()[0];
  uint8_t remainder = m_vlData.data()[1];
  uint32_t total = ((blocks - 1) * 32) + remainder;
  if (!total || total > VL_MAX_DATA_TRANSFER) {
    DEBUG_LOGF("Bad VL Data size: %u", total);
    return false;
  }
  // if there are size + 2 bytes in the VLData receiver
  // then a full message is ready, the + 2 is from the
  // two bytes for blocks + remainder that are sent first
  return (m_vlData.bytepos() >= (uint32_t)(total + 2));
}

// whether actively receiving
bool VLReceiver::isReceiving()
{
  // if there are at least 2 bytes in the data buffer then
  // the receiver is receiving a packet. If there is less
  // than 2 bytes then we're still waiting for the 'blocks'
  // and 'remainder' bytes which prefix a packet
  return (m_vlData.bytepos() > 2);
}

// the percent of data received
uint8_t VLReceiver::percentReceived()
{
  if (!isReceiving()) {
    return 0;
  }
  uint8_t blocks = m_vlData.data()[0];
  uint8_t remainder = m_vlData.data()[1];
  uint16_t total = ((blocks - 1) * 32) + remainder;
  // round by adding half of the total to the numerator
  return (uint8_t)((uint16_t)((m_vlData.bytepos() * 100 + (total / 2)) / total));
}

bool VLReceiver::receiveMode(Mode *pMode)
{
  ByteStream buf;
  // read from the receive buffer into the byte stream
  if (!read(buf)) {
    // no data to read right now, or an error
    DEBUG_LOG("No data available to read, or error reading");
    return false;
  }
  DEBUG_LOGF("Received %u bytes", buf.rawSize());
  // load the data into the target mode
  return pMode->loadFromBuffer(buf);
}

bool VLReceiver::beginReceiving()
{
  resetVLState();
  return true;
}

bool VLReceiver::endReceiving()
{
  resetVLState();
  return true;
}

bool VLReceiver::onNewData()
{
  if (bytesReceived() == m_previousBytes) {
    return false;
  }
  m_previousBytes = bytesReceived();
  return true;
}

bool VLReceiver::read(ByteStream &data)
{
  if (!m_vlData.bytepos() || m_vlData.bytepos() > VL_MAX_DATA_TRANSFER) {
    DEBUG_LOG("Nothing to read, or read too much");
    return false;
  }
  // read the size out (blocks + remainder)
  uint8_t blocks = m_vlData.data()[0];
  uint8_t remainder = m_vlData.data()[1];
  // calculate size from blocks + remainder
  uint32_t size = ((blocks - 1) * 32) + remainder;
  if (!size || size > VL_MAX_DATA_TRANSFER) {
    DEBUG_LOGF("Bad VL Data size: %u", size);
    return false;
  }
  // the actual data starts 2 bytes later because of the size byte
  const uint8_t *actualData = m_vlData.data() + 2;
  if (!data.rawInit(actualData, size)) {
    DEBUG_LOG("Failed to init buffer for VL read");
    return false;
  }
  // reset the VL state and receive buffer now
  resetVLState();
  return true;
}

// The recv PCI handler is called every time the pin state changes
void VLReceiver::recvPCIHandler()
{
  // toggle the tracked pin state no matter what
  m_pinState = (uint8_t)!m_pinState;
  // grab current time
  uint32_t now = m_engine.time().microseconds();
  // check previous time for validity
  if (!m_prevTime || m_prevTime > now) {
    m_prevTime = now;
    DEBUG_LOG("Bad first time diff, resetting...");
    resetVLState();
    return;
  }
  // calc time difference between previous change and now
  uint32_t diff = (uint32_t)(now - m_prevTime);
  // and update the previous changetime for next loop
  m_prevTime = now;
  // handle the bliank duration and process it
  handleVLTiming(diff);
}

// state machine that can be fed VL timings to parse them and interpret the intervals
void VLReceiver::handleVLTiming(uint32_t diff)
{
  // if the diff is too long or too short then it's not useful
  if ((diff > VL_HEADER_MARK_MAX && m_recvState < READING_DATA_MARK) || diff < VL_TIMING_MIN) {
    DEBUG_LOGF("bad delay: %u, resetting...", diff);
    resetVLState();
    return;
  }
  switch (m_recvState) {
  case WAITING_HEADER_MARK: // initial state
    if (diff >= VL_HEADER_SPACE_MIN && diff <= VL_HEADER_MARK_MAX) {
      m_recvState = WAITING_HEADER_SPACE;
    } else {
      DEBUG_LOGF("Bad header mark %u, resetting...", diff);
      resetVLState();
    }
    break;
  case WAITING_HEADER_SPACE:
    if (diff >= VL_HEADER_SPACE_MIN && diff <= VL_HEADER_MARK_MAX) {
      m_recvState = READING_DATA_MARK;
    } else {
      DEBUG_LOGF("Bad header space %u, resetting...", diff);
      resetVLState();
    }
    break;
  case READING_DATA_MARK:
    // classify mark/space based on the timing and write into buffer
    m_vlData.write1Bit((diff > (VL_TIMING * 2)) ? 1 : 0);
    m_recvState = READING_DATA_SPACE;
    break;
  case READING_DATA_SPACE:
    // the space could be just a regular space, or a gap in between blocks
    m_recvState = READING_DATA_MARK;
    break;
  default: // ??
    DEBUG_LOGF("Bad receive state: %u", m_recvState);
    break;
  }
}

void VLReceiver::resetVLState()
{
  m_previousBytes = 0;
  m_recvState = WAITING_HEADER_MARK;
  // zero out the receive buffer and reset bit receiver position
  m_vlData.reset();
  DEBUG_LOG("VL State Reset");
}

#endif

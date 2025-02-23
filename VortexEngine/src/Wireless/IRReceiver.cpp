#include "IRReceiver.h"
#include "IRConfig.h"

#if IR_ENABLE_RECEIVER == 1

#include "../VortexEngine.h"

#include "../Serial/ByteStream.h"
#include "../Serial/BitStream.h"
#include "../Time/TimeControl.h"
#include "../Modes/Mode.h"
#include "../Log/Log.h"

IRReceiver::IRReceiver(VortexEngine &engine) :
  m_engine(engine),
  m_irData(),
  m_recvState(WAITING_HEADER_MARK),
  m_prevTime(0),
  m_pinState(0),
  m_previousBytes(0)
{
}

IRReceiver::~IRReceiver()
{
}

bool IRReceiver::init()
{
  m_irData.init(IR_RECV_BUF_SIZE);
  return true;
}

void IRReceiver::cleanup()
{
}

bool IRReceiver::dataReady()
{
  // is the receiver actually receiving data?
  if (!isReceiving()) {
    return false;
  }
  uint8_t blocks = m_irData.data()[0];
  uint8_t remainder = m_irData.data()[1];
  uint32_t total = ((blocks - 1) * 32) + remainder;
  if (!total || total > IR_MAX_DATA_TRANSFER) {
    DEBUG_LOGF("Bad IR Data size: %u", total);
    return false;
  }
  // if there are size + 2 bytes in the IRData receiver
  // then a full message is ready, the + 2 is from the
  // two bytes for blocks + remainder that are sent first
  return (m_irData.bytepos() >= (uint32_t)(total + 2));
}

// whether actively receiving
bool IRReceiver::isReceiving()
{
  // if there are at least 2 bytes in the data buffer then
  // the receiver is receiving a packet. If there is less
  // than 2 bytes then we're still waiting for the 'blocks'
  // and 'remainder' bytes which prefix a packet
  return (m_irData.bytepos() > 2);
}

// the percent of data received
uint8_t IRReceiver::percentReceived()
{
  if (!isReceiving()) {
    return 0;
  }
  uint8_t blocks = m_irData.data()[0];
  uint8_t remainder = m_irData.data()[1];
  uint16_t total = ((blocks - 1) * 32) + remainder;
  // round by adding half of the total to the numerator
  return (uint8_t)((uint16_t)((m_irData.bytepos() * 100 + (total / 2)) / total));
}

bool IRReceiver::receiveMode(Mode *pMode)
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

bool IRReceiver::beginReceiving()
{
  resetIRState();
  return true;
}

bool IRReceiver::endReceiving()
{
  resetIRState();
  return true;
}

bool IRReceiver::onNewData()
{
  if (bytesReceived() == m_previousBytes) {
    return false;
  }
  m_previousBytes = bytesReceived();
  return true;
}

bool IRReceiver::read(ByteStream &data)
{
  if (!m_irData.bytepos() || m_irData.bytepos() > IR_MAX_DATA_TRANSFER) {
    DEBUG_LOG("Nothing to read, or read too much");
    return false;
  }
  // read the size out (blocks + remainder)
  uint8_t blocks = m_irData.data()[0];
  uint8_t remainder = m_irData.data()[1];
  // calculate size from blocks + remainder
  uint32_t size = ((blocks - 1) * 32) + remainder;
  if (!size || size > IR_MAX_DATA_TRANSFER) {
    DEBUG_LOGF("Bad IR Data size: %u", size);
    return false;
  }
  // the actual data starts 2 bytes later because of the size byte
  const uint8_t *actualData = m_irData.data() + 2;
  if (!data.rawInit(actualData, size)) {
    DEBUG_LOG("Failed to init buffer for IR read");
    return false;
  }
  // reset the IR state and receive buffer now
  resetIRState();
  return true;
}

// The recv PCI handler is called every time the pin state changes
void IRReceiver::recvPCIHandler()
{
  // toggle the tracked pin state no matter what
  m_pinState = (uint8_t)!m_pinState;
  // grab current time
  uint32_t now = m_engine.time().microseconds();
  // check previous time for validity
  if (!m_prevTime || m_prevTime > now) {
    m_prevTime = now;
    DEBUG_LOG("Bad first time diff, resetting...");
    resetIRState();
    return;
  }
  // calc time difference between previous change and now
  uint32_t diff = (uint32_t)(now - m_prevTime);
  // and update the previous changetime for next loop
  m_prevTime = now;
  // handle the blink duration and process it
  handleIRTiming(diff);
}

// state machine that can be fed IR timings to parse them and interpret the intervals
void IRReceiver::handleIRTiming(uint32_t diff)
{
  // if the diff is too long or too short then it's not useful
  if ((diff > IR_HEADER_MARK_MAX && m_recvState < READING_DATA_MARK) || diff < IR_TIMING_MIN) {
    DEBUG_LOGF("bad delay: %u, resetting...", diff);
    resetIRState();
    return;
  }
  switch (m_recvState) {
  case WAITING_HEADER_MARK: // initial state
    if (diff >= IR_HEADER_MARK_MIN && diff <= IR_HEADER_MARK_MAX) {
      m_recvState = WAITING_HEADER_SPACE;
    } else {
      DEBUG_LOGF("Bad header mark %u, resetting...", diff);
      resetIRState();
    }
    break;
  case WAITING_HEADER_SPACE:
    if (diff >= IR_HEADER_SPACE_MIN && diff <= IR_HEADER_SPACE_MAX) {
      m_recvState = READING_DATA_MARK;
    } else {
      DEBUG_LOGF("Bad header space %u, resetting...", diff);
      resetIRState();
    }
    break;
  case READING_DATA_MARK:
    // classify mark/space based on the timing and write into buffer
    m_irData.write1Bit((diff > (IR_TIMING * 2)) ? 1 : 0);
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

void IRReceiver::resetIRState()
{
  m_previousBytes = 0;
  m_recvState = WAITING_HEADER_MARK;
  // zero out the receive buffer and reset bit receiver position
  m_irData.reset();
  DEBUG_LOG("IR State Reset");
}

#endif

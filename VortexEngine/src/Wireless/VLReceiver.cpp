#include "VLReceiver.h"
#include "IRConfig.h"

#if VL_ENABLE_RECEIVER == 1

#include "../Serial/ByteStream.h"
#include "../Serial/BitStream.h"
#include "../Time/TimeControl.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

#ifdef VORTEX_EMBEDDED
#include <avr/interrupt.h>
#include <avr/io.h>
#endif

BitStream VLReceiver::m_vlData;
VLReceiver::RecvState VLReceiver::m_recvState = WAITING_HEADER_MARK;
uint32_t VLReceiver::m_prevTime = 0;
uint8_t VLReceiver::m_pinState = 0;
uint32_t VLReceiver::m_previousBytes = 0;

#ifdef VORTEX_EMBEDDED
#define MIN_THRESHOLD   200
#define BASE_OFFSET     100
#define THRESHOLD_BEGIN (MIN_THRESHOLD + BASE_OFFSET)
// the sample count exponent, so 5 means 2^5 = 32 samples
//   0 NONE No accumulation > doesn't work
//   1 ACC2 2 results accumulated  > doesn't work
//   2 ACC4 4 results accumulated  > works okay
//   3 ACC8 8 results accumulated  > works decent
//   4 ACC16 16 results accumulated > works very well
//   5 ACC32 32 results accumulated > works best
//   6 ACC64 64 results accumulated > doesn't work
#define SAMPLE_COUNT    5
// the threshold needs to start high then it will be automatically pulled down
uint16_t threshold = THRESHOLD_BEGIN;
ISR(ADC0_WCOMP_vect)
{
  // this will store the last known state
  static bool wasAboveThreshold = false;
  // grab the current analog value but divide it by 8 (the number of samples)
  uint16_t val = (ADC0.RES >> SAMPLE_COUNT);
  // calculate a threshold by using the baseline minimum value that is above 0
  // with a static offset, this ensures whatever the baseline light level and/or
  // hardware sensitivity is it will always pick a threshold just above the 'off'
  if (val > MIN_THRESHOLD && val < (threshold + BASE_OFFSET)) {
    threshold = val + BASE_OFFSET;
  }
  // compare the current analog value to the light threshold
  bool isAboveThreshold = (val > threshold);
  if (wasAboveThreshold != isAboveThreshold) {
    VLReceiver::recvPCIHandler();
    wasAboveThreshold = isAboveThreshold;
  }
  // Clear the Window Comparator interrupt flag
  ADC0.INTFLAGS = ADC_WCMP_bm;
}
#endif

bool VLReceiver::init()
{
#ifdef VORTEX_EMBEDDED
  // Disable digital input buffer on the pin to save power
  PORTB.PIN1CTRL &= ~PORT_ISC_gm;
  PORTB.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
#endif
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
#ifdef VORTEX_EMBEDDED
  // Set up the ADC
  // sample campacitance, VDD reference, prescaler division
  // Options are:
  //  0x0 DIV2 CLK_PER divided by 2 > works
  //  0x1 DIV4 CLK_PER divided by 4 > works
  //  0x2 DIV8 CLK_PER divided by 8 > works
  //  0x3 DIV16 CLK_PER divided by 16 > works
  //  0x4 DIV32 CLK_PER divided by 32 > doesn't work
  //  0x5 DIV64 CLK_PER divided by 64 > doesn't work
  //  0x6 DIV128 CLK_PER divided by 128 > doesn't work
  //  0x7 DIV256 CLK_PER divided by 256 > doesn't work
#if (F_CPU == 20000000)
  ADC0.CTRLC = ADC_SAMPCAP_bm | ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV2_gc;
#else
  ADC0.CTRLC = ADC_SAMPCAP_bm | ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV2_gc;
#endif
  // no sampling delay and no delay variation
  ADC0.CTRLD = 0;
  // sample length
  //  0 = doesn't work
  //  1+ = works
  ADC0.SAMPCTRL = 1;
  // Select the analog pin input PB1 (AIN10)
  ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc;
  // Initialize the Window Comparator Mode in above
  ADC0.CTRLE = ADC_WINCM_ABOVE_gc;
  // Set the threshold value very low
  ADC0.WINHT = 0x1;
  ADC0.WINLT = 0;
  // set sampling amount
  //   0x0 NONE No accumulation > doesn't work
  //   0x1 ACC2 2 results accumulated  > doesn't work
  //   0x2 ACC4 4 results accumulated  > works okay
  //   0x3 ACC8 8 results accumulated  > works decent
  //   0x4 ACC16 16 results accumulated > works very well
  //   0x5 ACC32 32 results accumulated > works best
  //   0x6 ACC64 64 results accumulated > doesn't work
  ADC0.CTRLB = SAMPLE_COUNT;
  // Enable Window Comparator interrupt
  ADC0.INTCTRL = ADC_WCMP_bm;
  // Enable the ADC and start continuous conversions
  ADC0.CTRLA = ADC_ENABLE_bm | ADC_FREERUN_bm;
  // start the first conversion
  ADC0.COMMAND = ADC_STCONV_bm;
#endif
  resetVLState();
  return true;
}

bool VLReceiver::endReceiving()
{
#ifdef VORTEX_EMBEDDED
  // Stop conversions and disable the ADC
  ADC0.CTRLA &= ~(ADC_ENABLE_bm | ADC_FREERUN_bm);
  ADC0.INTCTRL = 0;
#endif
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
  uint32_t now = Time::microseconds();
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
#ifdef VORTEX_EMBEDDED
  // reset the threshold to a high value so that it can be pulled down again
  threshold = THRESHOLD_BEGIN;
#endif
  DEBUG_LOG("VL State Reset");
}

#endif

#include "IRSender.h"
#include "IRConfig.h"

#if IR_ENABLE_SENDER == 1

#include "../Time/TimeControl.h"
#include "../Modes/Mode.h"
#include "../Log/Log.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#endif

// the serial buffer for the data
ByteStream IRSender::m_serialBuf;
// a bit walker for the serial data
BitStream IRSender::m_bitStream;
// whether actively sending
bool IRSender::m_isSending = false;
// the time of the last sent chunk
uint32_t IRSender::m_lastSendTime = 0;
// some runtime meta info
uint32_t IRSender::m_size = 0;
// the number of blocks that will be sent
uint8_t IRSender::m_numBlocks = 0;
// the amount in the final block
uint8_t IRSender::m_remainder = 0;
// configuration options for the sender
uint32_t IRSender::m_blockSize = 0;
// write total
uint32_t IRSender::m_writeCounter = 0;

#if defined(VORTEX_EMBEDDED)
// Timer used for PWM, is initialized in initpwm()
Tcc *IR_TCCx;
#endif

bool IRSender::init()
{
  // initialize the IR device
  initPWM();
  return true;
}

void IRSender::cleanup()
{
}

bool IRSender::loadMode(const Mode *targetMode)
{
  m_serialBuf.clear();
  // save the target mode to it's savefile buffer format
  if (!targetMode->saveToBuffer(m_serialBuf)) {
    DEBUG_LOG("Failed to save mode to buffer");
    return false;
  }
  // ensure the data isn't too big
  if (m_serialBuf.rawSize() > IR_MAX_DATA_TRANSFER) {
    DEBUG_LOGF("Cannot transfer that much data: %u bytes", m_serialBuf.rawSize());
    return false;
  }
  // point the bitstream at the serial buffer
  m_bitStream.init((uint8_t *)m_serialBuf.rawData(), m_serialBuf.rawSize());
  // the size of the packet
  m_size = m_serialBuf.rawSize();
  // the number of blocks that will be sent (possibly just 1 with less than 32 bytes)
  m_numBlocks = (m_size + (IR_DEFAULT_BLOCK_SIZE - 1)) / IR_DEFAULT_BLOCK_SIZE;
  // the amount in the final block (possibly the only block)
  m_remainder = m_size % (IR_DEFAULT_BLOCK_SIZE + 0);
  DEBUG_LOGF("Num blocks: %u", m_numBlocks);
  DEBUG_LOGF("Remainder: %u", m_remainder);
  DEBUG_LOGF("Size: %u", m_size);
  // reset write counter
  m_writeCounter = 0;
  return true;
}

bool IRSender::send()
{
  // must have at least one block to send
  if (m_numBlocks < 1) {
    m_isSending = false;
    return m_isSending;
  }
  if (!m_isSending) {
    beginSend();
  }
  if (m_lastSendTime > 0 && m_lastSendTime + IR_DEFAULT_BLOCK_SPACING > Time::getCurtime()) {
    // don't send yet
    return m_isSending;
  }
  // blocksize is default unless it's the last block, then it's the remainder
  uint32_t blocksize = (m_numBlocks == 1) ? m_remainder : IR_DEFAULT_BLOCK_SIZE;
  DEBUG_LOGF("IRSender Sending block #%u", m_numBlocks);
  // pointer to the data at the correct location
  const uint8_t *buf_ptr = m_bitStream.data() + m_writeCounter;
  // iterate each byte in the block and write it
  for (uint32_t i = 0; i < blocksize; ++i) {
    // write the byte
    sendByte(*buf_ptr);
    // increment to next byte in buffer
    buf_ptr++;
    // record the written byte
    m_writeCounter++;
  }
  // wrote one block
  m_numBlocks--;
  // still sending if we have more blocks
  m_isSending = (m_numBlocks > 0);
  // the curtime
  m_lastSendTime = Time::getCurtime();
  // return whether still sending
  return m_isSending;
}

void IRSender::beginSend()
{
  m_isSending = true;
  DEBUG_LOGF("[%zu] Beginning send size %u (blocks: %u remainder: %u blocksize: %u)",
    Time::microseconds(), m_size, m_numBlocks, m_remainder, m_blockSize);
  // init sender before writing, is this necessary here? I think so
  initPWM();
  // wakeup the other receiver with a very quick mark/space
  sendMark(50);
  sendSpace(100);
  // now send the header
  sendMark(IR_HEADER_MARK);
  sendSpace(IR_HEADER_SPACE);
  // reset writeCounter
  m_writeCounter = 0;
  // write the number of blocks being sent, most likely just 1
  sendByte(m_numBlocks);
  // now write the number of bytes in the last block
  sendByte(m_remainder);
}

void IRSender::sendByte(uint8_t data)
{
  // Sends from left to right, MSB first
  for (int b = 0; b < 8; b++) {
    // grab the bit of data at the indexspace
    uint32_t bit = (data >> (7 - b)) & 1;
    // send 3x timing size for 1s and 1x timing for 0
    sendMark(IR_TIMING + (IR_TIMING * (2 * bit)));
    // send 1x timing size for space
    sendSpace(IR_TIMING);
  }
  DEBUG_LOGF("Sent byte[%u]: 0x%x", m_writeCounter, data);
 }

void IRSender::sendMark(uint16_t time)
{
#ifdef VORTEX_LIB
  // send mark timing over socket
  Vortex::vcallbacks()->infraredWrite(true, time);
#else
  startPWM();
  Time::delayMicroseconds(time);
#endif
}

void IRSender::sendSpace(uint16_t time)
{
#ifdef VORTEX_LIB
  // send space timing over socket
  Vortex::vcallbacks()->infraredWrite(false, time);
#else
  stopPWM();
  Time::delayMicroseconds(time);
#endif
}

// shamelessly stolen from IRLib2, thanks
void IRSender::initPWM()
{
#if defined(VORTEX_EMBEDDED)
  // just in case
  pinMode(IR_SEND_PWM_PIN, OUTPUT);
  digitalWrite(IR_SEND_PWM_PIN, LOW); // When not sending PWM, we want it low
  uint8_t port = g_APinDescription[IR_SEND_PWM_PIN].ulPort; // 0
  uint8_t pin = g_APinDescription[IR_SEND_PWM_PIN].ulPin;   // 8
  ETCChannel IR_TCC_Channel = TCC0_CH0;
  int8_t IR_PER_EorF = PORT_PMUX_PMUXE_E;
  //println();Serial.print("Port:"); Serial.print(port,DEC); Serial.print(" Pin:"); Serial.println(pin,DEC);
  // Enable the port multiplexer for the PWM channel on pin
  PORT->Group[port].PINCFG[pin].bit.PMUXEN = 1;

  // Connect the TCC timer to the port outputs - port pins are paired odd PMUXO and even PMUXEII
  // F & E peripherals specify the timers: TCC0, TCC1 and TCC2
  PORT->Group[port].PMUX[pin >> 1].reg |= IR_PER_EorF;

//  pinPeripheral (IR_SEND_PWM_PIN,PIO_TIMER_ALT);
  // Feed GCLK0 to TCC0 and TCC1
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |       // Enable GCLK0 to TCC0 and TCC1
                     GCLK_CLKCTRL_GEN_GCLK0 |   // Select GCLK0
                     GCLK_CLKCTRL_ID_TCC0_TCC1; // Feed GCLK0 to TCC0 and TCC1
  while (GCLK->STATUS.bit.SYNCBUSY);            // Wait for synchronization

  // Normal (single slope) PWM operation: timers countinuously count up to PER
  // register value and then is reset to 0
  IR_TCCx = (Tcc*) GetTC(IR_TCC_Channel);
  IR_TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;   // Setup single slope PWM on TCCx
  while (IR_TCCx->SYNCBUSY.bit.WAVE);           // Wait for synchronization

  // Each timer counts up to a maximum or TOP value set by the PER register,
  // this determines the frequency of the PWM operation.
  uint32_t cc = F_CPU/(38*1000) - 1;
  IR_TCCx->PER.reg = cc;      // Set the frequency of the PWM on IR_TCCx
  while(IR_TCCx->SYNCBUSY.bit.PER);

  // The CCx register value corresponds to the pulsewidth in microseconds (us)
  // Set the duty cycle of the PWM on TCC0 to 33%
  IR_TCCx->CC[GetTCChannelNumber(IR_TCC_Channel)].reg = cc/3;
  while (IR_TCCx->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);
  //while(IR_TCCx->SYNCBUSY.bit.CC3);

  // Enable IR_TCCx timer but do not turn on PWM yet. Will turn it on later.
  IR_TCCx->CTRLA.reg |= TCC_CTRLA_PRESCALER_DIV1;     // Divide GCLK0 by 1
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);
  IR_TCCx->CTRLA.reg &= ~TCC_CTRLA_ENABLE;            //initially off will turn on later
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);
#endif
}

void IRSender::startPWM()
{
#if defined(VORTEX_EMBEDDED)
  // start the PWM
  IR_TCCx->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);
#endif
}

void IRSender::stopPWM()
{
#if defined(VORTEX_EMBEDDED)
  // stop the PWM
  IR_TCCx->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);
#endif
}

#endif

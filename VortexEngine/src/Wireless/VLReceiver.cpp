#include "VLReceiver.h"
#include "IRConfig.h"

#if VL_ENABLE_RECEIVER == 1

#include "../Serial/ByteStream.h"
#include "../Serial/BitStream.h"
#include "../Time/TimeControl.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

BitStream VLReceiver::m_vlData;
VLReceiver::RecvState VLReceiver::m_recvState = WAITING_HEADER_MARK;
uint32_t VLReceiver::m_prevTime = 0;
uint8_t VLReceiver::m_pinState = 0;
uint32_t VLReceiver::m_previousBytes = 0;

#ifdef VORTEX_EMBEDDED
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "../Serial/Serial.h"

// ADC and timer configuration
#define ADC_CHANNEL ADC1_CHANNEL_1 // Update this based on the actual ADC channel used
#define ADC_ATTEN ADC_ATTEN_DB_0
#define ADC_WIDTH ADC_WIDTH_BIT_12
#define TIMER_INTERVAL_MICRO_SEC 1000 // Check every 10ms, adjust as needed for your application

// Timer handle as a global variable for control in beginReceiving and endReceiving
esp_timer_handle_t periodic_timer = nullptr;
esp_adc_cal_characteristics_t adc_chars;

#define MIN_THRESHOLD   200
#define BASE_OFFSET     100
#define THRESHOLD_BEGIN (MIN_THRESHOLD + BASE_OFFSET)
// the threshold needs to start high then it will be automatically pulled down
uint32_t threshold = THRESHOLD_BEGIN;
void VLReceiver::adcCheckTimerCallback(void *arg)
{
  static bool wasAboveThreshold = false;
  uint32_t raw = adc1_get_raw(ADC_CHANNEL);
  uint32_t val = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

  if (val > MIN_THRESHOLD && val < (threshold + BASE_OFFSET)) {
    threshold = val + BASE_OFFSET;
  }
  bool isAboveThreshold = (val > threshold);
  if (wasAboveThreshold != isAboveThreshold) {
    wasAboveThreshold = isAboveThreshold;
    VLReceiver::recvPCIHandler();
  }
}
#endif

bool VLReceiver::init()
{
#ifdef VORTEX_EMBEDDED
  // Initialize ADC for GPIO1 (or appropriate pin connected to your light sensor)
  adc1_config_width(ADC_WIDTH);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
  memset(&adc_chars, 0, sizeof(adc_chars));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 0, &adc_chars);
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
  if (periodic_timer) {
    DEBUG_LOG("VL Reception already running.");
    return false; // Timer is already running
  }
  // Initialize timer for periodic ADC checks
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &VLReceiver::adcCheckTimerCallback,
      .name = "adc_check_timer",
  };
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TIMER_INTERVAL_MICRO_SEC));
#endif
  resetVLState();
  return true;
}

bool VLReceiver::endReceiving()
{
#ifdef VORTEX_EMBEDDED
  if (periodic_timer == nullptr) {
    DEBUG_LOG("VL Reception was not running.");
    return false; // Timer was not running
  }
  // Stop and delete the timer
  ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
  ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
  periodic_timer = nullptr;
  DEBUG_LOG("VL Reception stopped.");
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
    //DEBUG_LOG("Bad first time diff, resetting...");
    resetVLState();
    return;
  }
  //DEBUG_LOGF("Received: %u", m_pinState);
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
    //DEBUG_LOGF("bad delay: %u, resetting...", diff);
    resetVLState();
    return;
  }
  switch (m_recvState) {
  case WAITING_HEADER_MARK: // initial state
    if (diff >= VL_HEADER_SPACE_MIN && diff <= VL_HEADER_MARK_MAX) {
      m_recvState = WAITING_HEADER_SPACE;
    } else {
      //DEBUG_LOGF("Bad header mark %u, resetting...", diff);
      resetVLState();
    }
    break;
  case WAITING_HEADER_SPACE:
    if (diff >= VL_HEADER_SPACE_MIN && diff <= VL_HEADER_MARK_MAX) {
      m_recvState = READING_DATA_MARK;
    } else {
      //DEBUG_LOGF("Bad header space %u, resetting...", diff);
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
    //DEBUG_LOGF("Bad receive state: %u", m_recvState);
    break;
  }
}

void VLReceiver::resetVLState()
{
  m_previousBytes = 0;
  m_recvState = WAITING_HEADER_MARK;
  // zero out the receive buffer and reset bit receiver position
  m_vlData.reset();
  //DEBUG_LOG("VL State Reset");
}

#endif

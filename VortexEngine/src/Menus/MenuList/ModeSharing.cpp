#include "ModeSharing.h"

#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Wireless/VLReceiver.h"
#include "../../Wireless/VLSender.h"
#include "../../Wireless/IRReceiver.h"
#include "../../Wireless/IRSender.h"
#include "../../Buttons/Button.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

ModeSharing::ModeSharing(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_sharingMode(ModeShareState::SHARE_RECEIVE_VL),
  m_timeOutStartTime(0),
  m_lastSendTime(0),
  m_shouldEndSend(false)
{
}

ModeSharing::~ModeSharing()
{
}

bool ModeSharing::init()
{
  if (!Menu::init()) {
    return false;
  }
  // skip led selection
  m_ledSelected = true;
  // start on receive because it's the more responsive of the two
  // the odds of opening receive and then accidentally receiving
  // a mode that is being broadcast nearby is completely unlikely
  beginReceivingVL();
  DEBUG_LOG("Entering Mode Sharing");
  return true;
}

Menu::MenuAction ModeSharing::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }
  switch (m_sharingMode) {
  case ModeShareState::SHARE_RECEIVE_VL:
    // render the 'receive mode' lights
    showReceiveModeVL();
    // load any modes that are received
    receiveModeVL();
    break;
  case ModeShareState::SHARE_SEND_VL:
    // render the 'send mode' lights
    showSendModeVL();
    // continue sending any data as long as there is more to send
    continueSendingVL();
    break;
  case ModeShareState::SHARE_RECEIVE_IR:
    // render the 'receive mode' lights
    showReceiveModeIR();
    // load any modes that are received
    receiveModeIR();
    break;
  case ModeShareState::SHARE_SEND_IR:
    // render the 'send mode' lights
    showSendModeIR();
    // continue sending any data as long as there is more to send
    continueSendingIR();
    break;
  }
  return MENU_CONTINUE;
}

// handlers for clicks
void ModeSharing::onShortClickM()
{
  switch (m_sharingMode) {
  case ModeShareState::SHARE_RECEIVE_VL:
    // click while on receive -> end receive, start sending
    VLReceiver::endReceiving();
    beginSendingVL();
    DEBUG_LOG("Switched to send VL");
    break;
  case ModeShareState::SHARE_RECEIVE_IR:
    if (!IRSender::isSending()) {
      beginSendingIR();
      DEBUG_LOG("Switched to send IR");
    } else {
      m_shouldEndSend = true;
    }
    break;
  case ModeShareState::SHARE_SEND_IR:
    m_shouldEndSend = true;
    break;
  default:
    break;
  }
  Leds::clearAll();
}

// handlers for clicks
void ModeSharing::onShortClickL()
{
  switchVLIR();
}

void ModeSharing::onShortClickR()
{
  switchVLIR();
}

void ModeSharing::switchVLIR()
{
  switch (m_sharingMode) {
  case ModeShareState::SHARE_RECEIVE_VL:
    VLReceiver::endReceiving();
    beginReceivingIR();
    break;
  case ModeShareState::SHARE_RECEIVE_IR:
    IRReceiver::endReceiving();
    beginReceivingVL();
    break;
  default:
    break;
  }
  Leds::clearAll();
}

void ModeSharing::onLongClickM()
{
  Modes::updateCurMode(&m_previewMode);
  leaveMenu(true);
}

void ModeSharing::beginSendingIR()
{
  // if the sender is sending then cannot start again
  if (IRSender::isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  m_sharingMode = ModeShareState::SHARE_SEND_IR;
  Leds::clearAll();
  Leds::update();
  // initialize it with the current mode data
  IRSender::loadMode(Modes::curMode());
  // send the first chunk of data, leave if we're done
  if (!IRSender::send()) {
    // just set the last time and wait
    m_lastSendTime = Time::getCurtime();
  }
  DEBUG_LOG("Switched to sending IR");
}

void ModeSharing::continueSendingIR()
{
  // if the sender isn't sending then nothing to do
  if (!IRSender::isSending()) {
    if (m_lastSendTime && m_lastSendTime < (Time::getCurtime() + MS_TO_TICKS(350))) {
      if (m_shouldEndSend) {
        beginReceivingIR();
        m_shouldEndSend = false;
      } else {
        beginSendingIR();
      }
    }
    return;
  }
  if (!IRSender::send()) {
    // just set the last time and wait
    m_lastSendTime = Time::getCurtime();
  }
}

void ModeSharing::beginReceivingIR()
{
  m_sharingMode = ModeShareState::SHARE_RECEIVE_IR;
  IRReceiver::beginReceiving();
  DEBUG_LOG("Switched to receiving IR");
}

void ModeSharing::receiveModeIR()
 {
  // if reveiving new data set our last data time
  if (IRReceiver::onNewData()) {
    m_timeOutStartTime = Time::getCurtime();
    // if our last data was more than time out duration reset the recveiver
  } else if (m_timeOutStartTime > 0 && (m_timeOutStartTime + MAX_TIMEOUT_DURATION) < Time::getCurtime()) {
    IRReceiver::resetIRState();
    m_timeOutStartTime = 0;
    return;
  }
  // check if the IRReceiver has a full packet available
  if (!IRReceiver::dataReady()) {
    // nothing available yet
    return;
  }
  DEBUG_LOG("Mode ready to receive! Receiving...");
  // receive the IR mode into the current mode
  if (!IRReceiver::receiveMode(&m_previewMode)) {
    ERROR_LOG("Failed to receive mode");
    return;
  }
  DEBUG_LOGF("Success receiving mode: %u", m_previewMode.getPatternID());
  if (!m_advanced) {
    Modes::updateCurMode(&m_previewMode);
    // leave menu and save settings, even if the mode was the same whatever
    leaveMenu(true);
  }
}

void ModeSharing::beginSendingVL()
{
  // if the sender is sending then cannot start again
  if (VLSender::isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  m_sharingMode = ModeShareState::SHARE_SEND_VL;
  // initialize it with the current mode data
  VLSender::loadMode(Modes::curMode());
  // send the first chunk of data, leave if we're done
  if (!VLSender::send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingVL();
  }
  DEBUG_LOG("Switched to sending VL");
}

void ModeSharing::continueSendingVL()
{
  // if the sender isn't sending then nothing to do
  if (!VLSender::isSending()) {
    return;
  }
  if (!VLSender::send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingVL();
  }
}

void ModeSharing::beginReceivingVL()
{
  m_sharingMode = ModeShareState::SHARE_RECEIVE_VL;
  VLReceiver::beginReceiving();
  DEBUG_LOG("Switched to receiving VL");
}

void ModeSharing::receiveModeVL()
{
  // if reveiving new data set our last data time
  if (VLReceiver::onNewData()) {
    m_timeOutStartTime = Time::getCurtime();
    // if our last data was more than time out duration reset the recveiver
  } else if (m_timeOutStartTime > 0 && (m_timeOutStartTime + MAX_TIMEOUT_DURATION) < Time::getCurtime()) {
    VLReceiver::resetVLState();
    m_timeOutStartTime = 0;
    return;
  }
  // check if the VLReceiver has a full packet available
  if (!VLReceiver::dataReady()) {
    // nothing available yet
    return;
  }
  DEBUG_LOG("Mode ready to receive! Receiving...");
  // receive the VL mode into the current mode
  if (!VLReceiver::receiveMode(&m_previewMode)) {
    ERROR_LOG("Failed to receive mode");
    return;
  }
  DEBUG_LOGF("Success receiving mode: %u", m_previewMode.getPatternID());
  if (!m_advanced) {
    Modes::updateCurMode(&m_previewMode);
    // leave menu and save settings, even if the mode was the same whatever
    leaveMenu(true);
  }
}

void ModeSharing::showSendModeVL()
{
  // show a dim color when not sending
  Leds::clearAll();
}

void ModeSharing::showSendModeIR()
{
  // show a dim color when not sending
  Leds::setAll(RGB_CYAN5);
  Leds::setAllEvens(RGB_CYAN0);
}

void ModeSharing::showReceiveModeVL()
{
  if (VLReceiver::isReceiving()) {
    // using uint32_t to avoid overflow, the result should be within 10 to 255
    Leds::clearAll();
    Leds::setRange(LED_FIRST, (LedPos)(VLReceiver::percentReceived() / 10), RGBColor(0, 1, 0));
  } else {
    if (m_advanced) {
      m_previewMode.play();
    } else {
      Leds::setAll(0x010101);
    }
  }
}

void ModeSharing::showReceiveModeIR()
{
  if (VLReceiver::isReceiving()) {
    // using uint32_t to avoid overflow, the result should be within 10 to 255
    Leds::clearAll();
    Leds::setRange(LED_FIRST, (LedPos)(VLReceiver::percentReceived() / 10), RGBColor(0, 1, 0));
  } else {
    if (m_advanced) {
      m_previewMode.play();
    } else {
      Leds::setAll(RGB_CYAN0);
    }
  }
}

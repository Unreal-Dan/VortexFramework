#include "ModeSharing.h"

#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Wireless/IRReceiver.h"
#include "../../Wireless/IRSender.h"
#include "../../Buttons/Button.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

ModeSharing::ModeSharing(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_sharingMode(ModeShareState::SHARE_SEND),
  m_lastActionTime(0),
  m_timeOutStartTime(0)
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
  // This makes send mode begin with waiting instead of sending
  m_lastActionTime = Time::getCurtime() + 1;
  // just start spewing out modes everywhere
  m_sharingMode = ModeShareState::SHARE_SEND;
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
  case ModeShareState::SHARE_SEND:
    // render the 'send mode' lights
    showSendMode();
    if (!IRSender::isSending()) {
      if (!m_lastActionTime || ((m_lastActionTime + MAX_WAIT_DURATION) < Time::getCurtime())) {
        beginSending();
      }
    }
    // continue sending any data as long as there is more to send
    continueSending();
    break;
  case ModeShareState::SHARE_RECEIVE:
    // render the 'receive mode' lights
    showReceiveMode();
    // load any modes that are received
    receiveMode();
    break;
  }
  return MENU_CONTINUE;
}

// handlers for clicks
void ModeSharing::onShortClick()
{
  switch (m_sharingMode) {
  case ModeShareState::SHARE_SEND:
    // click while on send -> start listening
    IRReceiver::beginReceiving();
    m_sharingMode = ModeShareState::SHARE_RECEIVE;
    DEBUG_LOG("Switched to receive mode");
    break;
  case ModeShareState::SHARE_RECEIVE:
  default:
    // click while on receive -> end receive, start sending
    IRReceiver::endReceiving();
    m_sharingMode = ModeShareState::SHARE_SEND;
    DEBUG_LOG("Switched to send mode");
    break;
  }
  Leds::clearAll();
}

void ModeSharing::onLongClick()
{
  Modes::updateCurMode(&m_previewMode);
  leaveMenu(true);
}

void ModeSharing::beginSending()
{
  // if the sender is sending then cannot start again
  if (IRSender::isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  // initialize it with the current mode data
  IRSender::loadMode(Modes::curMode());
  // send the first chunk of data, leave if we're done
  if (!IRSender::send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    m_lastActionTime = Time::getCurtime();
  }
}

void ModeSharing::continueSending()
{
  // if the sender isn't done then keep sending data
  if (IRSender::isSending()) {
    if (!IRSender::send()) {
      // when send has completed, stores time that last action was completed to calculate interval between sends
      m_lastActionTime = Time::getCurtime();
    }
  }
}

void ModeSharing::receiveMode()
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

void ModeSharing::showSendMode()
{
  Leds::setAll(RGB_WHITE0);
  Leds::blinkAll(10, 25, RGB_CYAN3);
}

void ModeSharing::showReceiveMode()
{
  Leds::clearAll();
  if (IRReceiver::isReceiving()) {
    // TODO: don't use floats here
    uint8_t percent = IRReceiver::percentReceived();
    LedPos l = (LedPos)(percent / (255.0 / LED_COUNT));
    Leds::setRange(LED_FIRST, l, RGB_GREEN);
    return;
  }
  // gradually empty from thumb to pinkie
  Leds::setAll(RGB_WHITE0);
}

// override showExit so it isn't displayed on thumb
void ModeSharing::showExit()
{
}

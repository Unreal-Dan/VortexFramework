#include "ModeSharing.h"

#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Wireless/VLReceiver.h"
#include "../../Wireless/VLSender.h"
#include "../../Patterns/Pattern.h"
#include "../../Buttons/Button.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

ModeSharing::ModeSharing(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_sharingMode(ModeShareState::SHARE_RECEIVE),
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
  if (!m_advanced) {
    // skip led selection
    m_ledSelected = true;
  }
  // start on receive because it's the more responsive of the two
  // the odds of opening receive and then accidentally receiving
  // a mode that is being broadcast nearby is completely unlikely
  beginReceiving();
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

void ModeSharing::onLedSelected()
{
  // if we selected leds that implies advanced mode
  if (m_targetLeds == MAP_LED(LED_1)) {
    // if we selected the top led then simply swap the two patterns
    // so that the top led is sent first -- if the receiver is receiving
    // into one slot then it will only use the first pattern to do so
    m_previewMode.swapPatterns(LED_0, LED_1);
  }
}

// handlers for clicks
void ModeSharing::onShortClick()
{
  switch (m_sharingMode) {
  case ModeShareState::SHARE_RECEIVE:
    // click while on receive -> end receive, start sending
    VLReceiver::endReceiving();
    beginSending();
    DEBUG_LOG("Switched to send mode");
    break;
  default:
    break;
  }
  Leds::clearAll();
}

void ModeSharing::onLongClick()
{
  leaveMenu();
}

void ModeSharing::beginSending()
{
  // if the sender is sending then cannot start again
  if (VLSender::isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  m_sharingMode = ModeShareState::SHARE_SEND;
  // initialize it with the current mode data
  VLSender::loadMode(&m_previewMode);
  // send the first chunk of data, leave if we're done
  if (!VLSender::send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceiving();
  }
}

void ModeSharing::continueSending()
{
  // if the sender isn't sending then nothing to do
  if (!VLSender::isSending()) {
    return;
  }
  if (!VLSender::send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceiving();
  }
}

void ModeSharing::beginReceiving()
{
  m_sharingMode = ModeShareState::SHARE_RECEIVE;
  VLReceiver::beginReceiving();
}

void ModeSharing::receiveMode()
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
  if (m_advanced && m_targetLeds != MAP_LED_ALL) {
    LedPos target = ledmapGetFirstLed(m_targetLeds);
    LedPos other = LED_1;
    // if the user picked the top led to copy into then swap the patterns
    // in the incoming mode so the 0th pattern is on the top led
    if (target == LED_1) {
      other = LED_0;
      m_previewMode.swapPatterns(LED_0, LED_1);
    }
    m_previewMode.copyPatternFrom(Modes::curMode(), other, other);
  }
  Modes::updateCurMode(&m_previewMode);
  // leave menu and save settings, even if the mode was the same whatever
  leaveMenu(true);
}

void ModeSharing::showSendMode()
{
  // show a dim color when not sending
  if (!VLSender::isSending()) {
    Leds::setAll(RGBColor(0, 20, 20));
  }
}

void ModeSharing::showReceiveMode()
{
  if (VLReceiver::isReceiving()) {
    // using uint32_t to avoid overflow, the result should be within 10 to 255
    Leds::setIndex(LED_0, RGBColor(0, VLReceiver::percentReceived(), 0));
    Leds::clearIndex(LED_1);
  } else {
    Leds::setAll(RGB_WHITE0);
  }
}

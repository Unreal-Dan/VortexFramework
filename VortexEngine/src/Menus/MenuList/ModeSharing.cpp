#include "ModeSharing.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Wireless/IRReceiver.h"
#include "../../Wireless/VLSender.h"
#include "../../Wireless/IRSender.h"
#include "../../Buttons/Button.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

ModeSharing::ModeSharing(VortexEngine &engine, const RGBColor &col, bool advanced) :
  Menu(engine, col, advanced),
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
  // skip led selection
  m_ledSelected = true;
  // start on receive because it's the more responsive of the two
  // the odds of opening receive and then accidentally receiving
  // a mode that is being broadcast nearby is completely unlikely
  beginReceivingIR();
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
  case ModeShareState::SHARE_SEND_IR:
    // render the 'send mode' lights
    showSendModeIR();
    // continue sending any data as long as there is more to send
    continueSendingIR();
    break;
  case ModeShareState::SHARE_SEND_VL:
    // render the 'send mode' lights
    showSendModeVL();
    // continue sending any data as long as there is more to send
    continueSendingVL();
    break;
  case ModeShareState::SHARE_RECEIVE:
    // render the 'receive mode' lights
    showReceiveMode();
    // load any modes that are received
    receiveModeIR();
    break;
  }
  return MENU_CONTINUE;
}

// handlers for clicks
void ModeSharing::onShortClick()
{
  switch (m_sharingMode) {
  case ModeShareState::SHARE_RECEIVE:
    // click while on receive -> end receive, start sending
    m_engine.irReceiver().endReceiving();
    beginSendingIR();
    DEBUG_LOG("Switched to send mode");
    break;
  default:
    break;
  }
  m_engine.leds().clearAll();
}

void ModeSharing::onLongClick()
{
  m_engine.modes().updateCurMode(&m_previewMode);
  leaveMenu(true);
}

void ModeSharing::beginSendingVL()
{
  // if the sender is sending then cannot start again
  if (m_engine.vlSender().isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  m_sharingMode = ModeShareState::SHARE_SEND_VL;
  // initialize it with the current mode data
  m_engine.vlSender().loadMode(m_engine.modes().curMode());
  // send the first chunk of data, leave if we're done
  if (!m_engine.vlSender().send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingIR();
  }
}

void ModeSharing::beginSendingIR()
{
  // if the sender is sending then cannot start again
  if (m_engine.irSender().isSending()) {
    ERROR_LOG("Cannot begin sending, sender is busy");
    return;
  }
  m_sharingMode = ModeShareState::SHARE_SEND_IR;
  // initialize it with the current mode data
  m_engine.irSender().loadMode(m_engine.modes().curMode());
  // send the first chunk of data, leave if we're done
  if (!m_engine.irSender().send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingIR();
  }
}

void ModeSharing::continueSendingVL()
{
  // if the sender isn't sending then nothing to do
  if (!m_engine.vlSender().isSending()) {
    return;
  }
  if (!m_engine.vlSender().send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingIR();
  }
}

void ModeSharing::continueSendingIR()
{
  // if the sender isn't sending then nothing to do
  if (!m_engine.irSender().isSending()) {
    return;
  }
  if (!m_engine.irSender().send()) {
    // when send has completed, stores time that last action was completed to calculate interval between sends
    beginReceivingIR();
  }
}

void ModeSharing::beginReceivingIR()
{
  m_sharingMode = ModeShareState::SHARE_RECEIVE;
  m_engine.irReceiver().beginReceiving();
}

void ModeSharing::receiveModeIR()
{
  // if reveiving new data set our last data time
  if (m_engine.irReceiver().onNewData()) {
    m_timeOutStartTime = m_engine.time().getCurtime();
    // if our last data was more than time out duration reset the recveiver
  } else if (m_timeOutStartTime > 0 && (m_timeOutStartTime + MAX_TIMEOUT_DURATION) < m_engine.time().getCurtime()) {
    m_engine.irReceiver().resetIRState();
    m_timeOutStartTime = 0;
    return;
  }
  // check if the IRReceiver has a full packet available
  if (!m_engine.irReceiver().dataReady()) {
    // nothing available yet
    return;
  }
  DEBUG_LOG("Mode ready to receive! Receiving...");
  // receive the IR mode into the current mode
  if (!m_engine.irReceiver().receiveMode(&m_previewMode)) {
    ERROR_LOG("Failed to receive mode");
    return;
  }
  DEBUG_LOGF("Success receiving mode: %u", m_previewMode.getPatternID());
  if (!m_advanced) {
    m_engine.modes().updateCurMode(&m_previewMode);
    // leave menu and save settings, even if the mode was the same whatever
    leaveMenu(true);
  }
}

void ModeSharing::showSendModeVL()
{
  // show a dim color when not sending
  m_engine.leds().clearAll();
}

void ModeSharing::showSendModeIR()
{
  // show a dim color when not sending
  m_engine.leds().clearAll();
}

void ModeSharing::showReceiveMode()
{
  if (m_engine.irReceiver().isReceiving()) {
    // using uint32_t to avoid overflow, the result should be within 10 to 255
    m_engine.leds().setAll(RGBColor(0, m_engine.irReceiver().percentReceived(), 0));
  } else {
    if (m_advanced) {
      m_previewMode.play();
    } else {
      m_engine.leds().setAll(RGB_WHITE0);
    }
  }
}

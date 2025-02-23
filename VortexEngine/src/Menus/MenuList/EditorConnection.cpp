#include "EditorConnection.h"

#include "../../Patterns/PatternArgs.h"
#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Storage/Storage.h"
#include "../../Wireless/VLSender.h"
#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

#include <string.h>

EditorConnection::EditorConnection(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_state(STATE_DISCONNECTED),
  m_allowReset(true),
  m_previousModeIndex(0),
  m_numModesToReceive(0),
  m_rv(RV_OK)
{
}

EditorConnection::~EditorConnection()
{
  clearDemo();
}

bool EditorConnection::init()
{
  if (!Menu::init()) {
    return false;
  }
  // skip led selection
  m_ledSelected = true;
  clearDemo();
  DEBUG_LOG("Entering Editor Connection");
  return true;
}

Menu::MenuAction EditorConnection::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }
  // TODO: auto leave the editor menu when unplugged
  // show the editor
  showEditor();
  // receive any data from serial into the receive buffer
  receiveData();
  // handle the current state
  handleState();
  return MENU_CONTINUE;
}

void EditorConnection::onShortClick()
{
  // if the device has received any commands do not reset!
  if (!m_allowReset) {
    return;
  }
  // reset, this won't actually disconnect the com port
  m_state = STATE_DISCONNECTED;
  // clear the demo
  clearDemo();
  // sent a reset, do not allow another
  m_allowReset = false;
}

void EditorConnection::onShortClick2()
{
  sendCurModeVL();
}

void EditorConnection::onLongClick()
{
  leaveMenu(true);
}

void EditorConnection::onLongClick2()
{
  leaveMenu(true);
}

void EditorConnection::leaveMenu(bool doSave)
{
  SerialComs::write(EDITOR_VERB_GOODBYE);
  Menu::leaveMenu(true);
}

void EditorConnection::clearDemo()
{
  Colorset set(RGB_WHITE0);
  PatternArgs args(1, 0, 0);
  m_previewMode.setPattern(PATTERN_STROBE, LED_ALL, &args, &set);
  m_previewMode.init();
}

void EditorConnection::handleErrors()
{
  if (m_rv == RV_FAIL) {
    // handle failure from before, reset rv
    m_rv = RV_OK;
    // clear the buffer I guess
    m_receiveBuffer.clear();
  }
  // TODO: Custom error codes?
}

const EditorConnection::CommandState EditorConnection::commands[] = {
  { EDITOR_VERB_PULL_MODES, STATE_PULL_MODES },
  { EDITOR_VERB_PUSH_MODES, STATE_PUSH_MODES },
  { EDITOR_VERB_DEMO_MODE, STATE_DEMO_MODE },
  { EDITOR_VERB_CLEAR_DEMO, STATE_CLEAR_DEMO },
  { EDITOR_VERB_PULL_EACH_MODE, STATE_PULL_EACH_MODE },
  { EDITOR_VERB_PUSH_EACH_MODE, STATE_PUSH_EACH_MODE },
  { EDITOR_VERB_TRANSMIT_VL, STATE_TRANSMIT_MODE_VL },
  { EDITOR_VERB_SET_GLOBAL_BRIGHTNESS, STATE_SET_GLOBAL_BRIGHTNESS },
  { EDITOR_VERB_GET_GLOBAL_BRIGHTNESS, STATE_GET_GLOBAL_BRIGHTNESS },
};
#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

void EditorConnection::handleCommand()
{
  if (m_receiveBuffer.size() < 1) {
    return;
  }
  for (uint32_t i = 0; i < NUM_COMMANDS; ++i) {
    // commands are just one letter, don't consume it yet so we can compare
    // each cmd in the list, this probably should be a lookup table
    char receiveCmd = m_receiveBuffer.data()[0];
    // if the command matches
    if (receiveCmd != commands[i].cmd[0]) {
      continue;
    }
    // then consume the data from the receive buffer and update the state
    m_receiveBuffer.consume8();
    m_state = commands[i].cmdState;
    // we have now received at least one command, do not allow resetting
    m_allowReset = false;
    // don't need to check any more commands
    break;
  }
}

void EditorConnection::handleState()
{
  // operate on the state of the editor connection
  switch (m_state) {
  // -------------------------------
  //  Disconnected
  case STATE_DISCONNECTED:
  default:
    // not connected yet so check for connections
    if (!SerialComs::isConnected()) {
      if (!SerialComs::checkSerial()) {
        // no connection found just continue waiting
        break;
      }
    }
    // a connection was found, say hello
    m_state = STATE_GREETING;
    break;

  // -------------------------------
  //  Send Greeting
  case STATE_GREETING:
    // send the hello greeting with our version number and build time
    SerialComs::write(EDITOR_VERB_GREETING);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Chillin
  case STATE_IDLE:
    // handle any errors that may have occurred last run
    handleErrors();
    // parse the receive buffer for any commands from the editor
    handleCommand();
    // watch for disconnects
    if (!SerialComs::isConnected()) {
      Leds::holdAll(RGB_RED);
      leaveMenu(true);
    }
    break;

  // -------------------------------
  //  Send Modes to PC
  case STATE_PULL_MODES:
    // editor requested pull modes, send the modes
    sendModes();
    m_state = STATE_PULL_MODES_SEND;
    break;
  case STATE_PULL_MODES_SEND:
    // recive the send modes ack from the editor
    if (receiveMessage(EDITOR_VERB_PULL_MODES_DONE) == RV_WAIT) {
      // just wait
      break;
    }
    m_state = STATE_PULL_MODES_DONE;
    break;
  case STATE_PULL_MODES_DONE:
    // send our acknowledgement that the modes were sent
    SerialComs::write(EDITOR_VERB_PULL_MODES_ACK);
    // go idle
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Receive Modes from PC
  case STATE_PUSH_MODES:
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_PUSH_MODES_RECEIVE;
    break;
  case STATE_PUSH_MODES_RECEIVE:
    // receive the modes into the receive buffer
    if (receiveModes() == RV_WAIT) {
      // just keep waiting
      break;
    }
    // success modes were received send the done
    m_state = STATE_PUSH_MODES_DONE;
    break;
  case STATE_PUSH_MODES_DONE:
    SerialComs::write(EDITOR_VERB_PUSH_MODES_ACK);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Demo Mode from PC
  case STATE_DEMO_MODE:
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_DEMO_MODE_RECEIVE;
    break;
  case STATE_DEMO_MODE_RECEIVE:
    // receive the modes into the receive buffer
    if (receiveDemoMode() == RV_WAIT) {
      // just wait
      break;
    }
    // success modes were received send the done
    m_state = STATE_DEMO_MODE_DONE;
    break;
  case STATE_DEMO_MODE_DONE:
    // say we are done
    SerialComs::write(EDITOR_VERB_DEMO_MODE_ACK);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Reset Demo to Nothing
  case STATE_CLEAR_DEMO:
    clearDemo();
    SerialComs::write(EDITOR_VERB_CLEAR_DEMO_ACK);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Send Mode to Duo
  case STATE_TRANSMIT_MODE_VL:
#if VL_ENABLE_SENDER == 1
    // immediately load the mode and send it now
    VLSender::loadMode(&m_previewMode);
    VLSender::send();
#endif
    m_state = STATE_TRANSMIT_MODE_VL_TRANSMIT;
    break;
  case STATE_TRANSMIT_MODE_VL_TRANSMIT:
#if VL_ENABLE_SENDER == 1
    // if still sending and the send command indicated more data
    if (VLSender::isSending() && VLSender::send()) {
      // then continue sending
      break;
    }
#endif
    // othewrise, done, switch to the transmit done state
    m_state = STATE_TRANSMIT_MODE_VL_DONE;
    break;
  case STATE_TRANSMIT_MODE_VL_DONE:
    // done transmitting
    SerialComs::write(EDITOR_VERB_TRANSMIT_VL_ACK);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Send Modes to PC Safer
  case STATE_PULL_EACH_MODE:
    // editor requested pull modes, send the modes
    sendModeCount();
    m_state = STATE_PULL_EACH_MODE_COUNT;
    break;
  case STATE_PULL_EACH_MODE_COUNT:
    if (receiveMessage(EDITOR_VERB_PULL_EACH_MODE_ACK) == RV_WAIT) {
      // just wait
      break;
    }
    if (Modes::numModes() == 0) {
      m_state = STATE_PULL_EACH_MODE_DONE;
    } else {
      m_previousModeIndex = Modes::curModeIndex();
      m_state = STATE_PULL_EACH_MODE_SEND;
    }
    break;
  case STATE_PULL_EACH_MODE_SEND:
    // send the current mode
    sendCurMode();
    // wait for the ack
    m_state = STATE_PULL_EACH_MODE_WAIT;
    break;
  case STATE_PULL_EACH_MODE_WAIT:
    // recive the ack from the editor to send next mode
    if (receiveMessage(EDITOR_VERB_PULL_EACH_MODE_ACK) == RV_WAIT) {
      // just wait
      break;
    }
    // if there is still more modes
    if (Modes::curModeIndex() < (Modes::numModes() - 1)) {
      // then iterate to the next mode and send
      Modes::nextMode();
      m_state = STATE_PULL_EACH_MODE_SEND;
    } else {
      // otherwise done sending modes
      m_state = STATE_PULL_EACH_MODE_DONE;
    }
    break;
  case STATE_PULL_EACH_MODE_DONE:
    // send our acknowledgement that the modes were sent
    SerialComs::write(EDITOR_VERB_PULL_EACH_MODE_DONE);
    // switch back to the previous mode
    Modes::setCurMode(m_previousModeIndex);
    // go idle
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Receive Modes from PC Safer
  case STATE_PUSH_EACH_MODE:
    // editor requested to push modes, find out how many
    // ack the command and wait for the amount of modes
    SerialComs::write(EDITOR_VERB_PUSH_EACH_MODE_ACK);
    m_state = STATE_PUSH_EACH_MODE_COUNT;
    break;
  case STATE_PUSH_EACH_MODE_COUNT:
    if (receiveModeCount() == RV_WAIT) {
      // just wait
      break;
    }
    // clear modes and start receiving
    Modes::clearModes();
    // write out an ack
    SerialComs::write(EDITOR_VERB_PUSH_EACH_MODE_ACK);
    // ready to receive a mode
    m_state = STATE_PUSH_EACH_MODE_RECEIVE;
    break;
  case STATE_PUSH_EACH_MODE_RECEIVE:
    // receive the modes into the receive buffer
    if (receiveMode() == RV_WAIT) {
      // just wait
      break;
    }
    SerialComs::write(EDITOR_VERB_PUSH_EACH_MODE_ACK);
    if (m_numModesToReceive > 0) {
      m_numModesToReceive--;
    }
    if (!m_numModesToReceive) {
      // success modes were received send the done
      m_state = STATE_PUSH_EACH_MODE_DONE;
    }
    break;
  case STATE_PUSH_EACH_MODE_DONE:
    // did originally receive/send a DONE message here but it wasn't working
    // on lightshow.lol so just skip to IDLE
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Set Global Brightness
  case STATE_SET_GLOBAL_BRIGHTNESS:
    SerialComs::write(EDITOR_VERB_READY);
    m_state = STATE_SET_GLOBAL_BRIGHTNESS_RECEIVE;
    break;
  case STATE_SET_GLOBAL_BRIGHTNESS_RECEIVE:
    // set the brightness of the device
    if (receiveBrightness() == RV_WAIT) {
      // just keep waiting
      break;
    }
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Get Global Brightness
  case STATE_GET_GLOBAL_BRIGHTNESS:
    sendBrightness();
    m_state = STATE_IDLE;
    break;
  }
}

void EditorConnection::showEditor()
{
  switch (m_state) {
  case STATE_DISCONNECTED:
    Leds::clearAll();
    Leds::blinkAll(250, 150, RGB_WHITE0);
    break;
  case STATE_IDLE:
    m_previewMode.play();
    break;
  default:
    // do nothing!
    // Note if you clear the leds while selecting color
    // it may make the color selection choppy
    break;
  }
}

void EditorConnection::receiveData()
{
  // read more data into the receive buffer
  SerialComs::read(m_receiveBuffer);
}

void EditorConnection::sendModes()
{
  ByteStream modesBuffer;
  Modes::saveToBuffer(modesBuffer);
  SerialComs::write(modesBuffer);
}

void EditorConnection::sendModeCount()
{
  ByteStream buffer;
  buffer.serialize8(Modes::numModes());
  SerialComs::write(buffer);
}

void EditorConnection::sendCurMode()
{
  ByteStream modeBuffer;
  Mode *cur = Modes::curMode();
  if (!cur) {
    // ??
    return;
  }
  if (!cur->saveToBuffer(modeBuffer)) {
    // ??
    return;
  }
  SerialComs::write(modeBuffer);
}

void EditorConnection::sendCurModeVL()
{
#if VL_ENABLE_SENDER == 1
  m_state = STATE_TRANSMIT_MODE_VL;
#endif
}

ReturnCode EditorConnection::sendBrightness()
{
  ByteStream brightnessBuf;
  if (!brightnessBuf.serialize8(Leds::getBrightness())) {
    return RV_FAIL;
  }
  SerialComs::write(brightnessBuf);
  return RV_OK;
}

ReturnCode EditorConnection::receiveBuffer(ByteStream &buffer)
{
  // need at least the buffer size first
  uint32_t size = 0;
  if (m_receiveBuffer.size() < sizeof(size)) {
    // wait, not enough data available yet
    return RV_WAIT;
  }
  // grab the size out of the start
  m_receiveBuffer.resetUnserializer();
  size = m_receiveBuffer.peek32();
  if (m_receiveBuffer.size() < (size + sizeof(size))) {
    // don't unserialize yet, not ready
    return RV_WAIT;
  }
  // okay unserialize now, first unserialize the size
  if (!m_receiveBuffer.consume32(&size)) {
    // this shouldn't fail!
    return RV_FAIL;
  }
  // create a new ByteStream that will hold the full buffer of data
  if (!buffer.init(size)) {
    return RV_FAIL;
  }
  // then copy everything from the receive buffer into the rawdata
  // which is going to overwrite the crc/size/flags of the ByteStream
  if (!m_receiveBuffer.consume(size, buffer.rawData())) {
    return RV_FAIL;
  }
  buffer.sanity();
  if (!buffer.checkCRC()) {
    buffer.clear();
    // return true otherwise we'll get locked in this state forever
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::receiveModes()
{
  // create a new ByteStream that will hold the full buffer of data
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  if (!Modes::loadFromBuffer(buf) || !Modes::saveStorage()) {
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::receiveModeCount()
{
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  // unserialize the mode count
  if (!buf.consume8(&m_numModesToReceive)) {
    return RV_FAIL;
  }
  if (m_numModesToReceive > MAX_MODES) {
    return RV_FAIL;
  }
  // good mode count
  return RV_OK;
}

ReturnCode EditorConnection::receiveMode()
{
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  // unserialize the mode into the demo mode
  if (!Modes::addModeFromBuffer(buf)) {
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::receiveDemoMode()
{
  // create a new ByteStream that will hold the full buffer of data
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  // unserialize the mode into the demo mode
  if (!m_previewMode.loadFromBuffer(buf)) {
    // failure
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::receiveMessage(const char *message)
{
  size_t len = strlen(message);
  uint8_t byte = 0;
  // wait for the editor to ack the idle
  if (m_receiveBuffer.size() < len) {
    return RV_WAIT;
  }
  if (memcmp(m_receiveBuffer.data(), message, len) != 0) {
    return RV_FAIL;
  }
  for (size_t i = 0; i < len; ++i) {
    if (!m_receiveBuffer.consume8(&byte)) {
      return RV_FAIL;
    }
  }
  // we have now received at least one command, do not allow resetting
  m_allowReset = false;
  return RV_OK;
}

ReturnCode EditorConnection::receiveBrightness()
{
  // create a new ByteStream that will hold the full buffer of data
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    // RV_WAIT or RV_FAIL
    return m_rv;
  }
  if (!buf.size()) {
    // failure
    return RV_FAIL;
  }
  uint8_t brightness = 255;
  if (!buf.consume8(&brightness) || brightness == 0) {
    // they should never send 0 brightness
    return RV_FAIL;
  }
  if (brightness > 0) {
    Leds::setBrightness(brightness);
    Modes::saveHeader();
  }
  return RV_OK;
}

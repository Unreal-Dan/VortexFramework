#include "EditorConnection.h"

#include "../../Patterns/PatternArgs.h"
#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Storage/Storage.h"
#include "../../Wireless/VLSender.h"
#include "../../Wireless/VLReceiver.h"
#include "../../Time/TimeControl.h"
#include "../../Time/Timings.h"
#include "../../Colors/Colorset.h"
#include "../../Modes/DuoDefaultModes.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../UPDI/updi.h"
#include "../../Log/Log.h"

#include <string.h>

EditorConnection::EditorConnection(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_state(STATE_DISCONNECTED),
  m_timeOutStartTime(0),
  m_chromaModeIdx(0),
  m_allowReset(true),
  m_previousModeIndex(0),
  m_numModesToReceive(0),
  m_rv(RV_OK),
  m_curStep(0),
  m_firmwareSize(0),
  m_firmwareOffset(0),
  m_backupModes(true),
  m_backupModeNum(0)
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

void EditorConnection::onShortClickM()
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

void EditorConnection::onLongClickM()
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
  { EDITOR_VERB_LISTEN_VL, STATE_LISTEN_MODE_VL },
  { EDITOR_VERB_SET_GLOBAL_BRIGHTNESS, STATE_SET_GLOBAL_BRIGHTNESS },
  { EDITOR_VERB_GET_GLOBAL_BRIGHTNESS, STATE_GET_GLOBAL_BRIGHTNESS },
  { EDITOR_VERB_PULL_CHROMA_HDR, STATE_PULL_HEADER_CHROMALINK },
  { EDITOR_VERB_PUSH_CHROMA_HDR, STATE_PUSH_HEADER_CHROMALINK },
  { EDITOR_VERB_PULL_CHROMA_MODE, STATE_PULL_MODE_CHROMALINK },
  { EDITOR_VERB_PUSH_CHROMA_MODE, STATE_PUSH_MODE_CHROMALINK },
  { EDITOR_VERB_FLASH_FIRMWARE, STATE_CHROMALINK_FLASH_FIRMWARE },
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
    if (!SerialComs::isConnectedReal()) {
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
  //  Receive Mode from Duo
  case STATE_LISTEN_MODE_VL:
    // immediately load the mode and send it now
    VLReceiver::beginReceiving();
    m_state = STATE_LISTEN_MODE_VL_LISTEN;
    break;
  case STATE_LISTEN_MODE_VL_LISTEN:
    // immediately load the mode and send it now
    showReceiveModeVL();
    if (receiveModeVL() == RV_WAIT) {
      break;
    }
    m_state = STATE_LISTEN_MODE_VL_DONE;
    break;
  case STATE_LISTEN_MODE_VL_DONE:
    // done transmitting
    SerialComs::write(EDITOR_VERB_LISTEN_VL_ACK);
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

  // -------------------------------
  //  Get Chromalinked Duo Header
  case STATE_PULL_HEADER_CHROMALINK:
    if (pullHeaderChromalink() == RV_FAIL) {
      Leds::holdAll(RGB_RED);
    }
    // done
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Get Chromalinked Duo Mode
  case STATE_PULL_MODE_CHROMALINK:
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    m_state = STATE_PULL_MODE_CHROMALINK_SEND;
    break;
  case STATE_PULL_MODE_CHROMALINK_SEND:
    // send the stuff
    if (pullModeChromalink() == RV_WAIT) {
      // error?
      break;
    }
    // done
    m_curStep = 0;
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Set Chromalinked Duo Header
  case STATE_PUSH_HEADER_CHROMALINK:
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_PUSH_HEADER_CHROMALINK_RECEIVE;
    break;
  case STATE_PUSH_HEADER_CHROMALINK_RECEIVE:
    // receive the modes into the receive buffer
    if (pushHeaderChromalink() == RV_WAIT) {
      break;
    }
    // the trick is to send header after the modes so the reset comes at the end
    UPDI::reset();
    UPDI::disable();
    // success modes were received send the done
    SerialComs::write(EDITOR_VERB_PUSH_CHROMA_HDR_ACK);
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Set Chromalinked Duo Mode
  case STATE_PUSH_MODE_CHROMALINK:
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_PUSH_MODE_CHROMALINK_RECEIVE_IDX;
    break;
  case STATE_PUSH_MODE_CHROMALINK_RECEIVE_IDX:
    if (receiveModeIdx(m_chromaModeIdx) == RV_WAIT) {
      // just wait
      break;
    }
    SerialComs::write(EDITOR_VERB_READY);
    m_state = STATE_PUSH_MODE_CHROMALINK_RECEIVE;
    break;
  case STATE_PUSH_MODE_CHROMALINK_RECEIVE:
    if (pushModeChromalink() == RV_WAIT) {
      // not done keep going
      break;
    }
    SerialComs::write(EDITOR_VERB_PUSH_CHROMA_MODE_ACK);
    // done
    m_state = STATE_IDLE;
    break;

  // -------------------------------
  //  Flash Chromalinked Duo
  case STATE_CHROMALINK_FLASH_FIRMWARE:
    // editor requested to push modes, clear first and reset first
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_RECEIVE_SIZE;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_RECEIVE_SIZE:
    if (receiveFirmwareSize(m_firmwareSize) == RV_WAIT) {
      // continue waiting
      break;
    }
    m_curStep = 0;
    m_firmwareOffset = 0;
    m_backupModeNum = 0;
    Leds::setAll(RGB_ORANGE3);
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_BACKUP_MODES;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_BACKUP_MODES:
    if (backupDuoModes() == RV_WAIT) {
      // not done yet keep going
      break;
    }
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_ERASE_MEMORY;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_ERASE_MEMORY:
    Leds::setAll(RGB_CYAN0);
    UPDI::eraseMemory();
    SerialComs::write(EDITOR_VERB_READY);
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_FLASH_CHUNKS;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_FLASH_CHUNKS:
    // receive and write a chunk of firwmare
    if (writeDuoFirmware() == RV_WAIT) {
      // not done yet keep going
      break;
    }
    // done go to next state
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_RESTORE_MODES;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_RESTORE_MODES:
    // only once the entire firmware is written
    if (restoreDuoModes() == RV_WAIT) {
      // not done yet keep going
      break;
    }
    m_state = STATE_CHROMALINK_FLASH_FIRMWARE_DONE;
    break;
  case STATE_CHROMALINK_FLASH_FIRMWARE_DONE:
    m_firmwareOffset = 0;
    m_backupModeNum = 0;
    m_curStep = 0;
    // flag new firmware was written, so the duo turns on and writes it's save header
    UPDI::setFlagNewFirmware();
    // reset and disable updi because we are done
    UPDI::reset();
    UPDI::disable();
    // show green
    Leds::setAll(RGB_GREEN);
    SerialComs::write(EDITOR_VERB_FLASH_FIRMWARE_DONE);
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
    if (m_curStep == 0) {
      m_previewMode.play();
    }
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
  if (m_receiveBuffer.size() >= 512) {
    return;
  }
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

void EditorConnection::listenModeVL()
{
#if VL_ENABLE_SENDER == 1
  m_state = STATE_LISTEN_MODE_VL;
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
  // wait for the editor to ack the idle
  if (m_receiveBuffer.size() < len) {
    return RV_WAIT;
  }
  if (memcmp(m_receiveBuffer.data(), message, len) != 0) {
    return RV_FAIL;
  }
  if (!m_receiveBuffer.consume(len)) {
    return RV_FAIL;
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

ReturnCode EditorConnection::receiveModeVL()
{
  // if reveiving new data set our last data time
  if (VLReceiver::onNewData()) {
    m_timeOutStartTime = Time::getCurtime();
    // if our last data was more than time out duration reset the recveiver
  } else if (m_timeOutStartTime > 0 && (m_timeOutStartTime + MAX_TIMEOUT_DURATION) < Time::getCurtime()) {
    VLReceiver::resetVLState();
    m_timeOutStartTime = 0;
    return RV_WAIT;
  }
  // check if the VLReceiver has a full packet available
  if (!VLReceiver::dataReady()) {
    // nothing available yet
    return RV_WAIT;
  }
  DEBUG_LOG("Mode ready to receive! Receiving...");
  // receive the VL mode into the current mode
  if (!VLReceiver::receiveMode(&m_previewMode)) {
    ERROR_LOG("Failed to receive mode");
    return RV_FAIL;
  }
  DEBUG_LOGF("Success receiving mode: %u", m_previewMode.getPatternID());
  if (!Modes::updateCurMode(&m_previewMode)) {
    return RV_FAIL;
  }
  ByteStream modeBuffer;
  if (!m_previewMode.saveToBuffer(modeBuffer)) {
    return RV_FAIL;
  }
  SerialComs::write(modeBuffer);
  return RV_OK;
}

void EditorConnection::showReceiveModeVL()
{
  if (VLReceiver::isReceiving()) {
    // using uint32_t to avoid overflow, the result should be within 10 to 255
    //Leds::setAll(RGBColor(0, VLReceiver::percentReceived(), 0));
    Leds::setRange(LED_0, (LedPos)(VLReceiver::percentReceived() / 10), RGB_GREEN6);
    Leds::setRange(LED_10, (LedPos)(LED_10 + (VLReceiver::percentReceived() / 10)), RGB_GREEN6);
  } else {
    Leds::setAll(RGB_WHITE0);
  }
}

ReturnCode EditorConnection::receiveModeIdx(uint8_t &idx)
{
  // need at least the buffer size first
  if (m_receiveBuffer.size() < sizeof(idx)) {
    // wait, not enough data available yet
    return RV_WAIT;
  }
  m_receiveBuffer.resetUnserializer();
  // okay unserialize now, first unserialize the size
  if (!m_receiveBuffer.consume8(&idx)) {
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::receiveFirmwareSize(uint32_t &size)
{
  // need at least the buffer size first
  if (m_receiveBuffer.size() < sizeof(size)) {
    // wait, not enough data available yet
    return RV_WAIT;
  }
  m_receiveBuffer.resetUnserializer();
  // okay unserialize now, first unserialize the size
  if (!m_receiveBuffer.consume32(&size)) {
    return RV_FAIL;
  }
  return RV_OK;
}

ReturnCode EditorConnection::pullHeaderChromalink()
{
  // first read the duo save header
  ByteStream duoHeader;
  // doesn't matter if reading the header fails, we still need to send it
  bool success = UPDI::readHeader(duoHeader);
  // TODO: should these be here?
  UPDI::reset();
  UPDI::disable();
  // TODO: check version stuff? not really any need yet
  //DuoHeader *pHeader = (DuoHeader *)duoHeader.data();
  // send whatever we read, might be empty buffer if it failed
  SerialComs::write(duoHeader);
  // return whether reading the header was successful
  return success ? RV_OK : RV_FAIL;
}

ReturnCode EditorConnection::pushHeaderChromalink()
{
  // wait for the header then write it via updi
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  if (!UPDI::writeHeader(buf)) {
    return RV_FAIL;
  }
  UPDI::reset();
  UPDI::disable();
  return RV_OK;
}

// pull/push through the chromalink
ReturnCode EditorConnection::pullModeChromalink()
{
  // try to receive the mode index
  uint8_t modeIdx = 0;
  m_rv = receiveModeIdx(modeIdx);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  // only 9 modes on duo, maybe this should be a macro or something
  if (modeIdx >= 9) {
    return RV_FAIL;
  }
  ByteStream modeBuffer;
  // same doesn't matter if this fails still need to send
  bool success = UPDI::readMode(modeIdx, modeBuffer);
  UPDI::reset();
  UPDI::disable();
  // lol just use the mode index as the radial to set
  Leds::setRadial((Radial)modeIdx, success ? RGB_GREEN4 : RGB_RED4);
  if (!success) {
    // just send back a 0 if it failed
    modeBuffer.init(1);
    modeBuffer.serialize8(0);
  }
  // send the mode, could be empty buffer if reading failed
  SerialComs::write(modeBuffer);
  return success ? RV_OK : RV_FAIL;
}

ReturnCode EditorConnection::pushModeChromalink()
{
  // lol just use the mode index as the radial to set
  Leds::setRadials(RADIAL_0, (Radial)m_chromaModeIdx, RGB_GREEN4);
  // wait for the mode then write it via updi
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  if (!UPDI::writeMode(m_chromaModeIdx, buf)) {
    return RV_FAIL;
  }
  // the trick is to send header after the modes so the reset comes at the end
  UPDI::reset();
  UPDI::disable();
  return RV_OK;
}

ReturnCode EditorConnection::backupDuoModes()
{
  // TODO: Proper modes backup
#if 0
  if (m_backupModeNum == 9) {
    // reset counter for the restore step later
    m_backupModeNum = 0;
    // done
    return RV_OK;
  }
  // backing up the first mode
  if (m_backupModeNum == 0) {
    // default this to true to begin
    m_backupModes = true;
    // double check the version and valid header before backing up modes
    ByteStream duoHeader;
    if (UPDI::readHeader(duoHeader) && duoHeader.size() >= 5) {
      DuoHeader &duoHeader = UPDI::lastSaveHeader();
      if (duoHeader.vMajor < 1 || duoHeader.vMinor < 3) {
        // turn off mode backup the version isn't high enough
        m_backupModes = false;
      }
    }
  }
  // may use the defaults if backing up fails, default is whether backup is enabled
  bool useDefault = !m_backupModes;
  if (m_backupModes) {
    ByteStream &cur = m_modeBackups[m_backupModeNum];
    // if the mode cannot be loaded, or if it's CRC is bad then just use the default
    if (!UPDI::readMode(m_backupModeNum, cur) || !cur.checkCRC() || !cur.size()) {
      useDefault = true;
    }
  }
  // if not backing up, or backup failed, then store the default mode data in
  // the backup because we will always write out the backups after flashing
  if (useDefault) {
    m_modeBackups[m_backupModeNum].init(duo_default_sizes[m_backupModeNum], duo_default_modes[m_backupModeNum]);
  }
  Leds::setRadials(RADIAL_0, (Radial)m_backupModeNum, useDefault ? RGB_CYAN0 : RGB_PURPLE);
  // go to next mode
  m_backupModeNum++;
  return RV_WAIT;
#endif
  // for now we just use the defaults
  for (uint32_t i = 0; i < 9; ++i) {
    m_modeBackups[i].init(duo_default_sizes[i], duo_default_modes[i]);
  }
  return RV_OK;
}

ReturnCode EditorConnection::restoreDuoModes()
{
  Leds::setRadials(RADIAL_0, (Radial)m_backupModeNum, RGB_CYAN4);
  if (m_backupModeNum == 9) {
    // reset counter for the restore step later
    m_backupModeNum = 0;
    // done
    return RV_OK;
  }
  // each pass write out the backups, these may be the defaults
  if (!UPDI::writeMode(m_backupModeNum, m_modeBackups[m_backupModeNum])) {
    return RV_FAIL;
  }
  // go to next mode
  m_backupModeNum++;
  return RV_WAIT;
}

ReturnCode EditorConnection::writeDuoFirmware()
{
  // render some progress, do it before updating the offset so it starts at 0
  Leds::setAll(RGB_YELLOW0);
  Leds::setRadials(RADIAL_0, (Radial)((m_firmwareOffset / (float)m_firmwareSize) * RADIAL_COUNT), RGB_GREEN3);
  // first pass and backup modes is enabled
  if (m_firmwareOffset >= m_firmwareSize) {
    // done
    return RV_OK;
  }
  // wait for the mode then write it via updi
  ByteStream buf;
  m_rv = receiveBuffer(buf);
  if (m_rv != RV_OK) {
    return m_rv;
  }
  // write out the firmware and record it if successful
  if (!UPDI::writeFirmware(m_firmwareOffset, buf)) {
    // big error? this shouldn't happen
    return RV_FAIL;
  }
  m_firmwareOffset += buf.size();
  SerialComs::write(EDITOR_VERB_FLASH_FIRMWARE_ACK);
  // not done  yet
  return RV_WAIT;
}

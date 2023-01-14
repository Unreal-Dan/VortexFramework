#include "EditorConnection.h"

#include "../../Serial/ByteStream.h"
#include "../../Serial/Serial.h"
#include "../../Storage/Storage.h"
#include "../../Modes/ModeBuilder.h"
#include "../../Time/TimeControl.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

EditorConnection::EditorConnection() :
  Menu(),
  m_state(STATE_DISCONNECTED),
  m_pDemoMode(nullptr)
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

  DEBUG_LOG("Entering Editor Connection");
  return true;
}

bool EditorConnection::receiveMessage(const char *message)
{
  // wait for the editor to ack the idle
  if (m_receiveBuffer.size() == 0) {
    return false;
  }
  size_t len = strlen(message);
  uint8_t byte = 0;
  for (uint32_t i = 0; i < len; ++i) {
    if (m_receiveBuffer.peek8() != message[i]) {
      return false;
    }
    m_receiveBuffer.unserialize(&byte);
    // double check
    if (byte != message[i]) {
      return false;
    }
  }
  // if everything was read out, reset
  if (m_receiveBuffer.unserializerAtEnd()) {
    m_receiveBuffer.clear();
  }
  return true;
}

void EditorConnection::clearDemo()
{
  if (!m_pDemoMode) {
    return;
  }
  delete m_pDemoMode;
  m_pDemoMode = nullptr;
}

bool EditorConnection::run()
{
  if (!Menu::run()) {
    return false;
  }
  // show the editor
  showEditor();
  // receive any data from serial into the receive buffer
  receiveData();
  // operate on the state of the editor connection
  switch (m_state) {
  case STATE_DISCONNECTED:
    // not connected yet so check for connections
    if (!SerialComs::isConnected()) {
      if (!SerialComs::checkSerial()) {
        // no connection found just continue waiting
        return true;
      }
    }
    // a connection was found, say hello
    m_state = STATE_GREETING;
    break;
  case STATE_GREETING:
    m_receiveBuffer.clear();
    // send the hello greeting with our version number and build time
    SerialComs::write(EDITOR_VERB_GREETING);
    m_state = STATE_IDLE;
    break;
  case STATE_IDLE:
    // parse the receive buffer for any commands from the editor
    handleCommand();
    break;
  case STATE_PULL_MODES:
    // editor requested pull modes, send the modes
    sendModes();
    m_state = STATE_PULL_MODES_SEND;
    break;
  case STATE_PULL_MODES_SEND:
    // recive the send modes ack from the editor
    if (receiveMessage(EDITOR_VERB_PULL_MODES_DONE)) {
      m_state = STATE_PULL_MODES_DONE;
    }
    break;
  case STATE_PULL_MODES_DONE:
    m_receiveBuffer.clear();
    // send our acknowledgement that the modes were sent
    SerialComs::write(EDITOR_VERB_PULL_MODES_ACK);
    // go idle
    m_state = STATE_IDLE;
    break;
  case STATE_PUSH_MODES:
    // editor requested to push modes, clear first and reset first
    m_receiveBuffer.clear();
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_PUSH_MODES_RECEIVE;
    break;
  case STATE_PUSH_MODES_RECEIVE:
    // receive the modes into the receive buffer
    if (receiveModes()) {
      // success modes were received send the done
      m_state = STATE_PUSH_MODES_DONE;
    }
    break;
  case STATE_PUSH_MODES_DONE:
    // say we are done
    m_receiveBuffer.clear();
    SerialComs::write(EDITOR_VERB_PUSH_MODES_ACK);
    m_state = STATE_IDLE;
    break;
  case STATE_DEMO_MODE:
    // editor requested to push modes, clear first and reset first
    m_receiveBuffer.clear();
    // now say we are ready
    SerialComs::write(EDITOR_VERB_READY);
    // move to receiving
    m_state = STATE_DEMO_MODE_RECEIVE;
    break;
  case STATE_DEMO_MODE_RECEIVE:
    // receive the modes into the receive buffer
    if (receiveDemoMode()) {
      // success modes were received send the done
      m_state = STATE_DEMO_MODE_DONE;
    }
    break;
  case STATE_DEMO_MODE_DONE:
    // say we are done
    m_receiveBuffer.clear();
    SerialComs::write(EDITOR_VERB_DEMO_MODE_ACK);
    m_state = STATE_IDLE;
    break;
  case STATE_CLEAR_DEMO:
    clearDemo();
    m_receiveBuffer.clear();
    SerialComs::write(EDITOR_VERB_CLEAR_DEMO_ACK);
    m_state = STATE_IDLE;
  }
  return true;
}

// handlers for clicks
void EditorConnection::onShortClick()
{
  // reset, this won't actually disconnect the com port
  m_state = STATE_DISCONNECTED;
  // clear the demo
  clearDemo();
}

void EditorConnection::onLongClick()
{
  leaveMenu();
}

void EditorConnection::showEditor()
{
  switch (m_state) {
  case STATE_DISCONNECTED:
    Leds::clearAll();
    // gradually fill from thumb to pinkie
    Leds::blinkAll(Time::getCurtime(), 250, 150, RGB_BLANK);
    break;
  case STATE_IDLE:
    if (m_pDemoMode) {
      // thats all
      m_pDemoMode->play();
    } else {
      Leds::clearAll();
      Leds::blinkAll(Time::getCurtime(), 250, 150, RGB_BLUE);
    }
    break;
  case STATE_PULL_MODES:
  case STATE_PULL_MODES_SEND:
    Leds::clearAll();
    Leds::blinkAll(Time::getCurtime(), 250, 150, RGB_CYAN);
    break;
  case STATE_PUSH_MODES:
    Leds::clearAll();
    Leds::blinkAll(RGB_WHITE);
    break;
  case STATE_PUSH_MODES_RECEIVE:
    Leds::clearAll();
    Leds::blinkAll(RGB_BLANK);
    break;
  case STATE_PUSH_MODES_DONE:
    Leds::clearAll();
    Leds::blinkAll(RGB_ORANGE);
    break;
  case STATE_DEMO_MODE:
    Leds::clearAll();
    Leds::blinkAll(RGB_YELLOW);
    break;
  case STATE_DEMO_MODE_RECEIVE:
    Leds::clearAll();
    Leds::blinkAll(RGB_PURPLE);
    break;
  case STATE_DEMO_MODE_DONE:
    Leds::clearAll();
    Leds::blinkAll(RGB_BLUE);
    break;
  default:
    Leds::setAll(RGB_BLANK);
    break;
  }
}

// override showExit so it isn't displayed on thumb
void EditorConnection::showExit()
{
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

bool EditorConnection::receiveModes()
{
  // need at least the buffer size first
  uint32_t size = 0;
  if (m_receiveBuffer.size() < sizeof(size)) {
    // wait, not enough data available yet
    return false;
  }
  // grab the size out of the start
  m_receiveBuffer.resetUnserializer();
  size = m_receiveBuffer.peek32();
  if (m_receiveBuffer.size() < (size + sizeof(size))) {
    // don't unserialize yet, not ready
    return false;
  }
  // okay unserialize now, first unserialize the size
  m_receiveBuffer.unserialize(&size);
  // todo: this is kinda jank but w/e
  memmove(m_receiveBuffer.rawData(),
    ((uint8_t *)m_receiveBuffer.data()) + sizeof(size),
    m_receiveBuffer.size());
  Modes::loadFromBuffer(m_receiveBuffer);
  Modes::saveStorage();
  return true;
}

bool EditorConnection::receiveDemoMode()
{
  // need at least the buffer size first
  uint32_t size = 0;
  if (m_receiveBuffer.size() < sizeof(size)) {
    // wait, not enough data available yet
    return false;
  }
  // grab the size out of the start
  m_receiveBuffer.resetUnserializer();
  size = m_receiveBuffer.peek32();
  if (m_receiveBuffer.size() < (size + sizeof(size))) {
    // don't unserialize yet, not ready
    return false;
  }
  // okay unserialize now, first unserialize the size
  m_receiveBuffer.unserialize(&size);
  // todo: this is kinda jank but w/e
  memmove(m_receiveBuffer.rawData(),
    ((uint8_t *)m_receiveBuffer.data()) + sizeof(size),
    m_receiveBuffer.size());
  if (!m_receiveBuffer.checkCRC()) {
    Leds::setAll(RGB_WHITE);
    // bad crc
    return false;
  }
  // unserialize the mode into the demo mode
  clearDemo();
  m_pDemoMode = ModeBuilder::loadFromBuffer(m_receiveBuffer);
  return true;
}

void EditorConnection::handleCommand()
{
  if (receiveMessage(EDITOR_VERB_PULL_MODES)) {
    m_state = STATE_PULL_MODES;
  } else if (receiveMessage(EDITOR_VERB_PUSH_MODES)) {
    m_state = STATE_PUSH_MODES;
  } else if (receiveMessage(EDITOR_VERB_DEMO_MODE)) {
    m_state = STATE_DEMO_MODE;
  } else if (receiveMessage(EDITOR_VERB_CLEAR_DEMO)) {
    m_state = STATE_CLEAR_DEMO;
  }
}

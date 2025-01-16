#ifndef EDITOR_CONNECTION_H
#define EDITOR_CONNECTION_H

#include "../Menu.h"

#include "../../Serial/ByteStream.h"
#include "../../Modes/Mode.h"

class EditorConnection : public Menu
{
public:
  EditorConnection(const RGBColor &col, bool advanced);
  ~EditorConnection();

  bool init() override;
  MenuAction run() override;

  // broadcast the current preview mode over VL
  void sendCurModeVL();
  void listenModeVL();

  // pull/push through the chromalink
  bool pullHeaderChromalink();
  bool pushHeaderChromalink();
  bool pullModeChromalink();
  bool pushModeChromalink();

  bool writeDuoFirmware();
  bool backupDuoModes();
  bool restoreDuoModes();

  // handlers for clicks
  void onShortClickM() override;
  void onLongClickM() override;

  // menu conn
  void leaveMenu(bool doSave = false) override;

private:
  void handleCommand();
  void showEditor();
  void receiveData();
  void handleState();
  void sendModes();
  void sendModeCount();
  void sendCurMode();
  bool receiveBuffer(ByteStream &buffer);
  bool receiveFirmwareChunk(ByteStream &buffer);
  bool receiveModes();
  bool receiveModeCount();
  bool receiveMode();
  bool receiveDemoMode();
  bool receiveMessage(const char *message);
  void clearDemo();
  void receiveModeVL();
  void showReceiveModeVL();
  bool receiveModeIdx(uint8_t &idx);
  bool receiveFirmwareSize(uint32_t &idx);

  enum EditorConnectionState {
    // the editor is not connected
    STATE_DISCONNECTED,

    // Sending the greeting message
    STATE_GREETING,

    // entirely idle, waiting for commands
    STATE_IDLE,

    // editor pulls the modes from device, then wait for the sent modes ack
    STATE_PULL_MODES,
    STATE_PULL_MODES_SEND,
    STATE_PULL_MODES_DONE,

    // editor pushes modes to device, then waits for done
    STATE_PUSH_MODES,
    STATE_PUSH_MODES_RECEIVE,
    STATE_PUSH_MODES_DONE,

    // editor pushes mode to device for demo while idle
    STATE_DEMO_MODE,
    STATE_DEMO_MODE_RECEIVE,
    STATE_DEMO_MODE_DONE,

    // editor tells device to clear the demo preview, device acknowledge
    STATE_CLEAR_DEMO,

    // transmit the mode over visible light
    STATE_TRANSMIT_MODE_VL,
    STATE_TRANSMIT_MODE_VL_DONE,

    // receive a mode over VL
    STATE_LISTEN_MODE_VL,
    STATE_LISTEN_MODE_VL_DONE,

    // editor pulls the modes from device (safer version)
    STATE_PULL_EACH_MODE,
    STATE_PULL_EACH_MODE_COUNT,
    STATE_PULL_EACH_MODE_SEND,
    STATE_PULL_EACH_MODE_WAIT,
    STATE_PULL_EACH_MODE_DONE,

    // editor pushes modes to device (safer version)
    STATE_PUSH_EACH_MODE,
    STATE_PUSH_EACH_MODE_COUNT,
    STATE_PUSH_EACH_MODE_RECEIVE,
    STATE_PUSH_EACH_MODE_WAIT,
    STATE_PUSH_EACH_MODE_DONE,

    // pull the header from the chromalinked duo
    STATE_PULL_HEADER_CHROMALINK,

    // pull a mode from the chromalinked duo
    STATE_PULL_MODE_CHROMALINK,
    STATE_PULL_MODE_CHROMALINK_SEND,

    // push the header to the chromalinked duo
    STATE_PUSH_HEADER_CHROMALINK,
    STATE_PUSH_HEADER_CHROMALINK_RECEIVE,
    
    // push a mode to the chromalinked duo
    STATE_PUSH_MODE_CHROMALINK,
    STATE_PUSH_MODE_CHROMALINK_RECEIVE_IDX,
    STATE_PUSH_MODE_CHROMALINK_RECEIVE,

    // flash the firmware of the chromalinked duo
    STATE_CHROMALINK_FLASH_FIRMWARE,
    STATE_CHROMALINK_FLASH_FIRMWARE_RECEIVE_SIZE,
    STATE_CHROMALINK_FLASH_FIRMWARE_BACKUP_MODES,
    STATE_CHROMALINK_FLASH_FIRMWARE_ERASE_MEMORY,
    STATE_CHROMALINK_FLASH_FIRMWARE_FLASH_CHUNKS,
    STATE_CHROMALINK_FLASH_FIRMWARE_RESTORE_MODES,
    STATE_CHROMALINK_FLASH_FIRMWARE_DONE,

    // toggle whether flashing firmware will backup modes
    STATE_CHROMALINK_FLASH_FIRMWARE_TOGGLE_BACKUP,
  };

  // state of the editor
  EditorConnectionState m_state;
  // the data that is received
  ByteStream m_receiveBuffer;
  // receiver timeout
  uint32_t m_timeOutStartTime;
  // target chroma mode index for read/write
  uint8_t m_chromaModeIdx;
  // Whether at least one command has been received yet
  bool m_allowReset;
  // the mode index to return to after iterating the modes to send them
  uint8_t m_previousModeIndex;
  // the number of modes that should be received
  uint8_t m_numModesToReceive;

  // current step of transfer
  uint32_t m_curStep;
  // firmware size for flashing duo
  uint32_t m_firmwareSize;
  // how much firmware written so far
  uint32_t m_firmwareOffset;

  // whether to backup duo modes on firmware update
  bool m_backupModes;
  // backups of duo modes when flashing firmware
  ByteStream m_modeBackups[9];
  // counter for reading/writing modes during firmware flash
  uint8_t m_backupModeNum;
};

#endif

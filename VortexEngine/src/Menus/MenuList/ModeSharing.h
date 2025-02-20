#ifndef MODE_SHARING_H
#define MODE_SHARING_H

#include "../Menu.h"

class ModeSharing : public Menu
{
public:
  ModeSharing(const RGBColor &col, bool advanced);
  ~ModeSharing();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClickM() override;
  void onShortClickL() override;
  void onShortClickR() override;
  void onLongClickM() override;

private:
  void beginSendingVL();
  void continueSendingVL();
  void beginReceivingVL();
  void receiveModeVL();

  void beginSendingIR();
  void continueSendingIR();
  void beginReceivingIR();
  void receiveModeIR();

  void showSendModeVL();
  void showSendModeIR();
  void showReceiveModeVL();
  void showReceiveModeIR();

  void switchVLIR();

  enum class ModeShareState {
    SHARE_RECEIVE_VL,
    SHARE_SEND_VL,
    SHARE_RECEIVE_IR,
    SHARE_SEND_IR,
  };

  ModeShareState m_sharingMode;

  // the start time when checking for timing out
  uint32_t m_timeOutStartTime;
  uint32_t m_lastSendTime;

  // whether to end the next send and go back to receive
  bool m_shouldEndSend;
};

#endif

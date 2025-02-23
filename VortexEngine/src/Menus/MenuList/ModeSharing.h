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
  void onShortClick() override;
  void onLongClick() override;

private:
  void beginSending();
  void continueSending();
  void receiveMode();

  void showSendMode();
  void showReceiveMode();

  // override showExit so it isn't displayed on thumb
  virtual void showExit() override;

  enum class ModeShareState {
    SHARE_SEND,     // send mode
    SHARE_RECEIVE,  // receive mode
  };

  ModeShareState m_sharingMode;
  // last time data was sent
  uint64_t m_lastActionTime;

  // the start time when checking for timing out
  uint32_t m_timeOutStartTime;
};

#endif

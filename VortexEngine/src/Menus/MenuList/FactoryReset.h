#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#include "../Menu.h"

class FactoryReset : public Menu
{
public:
  FactoryReset(const RGBColor &col, bool advanced);
  ~FactoryReset();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClickL() override;
  void onShortClickR() override;
  void onLongClickM() override;

private:
  void showReset();
};

#endif

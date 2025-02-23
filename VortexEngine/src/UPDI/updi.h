#ifndef UPDI_H
#define UPDI_H

#include <inttypes.h>

#include "../VortexConfig.h"

// this is a replication of what the Duo save header looks like
// at the time of writing this, it may grow and contain more info
// but it should still be backwards compatible because of max size
struct DuoHeader {
  uint8_t vMajor;
  uint8_t vMinor;
  uint8_t globalFlags;
  uint8_t numModes;
  uint8_t idk;
  uint8_t vBuild;
  // 15 bytes total
  uint8_t unused[9];
};

class ByteStream;

class UPDI
{
public:
  static bool init();
  static void cleanup();

  static uint8_t isConnected();

  // read the duo save header over updi
  static bool readHeader(ByteStream &headerBuffer);
  static bool readMode(uint8_t idx, ByteStream &modeBuffer);

  static bool writeHeader(ByteStream &headerBuffer);
  static bool writeMode(uint8_t idx, ByteStream &modeBuffer);

  static bool writeFirmware(uint32_t offset, ByteStream &firmwareBuffer);
  static bool setFlagNewFirmware();

  static bool eraseMemory();
  static bool reset();

  // end the updi connection and reset the chip
  static bool disable();

  // the data from the last duo header that was loaded
  static DuoHeader &lastSaveHeader() { return m_lastSaveHeader; }

private:

#ifdef VORTEX_EMBEDDED
  // really old duos like 1.0.0
  static bool readHeaderLegacy1(ByteStream &headerBuffer);
  // kinda old duos like 1.2.0 to 1.3.0
  static bool readHeaderLegacy2(ByteStream &headerBuffer);

  static bool writeModeEeprom(uint8_t idx, ByteStream &modeBuffer);
  static bool writeModeFlash(uint8_t idx, ByteStream &modeBuffer);

  static bool writePage(uint16_t addr, const uint8_t *buf, uint16_t pageSize = 128);
  static bool readPage(uint8_t addr, uint8_t *buf, uint16_t pageSize = 128);

  static bool writeFlashPage(uint16_t addr, const uint8_t *buf);
  static bool readFlashPage(uint8_t addr, uint8_t *buf);

  static bool writeEepromPage(uint16_t addr, const uint8_t *buf);
  static bool readEepromPage(uint8_t addr, uint8_t *buf);

  // *** Base Addresses ***
  enum base
  {
    NVM_base = 0x1000,       /* Base address of the NVM controller */
    Sig_base = 0x1100,       /* Base address of the signature */
    Fuse_base = 0x1280,       /* Base address of the fuses */
    User_base = 0x1300,       /* Base address of the User Row EEPROM */
    EEPROM_base = 0x1400        /* Base address of the main EEPROM */
  };

  enum cmnd
  {
    NVM_CMD_NOP,      // Does nothing
    NVM_CMD_WP,       // Write page buffer to memory
    NVM_CMD_ER,       // Erase page
    NVM_CMD_ERWP,     // Erase and write page
    NVM_CMD_PBC,      // Page buffer clear
    NVM_CMD_CHER,     // Chip erase: erase Flash and EEPROM
    NVM_CMD_EEER,     // EEPROM Erase
    NVM_CMD_WFU       // Write fuse
  };

  enum cs_reg : uint8_t
  {
    Status_A, Status_B, Control_A, Control_B,
    Reg_4, Reg_5, Reg_6, ASI_Key_Status,
    ASI_Reset_Request, ASI_Control_A, ASI_System_Control_A, ASI_System_Status,
    ASI_CRC_Status, Reg_13, Reg_14, Reg_15
  };

  // *** NVM Registers (offsets from NVN_base are enum default values) ***
  enum nvm_reg
  {
    NVM_CTRLA,
    NVM_CTRLB,
    NVM_STATUS,
    NVM_INTCTRL,
    NVM_INTFLAGS,
    NVM_Reg_5,
    NVM_DATA_lo,
    NVM_DATA_hi,
    NVM_ADDR_lo,
    NVM_ADDR_hi
  };

  // *** NVM Commands (write to CTRLA to execute) ***
  enum nvm_cmd
  {
    NVM_NOP,      /* Does nothing */
    NVM_WP,       /* Write page buffer to memory */
    NVM_ER,       /* Erase page */
    NVM_ERWP,     /* Erase and write page */
    NVM_PBC,      /* Page buffer clear */
    NVM_CHER,     /* Chip erase: erase Flash and EEPROM */
    NVM_EEER,     /* EEPROM Erase */
    NVM_WFU       /* Write fuse */
  };

  static bool enterProgrammingMode();

  static void resetOn();
  static bool resetOff();

  static void sendByte(uint8_t byte);
  static uint8_t receiveByte();
  static void sendBreak();
  static void sendDoubleBreak();
  static void sendKey(const char *key);

  static uint8_t ldcs(cs_reg r);
  static void stcs(cs_reg r, uint8_t data);
  static void stptr_w(uint16_t addr);
  static void stptr_p(const uint8_t *addr_p, uint8_t n);
  static void stptr_l(uint32_t address);
  static void stptr_inc_16(uint8_t *data, uint16_t len);
  static void stinc_b(uint8_t data);
  static void stinc_b_noget(uint8_t data);
  static void stinc_w_noget(uint16_t data);
  static void rep(uint8_t repeats);
  static uint8_t ldinc_b();
  static uint8_t lds_b(uint16_t address);
  static uint8_t ld_b();
  static void sts_b(uint16_t address, uint8_t data);

  static void nvmCmd(nvm_cmd cmd) { UPDI::sts_b(NVM_base + NVM_CTRLA, cmd); }
  static void nvmWait() { while (UPDI::lds_b(NVM_base + NVM_STATUS) & 0x03); }

  template <uint8_t mask = 0xFF>
  static uint8_t cpu_mode()
  {
    uint8_t mode = ldcs(ASI_System_Status);
    return mode & mask;
  }

  static void sendEraseKey() { sendKey("NVMErase"); }
  static void sendProgKey() { sendKey("NVMProg "); }
  static void sendUserrowKey() { sendKey("NVMUs&te"); }
#endif

  static void copyLastSaveHeader(const ByteStream &srcData);

  // a copy of the last save header
  static DuoHeader m_lastSaveHeader;

  enum StorageType
  {
    // modern duos like 1.4.0+
    MODERN_STORAGE,
    // duos like 1.2.0. to 1.3..0
    LEGACY_STORAGE_1,
    // old duos like 1.0.0 to 1.2.0
    LEGACY_STORAGE_2,
  };

  // whether connected to a legacy duo
  static StorageType m_storageType;
};

#endif // UPDI_H

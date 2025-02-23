#include "Modes.h"
#include "DefaultModes.h"

#include "../VortexEngine.h"

#include "../Patterns/Pattern.h"

#include "../Serial/ByteStream.h"
#include "../Time/TimeControl.h"
#include "../Colors/Colorset.h"
#include "../Storage/Storage.h"
#include "../Buttons/Buttons.h"
#include "../Time/Timings.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

Modes::Modes(VortexEngine &engine) :
  m_engine(engine),
  m_curMode(0),
  m_numModes(0),
  m_pCurModeLink(nullptr),
  m_storedModes(nullptr),
  m_globalFlags(0),
  m_lastSwitchTime(0)
{
}
Modes::~Modes()
{
}

bool Modes::init()
{
#if MODES_TEST == 1
  Mode::test();
  test();
  return true;
#endif
  ByteStream headerBuffer;
  m_engine.storage().read(0, headerBuffer);
  unserializeSaveHeader(headerBuffer);
  m_loaded = false;
#ifdef VORTEX_LIB
  // enable the adv menus by default in vortex lib
  m_globalFlags |= MODES_FLAG_ADV_MENUS;
#endif
  return true;
}

void Modes::cleanup()
{
  clearModes();
}

bool Modes::load()
{
  if (m_loaded) {
    return true;
  }
  // try to load the saved settings or set defaults
  if (!loadStorage()) {
    if (!setDefaults()) {
      return false;
    }
    if (!saveStorage()) {
      return false;
    }
  }
  m_loaded = true;
  return true;
}

void Modes::play()
{
  if (!m_numModes) {
    // nothing to do just keep the leds cleared
    m_engine.leds().clearAll();
    return;
  }
  // should be able to initialize the current mode
  if (!m_pCurModeLink && !initCurMode()) {
    DEBUG_LOG("Error failed to load any modes!");
    return;
  }
  // shortclick cycles to the next mode
  if (m_engine.button().onShortClick()) {
    nextMode();
  }
  // play the current mode
  m_pCurModeLink->play();
}

// full save/load to/from buffer
bool Modes::saveToBuffer(ByteStream &modesBuffer)
{
  // first write out the header
  if (!serializeSaveHeader(modesBuffer)) {
    return false;
  }
  // serialize all modes data into the modesBuffer
  if (!serialize(modesBuffer)) {
    return false;
  }
  DEBUG_LOGF("Serialized all modes, uncompressed size: %u", modesBuffer.size());
  if (!modesBuffer.compress()) {
    return false;
  }
  return true;
}

// load modes from a save buffer
bool Modes::loadFromBuffer(ByteStream &modesBuffer)
{
  if (!modesBuffer.decompress()) {
    // failed to decompress?
    return false;
  }
  // read out the header first
  if (!unserializeSaveHeader(modesBuffer)) {
    return false;
  }
  // now just unserialize the list of modes
  if (!unserialize(modesBuffer)) {
    return false;
  }
  if (oneClickModeEnabled()) {
    // set the current mode to the startup mode
    switchToStartupMode();
  }
  return true;
}

bool Modes::saveHeader()
{
  ByteStream headerBuffer(MAX_MODE_SIZE);
  if (!serializeSaveHeader(headerBuffer)) {
    return false;
  }
  // serialize the number of modes
  if (!headerBuffer.serialize8(m_numModes)) {
    return false;
  }
  if (!m_engine.storage().write(0, headerBuffer)) {
    return false;
  }
  return true;
}

bool Modes::loadHeader()
{
  ByteStream headerBuffer;
  // only read storage if the modebuffer isn't filled
  if (!m_engine.storage().read(0, headerBuffer) || !headerBuffer.size()) {
    DEBUG_LOG("Empty buffer read from storage");
    // this kinda sucks whatever they had loaded is gone
    return false;
  }
  // this erases what is stored before we know whether there is data
  // but it's the easiest way to just re-load new data from storage
  clearModes();
  // read the header and load the data
  if (!unserializeSaveHeader(headerBuffer)) {
    return false;
  }
  // NOTE: We do not bother loading the number of modes because
  //       we can't really do anything with it anyway
  return true;
}

// NOTE: Flash storage is limited to about 10,000 writes so
//       use this function sparingly!
bool Modes::saveStorage()
{
  DEBUG_LOG("Saving modes...");
  saveHeader();
  // make sure the current mode is saved in case it has changed somehow
  saveCurMode();
  // uninstantiate cur mode so we have stack space to serialize
  if (m_pCurModeLink) {
    m_pCurModeLink->uninstantiate();
  }
  uint8_t i = 0;
  ModeLink *ptr = m_storedModes;
  while (ptr && i < MAX_MODES) {
    ByteStream modeBuffer(MAX_MODE_SIZE);
    // instantiate the mode temporarily
    Mode *mode = ptr->instantiate();
    if (!mode) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
    // serialize it into the target modes buffer
    if (!mode->serialize(modeBuffer)) {
      return false;
    }
    // just uninstansiate the mode after serializing
    ptr->uninstantiate();
    // next mode
    ptr = ptr->next();
    // now write this mode into a storage slot (skip first slot, that's header)
    if (!m_engine.storage().write(++i, modeBuffer)) {
      return false;
    }
  }
  // reinstanstiate the current mode
  if (m_pCurModeLink && !m_pCurModeLink->instantiate()) {
    return false;
  }
  DEBUG_LOGF("Serialized num modes: %u", m_numModes);
  return true;
}

bool Modes::loadStorage()
{
  // NOTE: We could call loadHeader here but then we wouldn't have the headerBuffer
  //       and in turn wouldn't be able to unserialize the number of modes. The number
  //       of modes is a weird case, it's technically part of the mode list not the
  //       header but it is stored in the same storage slot as the header
  ByteStream headerBuffer;
  // only read storage if the modebuffer isn't filled
  if (!m_engine.storage().read(0, headerBuffer) || !headerBuffer.size()) {
    DEBUG_LOG("Empty buffer read from storage");
    // this kinda sucks whatever they had loaded is gone
    return false;
  }
  // this erases what is stored before we know whether there is data
  // but it's the easiest way to just re-load new data from storage
  clearModes();
  // read the header and load the data
  if (!unserializeSaveHeader(headerBuffer)) {
    return false;
  }
  // unserialize the number of modes next
  uint8_t numModes = 0;
  if (!headerBuffer.unserialize8(&numModes)) {
    return false;
  }
  if (!numModes) {
    DEBUG_LOG("Did not find any modes");
    // this kinda sucks whatever they had loaded is gone
    return false;
  }
  // iterate each mode and read it out of it's storage slot then add it
  for (uint8_t i = 0; i < numModes; ++i) {
    ByteStream modeBuffer(MAX_MODE_SIZE);
    // read each mode from a storage slot and load it
    if (!m_engine.storage().read(i + 1, modeBuffer) || !addSerializedMode(modeBuffer)) {
      return false;
    }
  }
  if (oneClickModeEnabled()) {
    // set the current mode to the startup mode
    switchToStartupMode();
  }
  return true;
}

bool Modes::serializeSaveHeader(ByteStream &saveBuffer)
{
  // serialize the engine version into the modes buffer
  if (!m_engine.serializeVersion(saveBuffer)) {
    return false;
  }
  // NOTE: instead of global brightness the duo uses this to store the
  //       startup mode ID. The duo doesn't offer a global brightness option
  if (!saveBuffer.serialize8(m_globalFlags)) {
    return false;
  }
  // serialize the global brightness
  if (!saveBuffer.serialize8((uint8_t)m_engine.leds().getBrightness())) {
    return false;
  }
  DEBUG_LOGF("Serialized all modes, uncompressed size: %u", saveBuffer.size());
  return true;
}

bool Modes::unserializeSaveHeader(ByteStream &saveHeader)
{
  // reset the unserializer index before unserializing anything
  saveHeader.resetUnserializer();
  uint8_t major = 0;
  uint8_t minor = 0;
  // unserialize the vortex version
  if (!saveHeader.unserialize8(&major)) {
    return false;
  }
  if (!saveHeader.unserialize8(&minor)) {
    return false;
  }
  // check the version for incompatibility
  if (!m_engine.checkVersion(major, minor)) {
    // incompatible version
    ERROR_LOGF("Incompatible savefile version: %u.%u", major, minor);
    return false;
  }
  // NOTE: instead of global brightness the duo uses this to store the
  //       startup mode ID. The duo doesn't offer a global brightness option
  // unserialize the global brightness
  if (!saveHeader.unserialize8(&m_globalFlags)) {
    return false;
  }
  // unserialize the global brightness
  uint8_t brightness = 0;
  if (!saveHeader.unserialize8(&brightness)) {
    return false;
  }
  if (brightness) {
    m_engine.leds().setBrightness(brightness);
  }
  return true;
}

// Save all of the modes to a serial buffer
bool Modes::serialize(ByteStream &modesBuffer)
{
  // serialize the number of modes
  if (!modesBuffer.serialize8(m_numModes)) {
    return false;
  }
  // make sure the current mode is saved in case it has changed somehow
  saveCurMode();
  // uninstantiate cur mode so we have stack space to serialize
  if (m_pCurModeLink) {
    m_pCurModeLink->uninstantiate();
  }
  ModeLink *ptr = m_storedModes;
  while (ptr) {
    // instantiate the mode temporarily
    Mode *mode = ptr->instantiate();
    if (!mode) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
    // serialize it into the target modes buffer
    if (!mode->serialize(modesBuffer)) {
      return false;
    }
    // just uninstansiate the mode after serializing
    ptr->uninstantiate();
    // next mode
    ptr = ptr->next();
  }
  // reinstanstiate the current mode
  if (m_pCurModeLink && !m_pCurModeLink->instantiate()) {
    return false;
  }
  DEBUG_LOGF("Serialized num modes: %u", m_numModes);
  return true;
}

// load all modes from a serial buffer
bool Modes::unserialize(ByteStream &modesBuffer)
{
  DEBUG_LOG("Loading modes...");
  // this is good on memory, but it erases what they have stored before we
  // know whether there is something actually saved in the serial buffer
  clearModes();
  // unserialize the number of modes next
  uint8_t numModes = 0;
  if (!modesBuffer.unserialize8(&numModes)) {
    return false;
  }
  if (!numModes) {
    DEBUG_LOG("Did not find any modes");
    // this kinda sucks whatever they had loaded is gone
    return false;
  }
  // foreach expected mode
  for (uint8_t i = 0; i < numModes; ++i) {
    // just copy the serialized mode into the internal storage because
    // we store the modes in a serialized manner so that they are smaller
    // then we unpack them when we instantiate the mode
    if (!addSerializedMode(modesBuffer)) {
      DEBUG_LOGF("Failed to add mode %u after unserialization", i);
      // clear work so far?
      clearModes();
      return false;
    }
  }
  DEBUG_LOGF("Loaded %u modes from storage (%u bytes)", numModes, modesBuffer.size());
  return (m_numModes == numModes);
}

bool Modes::setDefaults()
{
  clearModes();
  // add each default mode with each of the given colors
  for (uint8_t i = 0; i < num_default_modes; ++i) {
    const default_mode_entry &def = default_modes[i];
    Colorset set(def.numColors, def.cols);
    addMode(def.patternID, nullptr, &set);
  }
  return true;
}

bool Modes::addSerializedMode(ByteStream &serializedMode)
{
#if MAX_MODES != 0
  if (m_numModes >= MAX_MODES) {
    return false;
  }
#endif
  // we must unserialize then re-serialize here because the
  // input argument may contain other patterns in the buffer
  // so we cannot just append the input arg to m_storedModes
  Mode tmpMode(m_engine);
  if (!tmpMode.unserialize(serializedMode)) {
    return false;
  }
  // initialize the mode because a pattern could theoretically serialize
  // differently after it has initialized
  tmpMode.init();
  return addMode(&tmpMode);
}

bool Modes::addModeFromBuffer(ByteStream &serializedMode)
{
#if MAX_MODES != 0
  if (m_numModes >= MAX_MODES) {
    return false;
  }
#endif
  if (!m_storedModes) {
    m_storedModes = new ModeLink(m_engine, serializedMode);
    if (!m_storedModes) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
  } else {
    if (!m_storedModes->append(serializedMode)) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
  }
  // increment mode counter
  m_numModes++;
  return true;
}

// shift the current mode to a different position relative to current position
// negative values for up, positive values for down, 0 for no move
bool Modes::shiftCurMode(int32_t offset)
{
  uint32_t newPos = (uint32_t)((int32_t)m_curMode + offset);
  if (newPos >= m_numModes) {
    return false;
  }
  if (newPos == m_curMode) {
    return true;
  }
  // get the target mode at the position of the shift
  ModeLink *target = getModeLink(newPos);
  if (!target) {
    // invalid new position?
    return false;
  }
  // special case for moving first in list forward
  if (!m_curMode && offset > 0) {
    // update main list ptr
    m_storedModes = m_storedModes->next();
  }
  // unlink the current link
  m_pCurModeLink->unlinkSelf();
  // update the current position to reflect our new pos
  m_curMode = newPos;
  // then re-link the mode at the new spot
  if (offset < 0) {
    // link the link before our target link
    target->linkBefore(m_pCurModeLink);
    // special case for moving into first in list
    if (!m_curMode) {
      m_storedModes = m_pCurModeLink;
    }
  } else {
    // link the link after our target link
    target->linkAfter(m_pCurModeLink);
  }
  return true;
}

bool Modes::addMode(PatternID id, RGBColor c1, RGBColor c2, RGBColor c3,
    RGBColor c4, RGBColor c5, RGBColor c6, RGBColor c7, RGBColor c8)
{
  Colorset set(c1, c2, c3, c4, c5, c6, c7, c8);
  return addMode(id, nullptr, &set);
}

bool Modes::addMode(PatternID id, const PatternArgs *args, const Colorset *set)
{
  // max modes
#if MAX_MODES != 0
  if (m_numModes >= MAX_MODES) {
    return false;
  }
#endif
  if (id >= PATTERN_COUNT) {
    return false;
  }
  Mode tmpMode(m_engine, id, args, set);
  // must init the mode so that it can be serialized
  tmpMode.init();
  // not a very good way to do this but it ensures the mode is
  // added in the same way
  return addMode(&tmpMode);
}

bool Modes::addMode(const Mode *mode)
{
#if MAX_MODES != 0
  // max modes
  if (m_numModes >= MAX_MODES) {
    return false;
  }
#endif
  if (!m_storedModes) {
    m_storedModes = new ModeLink(m_engine, mode);
    if (!m_storedModes) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
  } else {
    if (!m_storedModes->append(mode)) {
      ERROR_OUT_OF_MEMORY();
      return false;
    }
  }
  m_numModes++;
  return true;
}

bool Modes::updateCurMode(const Mode *mode)
{
  if (!mode) {
    return false;
  }
  Mode *pCur = curMode();
  if (!pCur) {
    return false;
  }
  // utilize copy operator
  *pCur = *mode;
  // immediately save this mode to the internal mode storage
  if (!saveCurMode()) {
    return false;
  }
  // initialize the new mode
  return initCurMode();
}

// set the current active mode by index
Mode *Modes::setCurMode(uint8_t index)
{
  if (!m_numModes) {
    return nullptr;
  }
  // clear the LEDs when switching modes
  m_engine.leds().clearAll();
  // if we have a current mode open, close it
  if (m_pCurModeLink) {
    m_pCurModeLink->uninstantiate();
  }
  int8_t newModeIdx = index % m_numModes;
  // lookup the new mode link
  ModeLink *newCurLink = getModeLink(newModeIdx);
  if (!newCurLink) {
    // what
    return nullptr;
  }
  // instantiate the new mode so it is ready, also so it can be checked for PATTERN_NONE
  Mode *newCur = newCurLink->instantiate();
  if (!newCur) {
    ERROR_OUT_OF_MEMORY();
    return nullptr;
  }
  // update to the new mode
  m_curMode = newModeIdx;
  m_pCurModeLink = newCurLink;
  // record the current time as the last switch time
  m_lastSwitchTime = m_engine.time().getCurtime();
  // update the global startup mode to be this mode, if we turn off
  // and turn back on into one click mode it will select this one
  Modes::setStartupMode(newModeIdx);
  // log the change
  DEBUG_LOGF("Switch to Mode: %u / %u (pattern id: %u)",
    m_curMode, m_numModes - 1, newCur->getPatternID());
  // return the new current mode
  return newCur;
}

// the current mode
Mode *Modes::curMode()
{
  // empty mode list
  if (!m_numModes) {
    return nullptr;
  }
  if (!m_pCurModeLink) {
    if (!initCurMode()) {
      ERROR_LOG("Failed to initialize current mode");
      return nullptr;
    }
  }
  // get current mode, instantiate it if necessary
  return m_pCurModeLink->instantiate();
}

// iterate to next mode and return it
Mode *Modes::nextMode()
{
  if (!m_numModes) {
    return nullptr;
  }
  // iterate the cur mode forward
  return setCurMode(m_curMode + 1);
}

// iterate to previous mode and return it
Mode *Modes::previousMode()
{
  if (!m_numModes) {
    return nullptr;
  }
  // iterate the cur mode backwards
  if (!m_curMode) {
    return setCurMode(numModes() - 1);
  }
  return setCurMode(m_curMode - 1);
}

Mode *Modes::nextModeSkipEmpty()
{
  do {
    if (setCurMode(m_curMode + 1) && !curMode()->isEmpty()) {
      break;
    }
  } while (m_curMode != 0);
  return curMode();
}

void Modes::deleteCurMode()
{
  if (!m_numModes || !m_pCurModeLink) {
    return;
  }
  // unlink the current mode so it can be deleted and
  // update the current mode link accordingly
  ModeLink *newCur = m_pCurModeLink->unlinkSelf();
  delete m_pCurModeLink;
  m_pCurModeLink = newCur;
  if (m_curMode) {
    m_curMode--;
  } else {
    m_storedModes = m_pCurModeLink;
  }
  m_numModes--;
  if (!m_numModes) {
    m_storedModes = nullptr;
  }
}

void Modes::clearModes()
{
  if (!m_numModes || !m_storedModes) {
    return;
  }
  // delete the first node and it will delete the entire chain
  delete m_storedModes;
  m_pCurModeLink = nullptr;
  m_storedModes = nullptr;
  m_numModes = 0;
  // might as well clear the leds
  m_engine.leds().clearAll();
}

void Modes::setStartupMode(uint8_t index)
{
  // zero out the upper nibble to disable
  m_globalFlags &= 0x0F;
  // or in the index value shifted into the upper nibble
  m_globalFlags |= (index << 4) & 0xF0;
}

uint8_t Modes::startupMode()
{
  // zero out the upper nibble to disable
  return (m_globalFlags & 0xF0) >> 4;
}

Mode *Modes::switchToStartupMode()
{
  return setCurMode(startupMode());
}

bool Modes::setFlag(uint8_t flag, bool enable, bool save)
{
  // then actually if it's enabled ensure the upper nibble is set
  if (enable) {
    // set the cur mode index as the upper nibble
    m_globalFlags |= flag;
  } else {
    m_globalFlags &= ~flag;
  }
  DEBUG_LOGF("Toggled instant on/off to %s", enable ? "on" : "off");
  if (!save) {
    // if save is not requested then just return here
    return true;
  }
  // otherwise need to update the global flags field of the save header in storage
  ByteStream headerBuffer;
  // read out the storage header so we can update the flag field
  if (!m_engine.storage().read(0, headerBuffer) || !headerBuffer.size()) {
    // if cannot read the save header then just save it normally
    return saveHeader();
  }
  // layout of the save header, this struct is never really used anywhere else
  // except here the actual layout of the save header is dictated by
  // saveHeader() and serializeSaveHeader()
  struct SaveHeader {
    uint8_t vMajor;
    uint8_t vMinor;
    uint8_t globalFlags;
    uint8_t brightness;
    uint8_t numModes;
  };
  // data cannot be NULL since size is non zero
  SaveHeader *pHeader = (SaveHeader *)headerBuffer.data();
  // update the global flags field in the header
  pHeader->globalFlags = m_globalFlags;
  // need to force the crc to recalc since we modified the data, just mark the
  // CRC as dirty and Storage::write() will re-calculate the CRC if it's dirty
  headerBuffer.setCRCDirty();
  // write the save header back to storage
  return m_engine.storage().write(0, headerBuffer);
}

#ifdef VORTEX_LIB
#include "Patterns/PatternBuilder.h"
// get the maximum size a mode can occupy
uint32_t Modes::maxModeSize()
{
  Mode maxMode(m_engine);
  uint8_t x = 0;
  for (LedPos p = LED_FIRST; p < LED_COUNT; ++p) {
    // blend takes up 8 params
    PatternArgs maxArgs((uint8_t)p + 0xd2, (uint8_t)p + 0xd3, (uint8_t)p + 0xd4, (uint8_t)p + 0xd5,
      (uint8_t)p + 0xd6, (uint8_t)p + 0xd7, (uint8_t)p + 0xd8, (uint8_t)p + 0xd9);
    //PatternArgs typicalArgs = PatternBuilder::getDefaultArgs(PATTERN_BLEND);
    Colorset maxSet;
    for (uint8_t i = 0; i < MAX_COLOR_SLOTS; ++i) {
      // different color in each slot
      RGBColor col;
      col.red = ++x;
      col.green = ++x;
      col.blue = ++x;
      maxSet.addColor(col);
    }
    maxMode.setPattern(PATTERN_BLEND, p, &maxArgs, &maxSet);
  }
  ByteStream stream;
  maxMode.saveToBuffer(stream);
  return stream.size();
}

// get the maximum size a savefile can occupy
uint32_t Modes::maxSaveSize()
{
#if MAX_MODES == 0
  // unbounded
  return 0;
#else
  ByteStream backupModes;
  saveToBuffer(backupModes);
  for (uint32_t i = 0; i < MAX_MODES; ++i) {
    Mode maxMode(m_engine);
    for (LedPos p = LED_FIRST; p < LED_COUNT; ++p) {
      // blend takes up 8 params
      PatternArgs maxArgs((uint8_t)p + 2, (uint8_t)p + 3, (uint8_t)p + 4, (uint8_t)p + 5,
        (uint8_t)p + 6, (uint8_t)p + 7, (uint8_t)p + 8, (uint8_t)p + 9);
      Colorset maxSet;
      for (uint32_t i = 0; i < MAX_COLOR_SLOTS; ++i) {
        // different color in each slot
        maxSet.addColor(RGBColor((uint8_t)p + (i * 3), (uint8_t)p + (i * 3) + 1, (uint8_t)p + (i * 3) + 2));
      }
      maxMode.setPattern(PATTERN_BLEND, p, &maxArgs, &maxSet);
    }
    addMode(&maxMode);
  }
  // grab the size of the new buffer
  ByteStream stream;
  saveToBuffer(stream);
  uint32_t size = stream.size();
  // restore backed up modes
  loadFromBuffer(backupModes);
  return size;
#endif
}

uint8_t Modes::globalFlags()
{
  return m_globalFlags;
}
#endif

// fetch a link from the chain by index
Modes::ModeLink *Modes::getModeLink(uint32_t index)
{
  if (index >= m_numModes) {
    return nullptr;
  }
  ModeLink *ptr = m_storedModes;
  while (index > 0 && ptr) {
    ptr = ptr->next();
    index--;
  }
  return ptr;
}

Mode *Modes::initCurMode(bool force)
{
  if (!m_numModes) {
    return nullptr;
  }
  // cleanup the current mode link
  if (m_pCurModeLink) {
    m_pCurModeLink->uninstantiate();
  }
  // update the current mode link based on the curmode index
  m_pCurModeLink = getModeLink(m_curMode);
  if (!m_pCurModeLink) {
    return nullptr;
  }
  if (force) {
    m_pCurModeLink->uninstantiate();
  }
  return m_pCurModeLink->instantiate();
}

bool Modes::saveCurMode()
{
  if (!m_pCurModeLink) {
    // if there's no loaded mode currently then there's nothing
    // to save so there's no error
    return false;
  }
  // force the current mode to save back to serial to catch changes
  return m_pCurModeLink->save();
}

Modes::ModeLink::ModeLink(VortexEngine &engine, const Mode *src, bool inst) :
  m_engine(engine),
  m_pInstantiatedMode(nullptr),
  m_storedMode(),
  m_next(nullptr),
  m_prev(nullptr)
{
  if (src) {
    init(src);
  }
  if (src && inst) {
    instantiate();
  }
}

Modes::ModeLink::ModeLink(VortexEngine &engine, const ByteStream &src, bool inst) :
  m_engine(engine),
  m_pInstantiatedMode(nullptr),
  m_storedMode(src),
  m_next(nullptr),
  m_prev(nullptr)
{
  if (src.size() && inst) {
    instantiate();
  }
}

Modes::ModeLink::~ModeLink()
{
  if (m_next) {
    delete m_next;
  }
  if (m_pInstantiatedMode) {
    delete m_pInstantiatedMode;
  }
}

bool Modes::ModeLink::init(const Mode *mode)
{
  if (!mode) {
    return false;
  }
  m_storedMode.clear();
  // serialize the mode so it can be instantiated anytime
  if (!mode->saveToBuffer(m_storedMode)) {
    return false;
  }
  return true;
}

bool Modes::ModeLink::append(const Mode *next)
{
  if (!next) {
    return false;
  }
  // if not end of chain, recurse on next link
  if (m_next) {
    return m_next->append(next);
  }
  m_next = new ModeLink(m_engine, next);
  if (!m_next) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  m_next->m_prev = this;
  return true;
}

bool Modes::ModeLink::append(const ByteStream &next)
{
  if (!next.size()) {
    return false;
  }
  // if not end of chain, recurse on next link
  if (m_next) {
    return m_next->append(next);
  }
  m_next = new ModeLink(m_engine, next);
  if (!m_next) {
    ERROR_OUT_OF_MEMORY();
    return false;
  }
  m_next->m_prev = this;
  return true;
}

void Modes::ModeLink::play()
{
  if (!m_pInstantiatedMode) {
    return;
  }
  m_pInstantiatedMode->play();
}

Modes::ModeLink *Modes::ModeLink::unlinkSelf()
{
  // unlink this node from the chain
  if (m_prev) {
    m_prev->m_next = m_next;
  }
  if (m_next) {
    m_next->m_prev = m_prev;
  }
  // grab the new link that will take this place
  ModeLink *newLink = m_prev ? m_prev : m_next;
  // clear the links of this node
  m_prev = nullptr;
  m_next = nullptr;
  return newLink;
}

void Modes::ModeLink::linkAfter(ModeLink *link)
{
  if (!link) {
    return;
  }
  if (m_next) {
    m_next->m_prev = link;
    link->m_next = m_next;
  }
  m_next = link;
  link->m_prev = this;
}

void Modes::ModeLink::linkBefore(ModeLink *link)
{
  if (!link) {
    return;
  }
  if (m_prev) {
    m_prev->m_next = link;
    link->m_prev = m_prev;
  }
  m_prev = link;
  link->m_next = this;
}

Mode *Modes::ModeLink::instantiate()
{
  if (m_pInstantiatedMode) {
    return m_pInstantiatedMode;
  }
  Mode *newMode = new Mode(m_engine);
  if (!newMode) {
    ERROR_OUT_OF_MEMORY();
    return nullptr;
  }
  m_storedMode.resetUnserializer();
  if (!newMode->loadFromBuffer(m_storedMode)) {
    return nullptr;
  }
  // TODO: init new mode?
  m_pInstantiatedMode = newMode;
  return m_pInstantiatedMode;
}

void Modes::ModeLink::uninstantiate()
{
  if (m_pInstantiatedMode) {
    delete m_pInstantiatedMode;
    m_pInstantiatedMode = nullptr;
  }
}

bool Modes::ModeLink::save()
{
  if (!m_pInstantiatedMode) {
    return false;
  }
  m_storedMode.clear();
  return m_pInstantiatedMode->saveToBuffer(m_storedMode);
}

#if MODES_TEST == 1
#include <assert.h>
#include <stdio.h>

#include "../Patterns/PatternBuilder.h"

void Modes::test()
{
  INFO_LOG("== Beginning Modes Test ==\n");

  RGBColor col = RGB_RED;
  assert(!addMode(PATTERN_COUNT, col));
  for (PatternID pat = PATTERN_FIRST; pat < PATTERN_COUNT; ++pat) {
    assert(addMode(pat, col));
  }
  assert(numModes() == PATTERN_COUNT);
  clearModes();
  assert(numModes() == 0);

  Colorset set(RGB_RED, RGB_GREEN, RGB_BLUE);
  assert(!addMode(PATTERN_COUNT, nullptr, &set));
  for (PatternID pat = PATTERN_FIRST; pat < PATTERN_COUNT; ++pat) {
    assert(addMode(pat, nullptr, &set));
  }
  assert(numModes() == PATTERN_COUNT);
  clearModes();
  assert(numModes() == 0);

  // add a new mode in various different ways
  assert(!addMode(PATTERN_COUNT, col));
  Colorset set2(RGB_RED, RGB_GREEN, RGB_BLUE);
  for (PatternID pat = PATTERN_FIRST; pat < PATTERN_COUNT; ++pat) {
    Mode tmpMode(pat, nullptr, &set2);
    assert(addMode(&tmpMode));
  }
  assert(numModes() == PATTERN_COUNT);
  clearModes();
  assert(numModes() == 0);

  INFO_LOG("addMode(): success\n");

  ByteStream modebuf;
  ByteStream modesave;
  PatternArgs args = PatternBuilder::getDefaultArgs(PATTERN_BASIC);
  Mode tmpMode(PATTERN_BASIC, &args, &set);
  tmpMode.serialize(modebuf);
  tmpMode.saveToBuffer(modesave);
  assert(addSerializedMode(modebuf));
  assert(numModes() == 1);
  assert(addModeFromBuffer(modesave));
  assert(numModes() == 2);
  assert(getModeLink(0) != nullptr);
  Mode *mode1 = getModeLink(0)->instantiate();
  assert(mode1 != nullptr);
  Mode *mode2 = getModeLink(1)->instantiate();
  assert(mode2 != nullptr);
  assert(mode1->equals(mode2));

  INFO_LOG("addSerializedMode(): success\n");

  Colorset newset(RGB_BLUE, RGB_RED, RGB_GREEN);
  assert(updateCurMode(PATTERN_HYPERSTROBE, nullptr));
  assert(getModeLink(0)->mode()->getPatternID() == PATTERN_HYPERSTROBE);
  assert(setCurMode(1));
  // update the current mode to match the given mode
  assert(updateCurMode(PATTERN_DOPS, &newset));
  assert(getModeLink(1)->mode()->getPatternID() == PATTERN_DOPS);
  Mode newTmp(PATTERN_BLEND, PatternBuilder::getDefaultArgs(PATTERN_BLEND),
    Colorset(RGB_YELLOW, RGB_ORANGE, RGB_CYAN, RGB_BLUE, RGB_WHITE, RGB_RED));

  INFO_LOG("updateCurMode(): success\n");

  assert(shiftCurMode(-1));
  assert(m_curMode == 0);
  assert(getModeLink(0)->instantiate()->getPatternID() == PATTERN_DOPS);
  assert(getModeLink(1)->instantiate()->getPatternID() == PATTERN_HYPERSTROBE);
  assert(shiftCurMode(0));
  assert(m_curMode == 0);
  assert(getModeLink(0)->instantiate()->getPatternID() == PATTERN_DOPS);
  assert(getModeLink(1)->instantiate()->getPatternID() == PATTERN_HYPERSTROBE);
  assert(shiftCurMode(1));
  assert(m_curMode == 1);
  assert(getModeLink(0)->instantiate()->getPatternID() == PATTERN_HYPERSTROBE);
  assert(getModeLink(1)->instantiate()->getPatternID() == PATTERN_DOPS);

  INFO_LOG("shiftCurMode(): success\n");

  deleteCurMode();
  assert(m_numModes == 1);
  assert(m_curMode == 0);
  deleteCurMode();
  assert(m_numModes == 0);
  assert(m_curMode == 0);

  INFO_LOG("deleteCurMode(): success\n");

  INFO_LOG("== Success Running Modes Test ==\n");
}
#endif

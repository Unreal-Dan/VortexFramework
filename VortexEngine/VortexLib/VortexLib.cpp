#include "VortexLib.h"

// VortexEngine includes
#include "VortexEngine.h"
#include "Buttons/Button.h"
#include "Buttons/Buttons.h"
#include "Serial/ByteStream.h"
#include "Wireless/IRReceiver.h"
#include "Wireless/VLReceiver.h"
#include "Patterns/PatternBuilder.h"
#include "Time/TimeControl.h"
#include "Patterns/Pattern.h"
#include "Colors/Colorset.h"
#include "Storage/Storage.h"
#include "Random/Random.h"
#include "Time/Timings.h"
#include "Menus/Menus.h"
#include "Modes/Modes.h"
#include "Menus/Menu.h"
#include "Modes/Mode.h"
#include "Random/Random.h"

#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#else
#include <Windows.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>

#ifdef WASM
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

// This wraps Vortex::tick but returns an array of led colors for the tick
val RunTick(Vortex &vortex) {
  // first run a tick
  vortex.tick();
  // then extract the color that was produced by the tick
  RGBColor *leds = vortex.engine().leds().ledData();
  val ledArray = val::array();
  for (uint32_t i = 0; i < vortex.engine().leds().ledCount(); ++i) {
    val color = val::object();
    color.set("red", leds[i].red);
    color.set("green", leds[i].green);
    color.set("blue", leds[i].blue);
    ledArray.set(i, color);
  }
  return ledArray;
}

emscripten::val getDataArray(const ByteStream &byteStream)
{
  const uint8_t *dataPtr = byteStream.data();
  uint32_t size = byteStream.size();

  emscripten::val dataArray = emscripten::val::array();

  for (uint32_t i = 0; i < size; ++i) {
    dataArray.call<void>("push", dataPtr[i]);
  }

  return dataArray;
}

emscripten::val getRawDataArray(const ByteStream &byteStream)
{
  const uint8_t *rawDataPtr = reinterpret_cast<const uint8_t *>(byteStream.rawData());
  uint32_t rawSize = byteStream.rawSize();

  emscripten::val rawDataArray = emscripten::val::array();

  for (uint32_t i = 0; i < rawSize; ++i) {
    rawDataArray.call<void>("push", rawDataPtr[i]);
  }

  return rawDataArray;
}

bool createByteStreamFromData(val bytesArray, ByteStream &stream)
{
  // Convert val to std::vector<uint8_t>
  std::vector<uint8_t> data = vecFromJSArray<uint8_t>(bytesArray);
  return stream.init(data.size(), data.data());
}

bool createByteStreamFromRawData(val bytesArray, ByteStream &stream)
{
  // Convert val to std::vector<uint8_t>
  std::vector<uint8_t> data = vecFromJSArray<uint8_t>(bytesArray);
  return stream.rawInit(data.data(), data.size());
}

// js is dumb and has issues doing this conversion I guess
PatternID intToPatternID(int val)
{
  return (PatternID)val;
}

Mode createMode(Vortex &vortex, PatternID pat, PatternArgs args, Colorset set)
{
  Mode newMode(vortex.engine(), vortex.engine().leds().ledCount());
  newMode.setPattern(pat, vortex.engine().leds().ledCount(), &args, &set);
  return newMode;
}

EMSCRIPTEN_BINDINGS(Vortex) {
  // vector<string>
  register_vector<std::string>("VectorString");

  // basic control functions
  function("RunTick", &RunTick);

  // Bind the HSVColor class
  class_<HSVColor>("HSVColor")
    .constructor<>()
    .constructor<uint8_t, uint8_t, uint8_t>()
    .constructor<uint32_t>()
    .function("empty", &HSVColor::empty)
    .function("clear", &HSVColor::clear)
    .function("raw", &HSVColor::raw)
    .property("hue", &HSVColor::hue)
    .property("sat", &HSVColor::sat)
    .property("val", &HSVColor::val);

  // Bind the RGBColor class
  class_<RGBColor>("RGBColor")
    .constructor<>()
    .constructor<uint8_t, uint8_t, uint8_t>()
    .constructor<uint32_t>()
    .function("empty", &RGBColor::empty)
    .function("clear", &RGBColor::clear)
    .function("adjustBrightness", &RGBColor::adjustBrightness)
    .function("serialize", &RGBColor::serialize)
    .function("unserialize", &RGBColor::unserialize)
    .function("raw", &RGBColor::raw)
    .property("red", &RGBColor::red)
    .property("green", &RGBColor::green)
    .property("blue", &RGBColor::blue);

  // Bind the utility conversion functions
  function("hsv_to_rgb_rainbow", &hsv_to_rgb_rainbow);
  function("hsv_to_rgb_raw_C", &hsv_to_rgb_raw_C);
  function("hsv_to_rgb_generic", &hsv_to_rgb_generic);
  function("rgb_to_hsv_approx", &rgb_to_hsv_approx);
  function("rgb_to_hsv_generic", &rgb_to_hsv_generic);

  class_<ByteStream>("ByteStream")
    .constructor<>()
    .constructor<uint32_t, const uint8_t *>()

    // member functions
    .function("rawInit", &ByteStream::rawInit, allow_raw_pointer<const uint8_t *>())
    .function("init", &ByteStream::init, allow_raw_pointer<const uint8_t *>())
    .function("clear", &ByteStream::clear)
    .function("shrink", &ByteStream::shrink)
    .function("append", &ByteStream::append)
    .function("extend", &ByteStream::extend)
    .function("trim", &ByteStream::trim)
    .function("compress", &ByteStream::compress)
    .function("decompress", &ByteStream::decompress)
    .function("recalcCRC", &ByteStream::recalcCRC)
    .function("sanity", &ByteStream::sanity)
    .function("checkCRC", &ByteStream::checkCRC)
    .function("isCRCDirty", &ByteStream::isCRCDirty)
    .function("serialize8", &ByteStream::serialize8)
    .function("serialize16", &ByteStream::serialize16)
    .function("serialize32", &ByteStream::serialize32)
    .function("resetUnserializer", &ByteStream::resetUnserializer)
    .function("moveUnserializer", &ByteStream::moveUnserializer)
    .function("unserializerAtEnd", &ByteStream::unserializerAtEnd)
    // TODO: provide better apis here
    //.function("unserialize8", &ByteStream::unserialize8)
    //.function("unserialize16", &ByteStream::unserialize16)
    //.function("unserialize32", &ByteStream::unserialize32)
    .function("peek8", &ByteStream::peek8)
    .function("peek16", &ByteStream::peek16)
    .function("peek32", &ByteStream::peek32)
    .function("data", &ByteStream::data, allow_raw_pointer<uint8_t>())
    .function("rawData", &ByteStream::rawData, allow_raw_pointer<void>())
    .function("rawSize", &ByteStream::rawSize)
    .function("size", &ByteStream::size)
    .function("capacity", &ByteStream::capacity)
    .function("is_compressed", &ByteStream::is_compressed)
    .function("CRC", &ByteStream::CRC);

  // Binding dynamic values from Leds class
  class_<Leds>("Leds")
    .function("ledCount", &Leds::ledCount)
    .function("ledLast", &Leds::ledLast)
    .function("ledMulti", &Leds::ledMulti)
    .function("ledAllSingle", &Leds::ledAllSingle)
    .function("ledAny", &Leds::ledAny)
    .function("ledData", &Leds::ledData, allow_raw_pointer<const RGBColor *>());

  function("intToPatternID", &intToPatternID);
  function("isMultiLedPatternID", &isMultiLedPatternID);
  function("isSingleLedPatternID", &isSingleLedPatternID);
  function("createMode", &createMode);

  enum_<PatternID>("PatternID")
    // Meta Constants
    .value("PATTERN_NONE", PatternID::PATTERN_NONE)

    // single led patterns
    .value("PATTERN_STROBE", PatternID::PATTERN_STROBE)
    .value("PATTERN_HYPERSTROBE", PatternID::PATTERN_HYPERSTROBE)
    .value("PATTERN_PICOSTROBE", PatternID::PATTERN_PICOSTROBE)
    .value("PATTERN_STROBIE", PatternID::PATTERN_STROBIE)
    .value("PATTERN_DOPS", PatternID::PATTERN_DOPS)
    .value("PATTERN_ULTRADOPS", PatternID::PATTERN_ULTRADOPS)
    .value("PATTERN_STROBEGAP", PatternID::PATTERN_STROBEGAP)
    .value("PATTERN_HYPERGAP", PatternID::PATTERN_HYPERGAP)
    .value("PATTERN_PICOGAP", PatternID::PATTERN_PICOGAP)
    .value("PATTERN_STROBIEGAP", PatternID::PATTERN_STROBIEGAP)
    .value("PATTERN_DOPSGAP", PatternID::PATTERN_DOPSGAP)
    .value("PATTERN_ULTRAGAP", PatternID::PATTERN_ULTRAGAP)
    .value("PATTERN_BLINKIE", PatternID::PATTERN_BLINKIE)
    .value("PATTERN_GHOSTCRUSH", PatternID::PATTERN_GHOSTCRUSH)
    .value("PATTERN_DOUBLEDOPS", PatternID::PATTERN_DOUBLEDOPS)
    .value("PATTERN_CHOPPER", PatternID::PATTERN_CHOPPER)
    .value("PATTERN_DASHGAP", PatternID::PATTERN_DASHGAP)
    .value("PATTERN_DASHDOPS", PatternID::PATTERN_DASHDOPS)
    .value("PATTERN_DASHCRUSH", PatternID::PATTERN_DASHCRUSH)
    .value("PATTERN_ULTRADASH", PatternID::PATTERN_ULTRADASH)
    .value("PATTERN_GAPCYCLE", PatternID::PATTERN_GAPCYCLE)
    .value("PATTERN_DASHCYCLE", PatternID::PATTERN_DASHCYCLE)
    .value("PATTERN_TRACER", PatternID::PATTERN_TRACER)
    .value("PATTERN_RIBBON", PatternID::PATTERN_RIBBON)
    .value("PATTERN_MINIRIBBON", PatternID::PATTERN_MINIRIBBON)
    .value("PATTERN_BLEND", PatternID::PATTERN_BLEND)
    .value("PATTERN_BLENDSTROBE", PatternID::PATTERN_BLENDSTROBE)
    .value("PATTERN_BLENDSTROBEGAP", PatternID::PATTERN_BLENDSTROBEGAP)
    .value("PATTERN_COMPLEMENTARY_BLEND", PatternID::PATTERN_COMPLEMENTARY_BLEND)
    .value("PATTERN_COMPLEMENTARY_BLENDSTROBE", PatternID::PATTERN_COMPLEMENTARY_BLENDSTROBE)
    .value("PATTERN_COMPLEMENTARY_BLENDSTROBEGAP", PatternID::PATTERN_COMPLEMENTARY_BLENDSTROBEGAP)
    .value("PATTERN_SOLID", PatternID::PATTERN_SOLID)
    // multi led patterns
    .value("PATTERN_HUE_SCROLL", PatternID::PATTERN_HUE_SCROLL)
    .value("PATTERN_THEATER_CHASE", PatternID::PATTERN_THEATER_CHASE)
    .value("PATTERN_CHASER", PatternID::PATTERN_CHASER)
    .value("PATTERN_ZIGZAG", PatternID::PATTERN_ZIGZAG)
    .value("PATTERN_ZIPFADE", PatternID::PATTERN_ZIPFADE)
    .value("PATTERN_DRIP", PatternID::PATTERN_DRIP)
    .value("PATTERN_DRIPMORPH", PatternID::PATTERN_DRIPMORPH)
    .value("PATTERN_CROSSDOPS", PatternID::PATTERN_CROSSDOPS)
    .value("PATTERN_DOUBLESTROBE", PatternID::PATTERN_DOUBLESTROBE)
    .value("PATTERN_METEOR", PatternID::PATTERN_METEOR)
    .value("PATTERN_SPARKLETRACE", PatternID::PATTERN_SPARKLETRACE)
    .value("PATTERN_VORTEXWIPE", PatternID::PATTERN_VORTEXWIPE)
    .value("PATTERN_WARP", PatternID::PATTERN_WARP)
    .value("PATTERN_WARPWORM", PatternID::PATTERN_WARPWORM)
    .value("PATTERN_SNOWBALL", PatternID::PATTERN_SNOWBALL)
    .value("PATTERN_LIGHTHOUSE", PatternID::PATTERN_LIGHTHOUSE)
    .value("PATTERN_PULSISH", PatternID::PATTERN_PULSISH)
    .value("PATTERN_FILL", PatternID::PATTERN_FILL)
    .value("PATTERN_BOUNCE", PatternID::PATTERN_BOUNCE)
    .value("PATTERN_SPLITSTROBIE", PatternID::PATTERN_SPLITSTROBIE)
    .value("PATTERN_BACKSTROBE", PatternID::PATTERN_BACKSTROBE)
    .value("PATTERN_VORTEX", PatternID::PATTERN_VORTEX)
    .value("PATTERN_COUNT", PatternID::PATTERN_COUNT);
    // meta constants
    //.value("PATTERN_FIRST", PatternID::PATTERN_FIRST)
    //.value("PATTERN_SINGLE_FIRST", PatternID::PATTERN_SINGLE_FIRST)
    //.value("PATTERN_MULTI_FIRST", PatternID::PATTERN_MULTI_FIRST)
    //.value("PATTERN_SINGLE_LAST", PatternID::PATTERN_SINGLE_LAST)
    //.value("PATTERN_SINGLE_COUNT", PatternID::PATTERN_SINGLE_COUNT)
    //.value("PATTERN_MULTI_LAST", PatternID::PATTERN_MULTI_LAST)
    //.value("PATTERN_MULTI_COUNT", PatternID::PATTERN_MULTI_COUNT)
    //.value("PATTERN_LAST", PatternID::PATTERN_LAST)

  enum_<MenuEntryID>("MenuEntryID")
    .value("MENU_NONE", MenuEntryID::MENU_NONE)
    .value("MENU_FIRST", MenuEntryID::MENU_FIRST)
    .value("MENU_RANDOMIZER", MenuEntryID::MENU_RANDOMIZER)
    .value("MENU_MODE_SHARING", MenuEntryID::MENU_MODE_SHARING)
#if ENABLE_EDITOR_CONNECTION == 1
    .value("MENU_EDITOR_CONNECTION", MenuEntryID::MENU_EDITOR_CONNECTION)
#endif
    .value("MENU_COLOR_SELECT", MenuEntryID::MENU_COLOR_SELECT)
    .value("MENU_PATTERN_SELECT", MenuEntryID::MENU_PATTERN_SELECT)
    .value("MENU_GLOBAL_BRIGHTNESS", MenuEntryID::MENU_GLOBAL_BRIGHTNESS)
    .value("MENU_FACTORY_RESET", MenuEntryID::MENU_FACTORY_RESET);

  class_<Colorset>("Colorset")
    .constructor<>()
    .constructor<RGBColor, RGBColor, RGBColor, RGBColor, RGBColor, RGBColor, RGBColor, RGBColor>()
    .constructor<uint8_t, const uint32_t *>()
    .function("init", &Colorset::init)
    .function("clear", &Colorset::clear)
    .function("equals", select_overload<bool(const Colorset & set) const>(&Colorset::equals))
    .function("get", &Colorset::get)
    .function("set", &Colorset::set)
    .function("skip", &Colorset::skip)
    .function("cur", &Colorset::cur)
    .function("setCurIndex", &Colorset::setCurIndex)
    .function("resetIndex", &Colorset::resetIndex)
    .function("curIndex", &Colorset::curIndex)
    .function("getPrev", &Colorset::getPrev)
    .function("getNext", &Colorset::getNext)
    .function("peek", &Colorset::peek)
    .function("peekNext", &Colorset::peekNext)
    .function("numColors", &Colorset::numColors)
    .function("shift", &Colorset::shift)
    .function("onStart", &Colorset::onStart)
    .function("onEnd", &Colorset::onEnd)
    .function("serialize", &Colorset::serialize)
    .function("unserialize", &Colorset::unserialize)
    .function("addColor", select_overload<bool(RGBColor)>(&Colorset::addColor))
    .function("addColorHSV", &Colorset::addColorHSV)
    .function("randomize", &Colorset::randomize)
    .function("adjustBrightness", &Colorset::adjustBrightness)
    .function("removeColor", &Colorset::removeColor)
    .function("operator[]", &Colorset::operator[])
    // If more specific or additional randomize functions are needed, they can be added here as well
    .function("randomizeSolid", &Colorset::randomizeSolid)
    .function("randomizeComplimentary", &Colorset::randomizeComplimentary)
    .function("randomizeTriadic", &Colorset::randomizeTriadic)
    .function("randomizeSquare", &Colorset::randomizeSquare)
    .function("randomizePentadic", &Colorset::randomizePentadic)
    .function("randomizeRainbow", &Colorset::randomizeRainbow);

  class_<PatternArgs>("PatternArgs")
    .constructor<>()
    .constructor<uint8_t>()
    .constructor<uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>()
    .constructor<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>()
    .function("init", select_overload<void()>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("init", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::init))
    .function("addArgs", select_overload<void(uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .function("addArgs", select_overload<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>(&PatternArgs::addArgs))
    .property("arg1", &PatternArgs::arg1)
    .property("arg2", &PatternArgs::arg2)
    .property("arg3", &PatternArgs::arg3)
    .property("arg4", &PatternArgs::arg4)
    .property("arg5", &PatternArgs::arg5)
    .property("arg6", &PatternArgs::arg6)
    .property("arg7", &PatternArgs::arg7)
    .property("arg8", &PatternArgs::arg8);

  class_<Pattern>("Pattern")
    .function("bind", &Pattern::bind)
    .function("init", &Pattern::init)
    .function("serialize", &Pattern::serialize)
    .function("unserialize", &Pattern::unserialize)
    .function("setArg", &Pattern::setArg)
    .function("getArg", &Pattern::getArg)
    .function("setArgs", &Pattern::setArgs)
    .function("getArgs", &Pattern::getArgs)
    .function("getNumArgs", &Pattern::getNumArgs)
    .function("equals", &Pattern::equals, allow_raw_pointer<const Pattern *>())
    .function("getColorset", select_overload<const Colorset() const>(&Pattern::getColorset))
    .function("setColorset", &Pattern::setColorset)
    .function("clearColorset", &Pattern::clearColorset)
    .function("setLedPos", &Pattern::setLedPos)
    .function("getPatternID", &Pattern::getPatternID)
    .function("getLedPos", &Pattern::getLedPos)
    .function("getFlags", &Pattern::getFlags)
    .function("hasFlags", &Pattern::hasFlags);

  class_<PatternBuilder>("PatternBuilder")
    .function("make", &PatternBuilder::make, allow_raw_pointers())
    .function("dupe", &PatternBuilder::dupe, allow_raw_pointers())
    .function("makeSingle", &PatternBuilder::makeSingle, allow_raw_pointers())
    .function("makeMulti", &PatternBuilder::makeMulti, allow_raw_pointers())
    //.function("unserialize", &PatternBuilder::unserialize)
    .function("getDefaultArgs", &PatternBuilder::getDefaultArgs)
    .function("numDefaultArgs", &PatternBuilder::numDefaultArgs)
    .function("isDefaultArgs", &PatternBuilder::isDefaultArgs);

  class_<Mode>("Mode")
    //.constructor<>()
    // overloading only works with param count not typing
    //.constructor<PatternID, const Colorset &>()
    //.constructor<PatternID, const PatternArgs &, const Colorset &>()
    //.constructor<PatternID, const PatternArgs *, const Colorset *>()
    //.constructor<const Mode *>()
    .function("copyFrom", select_overload<void(const Mode &)>(&Mode::operator=))
    .function("equals", &Mode::operator==)
    .function("notEquals", &Mode::operator!=)
    .function("init", &Mode::init)
    .function("play", &Mode::play)
    .function("saveToBuffer", &Mode::saveToBuffer)
    .function("loadFromBuffer", &Mode::loadFromBuffer)
    .function("serialize", &Mode::serialize)
    .function("unserialize", &Mode::unserialize)
    .function("equalsMode", &Mode::equals, allow_raw_pointer<const Mode *>())
    .function("setLedCount", &Mode::setLedCount)
    .function("getLedCount", &Mode::getLedCount)
    .function("getColorset", select_overload<const Colorset(LedPos) const>(&Mode::getColorset))
    .function("getPatternID", &Mode::getPatternID)
    .function("getPattern", select_overload<Pattern * (LedPos)>(&Mode::getPattern), emscripten::allow_raw_pointers())
    .function("getConstPattern", select_overload<const Pattern * (LedPos) const>(&Mode::getPattern), emscripten::allow_raw_pointers())
    .function("setPattern", static_cast<bool(Mode:: *)(PatternID, LedPos, const PatternArgs *, const Colorset *)>(&Mode::setPattern), emscripten::allow_raw_pointers())
    .function("setPattern2", &Mode::setPattern, allow_raw_pointers())
    .function("setColorset", select_overload<bool(const Colorset &, LedPos)>(&Mode::setColorset))
    .function("clearPattern", &Mode::clearPattern)
    .function("clearColorset", &Mode::clearColorset)
    .function("setArg", &Mode::setArg)
    .function("getArg", &Mode::getArg)
    .function("hasMultiLed", &Mode::hasMultiLed)
    .function("hasSingleLed", &Mode::hasSingleLed)
    .function("hasSameSingleLed", &Mode::hasSameSingleLed)
    .function("hasSparseSingleLed", &Mode::hasSparseSingleLed)
    .function("isEmpty", &Mode::isEmpty)
    //.function("getSingleLedMap", &Mode::getSingleLedMap)
    .function("isMultiLed", &Mode::isMultiLed);

  class_<Menus>("Menus")
    .function("init", &Menus::init)
    .function("cleanup", &Menus::cleanup)
    .function("run", &Menus::run)
    .function("openMenuSelection", &Menus::openMenuSelection)
    .function("openMenu", &Menus::openMenu)
    .function("showSelection", &Menus::showSelection)
    .function("checkOpen", &Menus::checkOpen)
    .function("checkInMenu", &Menus::checkInMenu)
    .function("curMenu", &Menus::curMenu, allow_raw_pointers())
    .function("curMenuID", &Menus::curMenuID);

  class_<Menu>("Menu")
    .function("init", &Menu::init)
    .function("run", &Menu::run)
    .function("onLedSelected", &Menu::onLedSelected)
    .function("onShortClick", &Menu::onShortClick)
    .function("onLongClick", &Menu::onLongClick)
    .function("leaveMenu", &Menu::leaveMenu)
    .function("setTargetLeds", &Menu::setTargetLeds);

  class_<Modes>("Modes")
    .function("init", &Modes::init)
    .function("cleanup", &Modes::cleanup)
    .function("play", &Modes::play)
    .function("saveToBuffer", select_overload<bool(ByteStream&)>(&Modes::saveToBuffer))
    .function("loadFromBuffer", select_overload<bool(ByteStream&)>(&Modes::loadFromBuffer))
    .function("loadStorage", &Modes::loadStorage)
    .function("saveStorage", &Modes::saveStorage)
    .function("serialize", &Modes::serialize)
    .function("unserialize", &Modes::unserialize)
    .function("setDefaults", &Modes::setDefaults)
    .function("shiftCurMode", &Modes::shiftCurMode)
    .function("updateCurMode", &Modes::updateCurMode, allow_raw_pointers())
    .function("setCurMode", &Modes::setCurMode, allow_raw_pointers())
    .function("curMode", &Modes::curMode, allow_raw_pointers())
    .function("nextMode", &Modes::nextMode, allow_raw_pointers())
    .function("previousMode", &Modes::previousMode, allow_raw_pointers())
    .function("nextModeSkipEmpty", &Modes::nextModeSkipEmpty, allow_raw_pointers())
    .function("numModes", &Modes::numModes)
    .function("curModeIndex", &Modes::curModeIndex)
    .function("lastSwitchTime", &Modes::lastSwitchTime)
    .function("deleteCurMode", &Modes::deleteCurMode)
    .function("clearModes", &Modes::clearModes)
    .function("setStartupMode", &Modes::setStartupMode)
    .function("startupMode", &Modes::startupMode)
    .function("initCurMode", &Modes::initCurMode, allow_raw_pointers())
    .function("saveCurMode", &Modes::saveCurMode)
    .function("setFlag", &Modes::setFlag)
    .function("getFlag", &Modes::getFlag)
    .function("resetFlags", &Modes::resetFlags)
    .function("setOneClickMode", &Modes::setOneClickMode)
    .function("oneClickModeEnabled", &Modes::oneClickModeEnabled)
    .function("setLocked", &Modes::setLocked)
    .function("locked", &Modes::locked)
    .function("setAdvancedMenus", &Modes::setAdvancedMenus)
    .function("advancedMenusEnabled", &Modes::advancedMenusEnabled)
    .function("setKeychainMode", &Modes::setKeychainMode)
    .function("keychainModeEnabled", &Modes::keychainModeEnabled);

  class_<VortexEngine>("VortexEngine")
    .function("init", &VortexEngine::init)
    .function("serial", &VortexEngine::serial)
    .function("time", &VortexEngine::time)
    .function("storage", &VortexEngine::storage)
    .function("irReceiver", &VortexEngine::irReceiver)
    .function("irSender", &VortexEngine::irSender)
    .function("vlReceiver", &VortexEngine::vlReceiver)
    .function("vlSender", &VortexEngine::vlSender)
    .function("leds", &VortexEngine::leds)
    .function("buttons", &VortexEngine::buttons)
    .function("button", &VortexEngine::button)
    .function("menus", &VortexEngine::menus)
    .function("modes", &VortexEngine::modes)
    .function("patternBuilder", &VortexEngine::patternBuilder);

  class_<Vortex>("Vortex")
    .constructor<>()
    .function("init", select_overload<void()>(&Vortex::init))
    .function("setInstantTimestep", &Vortex::setInstantTimestep)
    .function("shortClick", &Vortex::shortClick)
    .function("longClick", &Vortex::longClick)
    .function("menuEnterClick", &Vortex::menuEnterClick)
    .function("advMenuEnterClick", &Vortex::advMenuEnterClick)
    .function("deleteColClick", &Vortex::deleteColClick)
    .function("sleepClick", &Vortex::sleepClick)
    .function("forceSleepClick", &Vortex::forceSleepClick)
    .function("pressButton", &Vortex::pressButton)
    .function("releaseButton", &Vortex::releaseButton)
    .function("isButtonPressed", &Vortex::isButtonPressed)
    .function("sendWait", &Vortex::sendWait)
    .function("rapidClick", &Vortex::rapidClick)
    .function("getMenuDemoMode", &Vortex::getMenuDemoMode, allow_raw_pointer<arg<1>>())
    .function("setMenuDemoMode", &Vortex::setMenuDemoMode, allow_raw_pointer<arg<1>>())
    .function("quitClick", &Vortex::quitClick)
    .function("IRDeliver", &Vortex::IRDeliver)
    .function("VLDeliver", &Vortex::VLDeliver)
    //.function("getStorageStats", &Vortex::getStorageStats)
    .function("loadStorage", &Vortex::loadStorage)
    .function("openRandomizer", &Vortex::openRandomizer)
    .function("openColorSelect", &Vortex::openColorSelect)
    .function("openPatternSelect", &Vortex::openPatternSelect)
    .function("openGlobalBrightness", &Vortex::openGlobalBrightness)
    .function("openFactoryReset", &Vortex::openFactoryReset)
    .function("openModeSharing", &Vortex::openModeSharing)
    .function("openEditorConnection", &Vortex::openEditorConnection)
    .function("clearMenuTargetLeds", &Vortex::clearMenuTargetLeds)
    .function("setMenuTargetLeds", &Vortex::setMenuTargetLeds)
    .function("addMenuTargetLeds", &Vortex::addMenuTargetLeds)
    .function("getModes", &Vortex::getModes)
    .function("setModes", &Vortex::setModes)
    .function("getCurMode", &Vortex::getCurMode)
    .function("getCurModeRaw", &Vortex::getCurModeRaw)
    .function("matchLedCount", &Vortex::matchLedCount)
    .function("checkLedCount", &Vortex::checkLedCount)
    .function("setLedCount", &Vortex::setLedCount)
    .function("clearModes", &Vortex::clearModes)
    .function("curModeIndex", &Vortex::curModeIndex)
    .function("numModes", &Vortex::numModes)
    .function("numLedsInMode", &Vortex::numLedsInMode)
    .function("addNewMode", select_overload<bool(bool)>(&Vortex::addNewMode))
    .function("addNewMode", select_overload<bool(ByteStream &, bool)>(&Vortex::addNewMode))
    .function("addNewModeRaw", &Vortex::addNewModeRaw)
    .function("setCurMode", &Vortex::setCurMode)
    .function("nextMode", &Vortex::nextMode)
    .function("delCurMode", &Vortex::delCurMode)
    .function("shiftCurMode", &Vortex::shiftCurMode)
    //.function("setPattern", &Vortex::setPattern)
    .function("getPatternID", &Vortex::getPatternID)
    .function("getPatternName", &Vortex::getPatternName)
    .function("getModeName", &Vortex::getModeName)
    //.function("setPatternAt", &Vortex::setPatternAt)
    .function("getColorset", &Vortex::getColorset)
    .function("setColorset", &Vortex::setColorset)
    .function("getPatternArgs", &Vortex::getPatternArgs)
    .function("setPatternArgs", &Vortex::setPatternArgs)
    .function("isCurModeMulti", &Vortex::isCurModeMulti)
    .function("patternToString", &Vortex::patternToString)
    .function("ledToString", &Vortex::ledToString)
    .function("numCustomParams", &Vortex::numCustomParams)
    .function("getCustomParams", &Vortex::getCustomParams)
    .function("setUndoBufferLimit", &Vortex::setUndoBufferLimit)
    .function("addUndoBuffer", &Vortex::addUndoBuffer)
    .function("undo", &Vortex::undo)
    .function("redo", &Vortex::redo)
    .function("setTickrate", &Vortex::setTickrate)
    .function("getTickrate", &Vortex::getTickrate)
    .function("enableUndo", &Vortex::enableUndo)
    //.function("vcallbacks", &Vortex::vcallbacks)
    .function("doCommand", &Vortex::doCommand)
    .function("setSleepEnabled", &Vortex::setSleepEnabled)
    .function("sleepEnabled", &Vortex::sleepEnabled)
    .function("enterSleep", &Vortex::enterSleep)
    .function("isSleeping", &Vortex::isSleeping)
    .function("enableCommandLog", &Vortex::enableCommandLog)
    .function("getCommandLog", &Vortex::getCommandLog)
    .function("clearCommandLog", &Vortex::clearCommandLog)
    .function("enableLockstep", &Vortex::enableLockstep)
    .function("isLockstep", &Vortex::isLockstep)
    .function("enableStorage", &Vortex::enableStorage)
    .function("storageEnabled", &Vortex::storageEnabled)
    .function("setStorageFilename", &Vortex::setStorageFilename)
    .function("getStorageFilename", &Vortex::getStorageFilename)
    .function("printModeJson", &Vortex::printModeJson)
    .function("parseModeJson", &Vortex::parseModeJson)
    .function("printPatternJson", &Vortex::printPatternJson)
    .function("parsePatternJson", &Vortex::parsePatternJson)
    .function("printJson", &Vortex::printJson)
    .function("parseJson", &Vortex::parseJson)
    .function("setLockEnabled", &Vortex::setLockEnabled)
    .function("lockEnabled", &Vortex::lockEnabled)
    .function("engine", &Vortex::engine)
    .function("getVersion", &Vortex::getVersion)
    .function("getVersionMajor", &Vortex::getVersionMajor)
    .function("getVersionMinor", &Vortex::getVersionMinor)
    .function("getVersionBuild", &Vortex::getVersionBuild);

  function("getDataArray", &getDataArray);
  function("getRawDataArray", &getRawDataArray);
  function("createByteStreamFromData", &createByteStreamFromData);
  function("createByteStreamFromRawData", &createByteStreamFromRawData);

}
#endif

using namespace std;

// I wish there was a way to do this automatically but it would be
// quite messy and idk if it's worth it
static const char *patternNames[PATTERN_COUNT] = {
  "strobe", "hyperstrobe", "picostrobe", "strobie", "dops", "ultradops", "strobegap",
  "hypergap", "picogap", "strobiegap", "dopsgap", "ultragap", "blinkie",
  "ghostcrush", "doubledops", "chopper", "dashgap", "dashdops", "dash-crush",
  "ultradash", "gapcycle", "dashcycle", "tracer", "ribbon", "miniribbon",
  "blend", "blendstrobe", "blendstrobegap", "complementary_blend",
  "complementary_blendstrobe", "complementary_blendstrobegap", "solid",
  "hueshift", "theater_chase", "chaser", "zigzag", "zipfade", "drip",
  "dripmorph", "crossdops", "doublestrobe", "meteor", "sparkletrace",
  "vortexwipe", "warp", "warpworm", "snowball", "lighthouse", "pulsish",
  "fill", "bounce", "splitstrobie", "backstrobe", "vortex",
};

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  // Perform actions based on the reason for calling.
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
#endif

// called when engine reads digital pins, use this to feed button presses to the engine
long VortexCallbacks::checkPinHook(uint32_t pin)
{
  // leaving the pin checking to user-overrides rather than
  // checking the default pins here
  return 1; // HIGH
}

Vortex::Vortex() :
  m_engine(*this),
  m_undoBuffer(),
  m_undoLimit(0),
  m_undoIndex(0),
  m_undoEnabled(true),
  m_storedCallbacks(nullptr),
  m_consoleHandle(nullptr),
#if LOG_TO_FILE == 1
  m_logHandle(nullptr),
#endif
#ifdef WASM
  // pointer to the led array and led count in the engine
  m_leds(nullptr),
  m_led_count(0),
#endif
  m_buttonEventQueue(),
  m_initialized(false),
  m_buttonsPressed(0),
  m_selectedButton(0),
  m_commandLog(),
  m_commandLogEnabled(false),
  m_lockstepEnabled(false),
  m_storageEnabled(false),
  m_sleepEnabled(true),
  m_lockEnabled(true),
  m_lastCommand(0),
  m_randCtx(0)
{
  // default callbacks pointer that can be replaced with a derivative
  // of the VortexCallbacks class
  m_storedCallbacks = new VortexCallbacks(*this);
  if (!m_storedCallbacks) {
    // error! out of memory
  }
#ifdef _WIN32
  m_randCtx.seed((uint32_t)(time(NULL) ^ GetTickCount64()));
#else
  struct timespec ts;
  unsigned long theTick = 0U;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  theTick = ts.tv_nsec / 1000000;
  theTick += ts.tv_sec * 1000;
  m_randCtx.seed(time(NULL) ^ theTick);
#endif
}

Vortex::~Vortex()
{
  cleanup();
}

bool Vortex::init(VortexCallbacks *callbacks)
{
  if (m_storedCallbacks) {
    delete m_storedCallbacks;
  }
  // store the callbacks object
  m_storedCallbacks = callbacks;

#if LOG_TO_CONSOLE == 1
  if (!m_consoleHandle) {
#ifdef _WIN32
    AllocConsole();
    freopen_s(&m_consoleHandle, "CONOUT$", "w", stdout);
#else
    m_consoleHandle = stdout;
#endif
  }
#endif
#if LOG_TO_FILE == 1
  if (!m_logHandle) {
    time_t t = time(nullptr);
    tm tm;
    localtime_s(&tm, &t);
    ostringstream oss;
    oss << put_time(&tm, "%d-%m-%Y-%H-%M-%S");
    oss << "." << GetCurrentProcessId();
    string filename = VORTEX_LOG_NAME "." + oss.str() + ".txt";
    int err = fopen_s(&m_logHandle, filename.c_str(), "w");
    if (err != 0 || !m_logHandle) {
      MessageBox(NULL, "Failed to open logfile", to_string(err).c_str(), 0);
      return false;
    }
  }
#endif

  // init the engine
  m_engine.init();
  // load the modes
  // TODO: don't load modes here? separate api?
  m_engine.modes().load();
  // clear the modes
  //m_engine.modes().clearModes();
  // save and set undo buffer
  doSave();

  m_initialized = true;

  return true;
}

void Vortex::cleanup()
{
  m_engine.cleanup();
  if (m_storedCallbacks) {
    delete m_storedCallbacks;
    m_storedCallbacks = nullptr;
  }
  m_initialized = false;
}

void Vortex::handleNumber(char c)
{
  if (!isdigit(c) || !m_lastCommand) {
    return;
  }
  int number = c - '0';
  char newc = 0;
  // read the digits into the repeatAmount
  while (1) {
    newc = getchar();
    if (!isdigit(newc)) {
      // stop once we reach a non digit
      break;
    }
    // accumulate the digits into the repeat amount
    number = (number * 10) + (newc - '0');
  }
  // shove the last non-digit back into the stream
  ungetc(newc, stdin);
  DEBUG_LOGF("Repeating last command (%c) x%u times", m_lastCommand, number);
  m_commandLog += to_string(number);
  // check to see if we are repeating a 'rapid click' which is a special case
  // because the rapid click command itself is composed or 'r' and a repeat count
  // to designate how many rapid clicks to deliver
  switch (m_lastCommand) {
  case 'r':
    // perform a rapid click number times
    rapidClick(number);
    break;
  case 'b':
    // select button index to target for click events
    selectButton(number);
    break;
  default: // any other command
    if (number > 0) {
      // offset repeat amount by exactly 1 because it's already done
      number--;
    }
    // repeat the last command that many times
    while (number > 0) {
      doCommand(m_lastCommand);
      number--;
    }
    break;
  }
}

// injects a command into the engine, the engine will parse one command
// per tick so multiple commands will be queued up
void Vortex::doCommand(char c)
{
  if (!isprint(c)) {
    return;
  }

  m_commandLog += c;

  switch (c) {
  case 'c':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting short click");
    }
    shortClick();
    break;
  case 'l':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting long click");
    }
    longClick();
    break;
  case 'm':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting menu enter click");
    }
    menuEnterClick();
    break;
  case 'a':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting adv menu enter click");
    }
    advMenuEnterClick();
    break;
  case 'd':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting adv menu enter click");
    }
    deleteColClick();
    break;
  case 's':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting sleep click");
    }
    sleepClick();
    break;
  case 'f':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting force sleep click");
    }
    forceSleepClick();
    break;
  case 'q':
    //case '\n':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting quit click\n");
    }
    quitClick();
    break;
  case 't':
    if (isButtonPressed()) {
      if (m_lastCommand != c) {
        DEBUG_LOG("Injecting release");
      }
      releaseButton();
    } else {
      if (m_lastCommand != c) {
        DEBUG_LOG("Injecting press");
      }
      pressButton();
    }
    break;
  case 'r':
    // do nothing for the initial 'r', handleNumber() will handle the numeric
    // repeat count that follows this letter and issue the rapidClick(amt) call
    break;
  case 'b':
    // button selector, same as rapid click, it's a little weird because the number
    // it not a repeat amount but actually the button index to select -- but none
    // the less it works the same as rapid click so it will be handled inside handleNumber()
    break;
  case 'w':
    if (m_lastCommand != c) {
      DEBUG_LOG("Injecting wait");
    }
    sendWait();
    break;
  default:
    handleNumber(c);
    // return instead of break because this isn't a command
    return;
  }
  m_lastCommand = c;
}

// whether the engine has sleep enabled, if disabled it will always be awake
void Vortex::setSleepEnabled(bool enable)
{
  m_sleepEnabled = enable;
}

bool Vortex::sleepEnabled()
{
  return m_sleepEnabled;
}

// whether the engine is sleeping, and/or to enter sleep
void Vortex::enterSleep(bool save)
{
  m_engine.enterSleep(save);
}

bool Vortex::isSleeping()
{
  return m_engine.isSleeping();
}

bool Vortex::tick()
{
  if (!m_initialized) {
    // do not cleanup prematurely here
    return false;
  }
  // use ioctl to determine how many characters are on stdin so that
  // we don't call getchar() too many times and accidentally block
  uint32_t numInputs = getNumInputs();
  if (m_lockstepEnabled && !numInputs) {
    // don't tick till we have input
    return true;
  }
  // iterate the number of inputs on stdin and parse each letter
  // into a command for the engine, this will inject all of the commands
  // that are available into the engine but that doesn't necessarily
  // mean that the engine will do anything with them right away
  for (uint32_t i = 0; i < numInputs; ++i) {
    doCommand(getchar());
  }
  // tick the vortex engine forward
  m_engine.tick();
  return true;
}


void Vortex::installCallbacks(VortexCallbacks *callbacks)
{
  m_storedCallbacks = callbacks;
}

void Vortex::setInstantTimestep(bool timestep)
{
  m_engine.time().setInstantTimestep(timestep);
}

// select the button to send clicks to (0 = first button)
void Vortex::selectButton(uint8_t buttonIndex)
{
  if (buttonIndex >= m_engine.buttons().numButtons()) {
    return;
  }
  m_selectedButton = buttonIndex;
}

// send various clicks
void Vortex::shortClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_SHORT_CLICK));
}

void Vortex::longClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_LONG_CLICK));
}

void Vortex::menuEnterClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_MENU_ENTER_CLICK));
}

void Vortex::advMenuEnterClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_ADV_MENU_ENTER_CLICK));
}

void Vortex::deleteColClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_DELETE_COL));
}

void Vortex::sleepClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_SLEEP_CLICK));
}

void Vortex::forceSleepClick(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonEventQueue.push_back(VortexButtonEvent(buttonIndex, EVENT_FORCE_SLEEP_CLICK));
}

void Vortex::pressButton(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonsPressed |= (1 << buttonIndex);
}

void Vortex::releaseButton(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  m_buttonsPressed &= ~(1 << buttonIndex);
}

bool Vortex::isButtonPressed(uint8_t buttonIndex)
{
  if (!buttonIndex) {
    buttonIndex = m_selectedButton;
  }
  return (m_buttonsPressed & (1 << buttonIndex)) != 0;
}

void Vortex::sendWait(uint32_t amount)
{
  // reusing the button index as the wait amount
  m_buttonEventQueue.push_back(VortexButtonEvent(amount, EVENT_WAIT));
}

void Vortex::rapidClick(uint32_t amount)
{
  // reusing the button index as the consecutive press amount
  m_buttonEventQueue.push_back(VortexButtonEvent(amount, EVENT_RAPID_CLICK));
}

Mode *Vortex::getMenuDemoMode()
{
  Menu *pMenu = m_engine.menus().curMenu();
  if (!pMenu) {
    return nullptr;
  }
  return &pMenu->m_previewMode;
}

bool Vortex::setMenuDemoMode(const Mode *mode)
{
  if (!mode) {
     return false;
  }
  Menu *pMenu = m_engine.menus().curMenu();
  if (!pMenu) {
    return false;
  }
  if (!mode->getLedCount() || pMenu->m_previewMode.equals(mode)) {
    return false;
  }
  pMenu->m_previewMode = *mode;
  pMenu->m_previewMode.init();
  return true;
}

void Vortex::quitClick()
{
  m_buttonEventQueue.push_back(VortexButtonEvent(0, EVENT_QUIT_CLICK));
}

void Vortex::IRDeliver(uint32_t timing)
{
#if IR_ENABLE_RECEIVER == 1
  m_engine.irReceiver().handleIRTiming(timing);
#endif
}

void Vortex::VLDeliver(uint32_t timing)
{
#if VL_ENABLE_RECEIVER == 1
  m_engine.vlReceiver().handleVLTiming(timing);
#endif
}

void Vortex::getStorageStats(uint32_t *outTotal, uint32_t *outUsed)
{
  if (outTotal) {
    *outTotal = m_engine.totalStorageSpace();
  }
  if (outUsed) {
    *outUsed = m_engine.savefileSize();
  }
}

void Vortex::loadStorage()
{
  m_engine.modes().loadStorage();
}

void Vortex::openRandomizer(bool advanced)
{
  m_engine.menus().openMenu(MENU_RANDOMIZER, advanced);
}

void Vortex::openColorSelect(bool advanced)
{
  m_engine.menus().openMenu(MENU_COLOR_SELECT, advanced);
}

void Vortex::openPatternSelect(bool advanced)
{
  m_engine.menus().openMenu(MENU_PATTERN_SELECT, advanced);
}

void Vortex::openGlobalBrightness(bool advanced)
{
  m_engine.menus().openMenu(MENU_GLOBAL_BRIGHTNESS, advanced);
}

void Vortex::openFactoryReset(bool advanced)
{
  m_engine.menus().openMenu(MENU_FACTORY_RESET, advanced);
}

void Vortex::openModeSharing(bool advanced)
{
  m_engine.menus().openMenu(MENU_MODE_SHARING, advanced);
}

void Vortex::openEditorConnection(bool advanced)
{
#if ENABLE_EDITOR_CONNECTION == 1
  m_engine.menus().openMenu(MENU_EDITOR_CONNECTION, advanced);
#endif
}

void Vortex::clearMenuTargetLeds()
{
  Menu *cur = m_engine.menus().curMenu();
  if (!cur) {
    return;
  }
  cur->setTargetLeds(MAP_LED_NONE);
}

void Vortex::setMenuTargetLeds(LedMap targetLeds)
{
  Menu *cur = m_engine.menus().curMenu();
  if (!cur) {
    return;
  }
  cur->setTargetLeds(targetLeds);
}

void Vortex::addMenuTargetLeds(LedPos pos)
{
  Menu *cur = m_engine.menus().curMenu();
  if (!cur) {
    return;
  }
  LedMap curMap = cur->getTargetLeds();
  cur->setTargetLeds((LedMap)(curMap | MAP_LED(pos)));
}

bool Vortex::getModes(ByteStream &outStream)
{
  // now serialize all the modes
  return m_engine.modes().saveToBuffer(outStream);
}

bool Vortex::setModes(ByteStream &stream, bool save)
{
  clearModes();
  // now unserialize the stream of data that was read
  if (!m_engine.modes().loadFromBuffer(stream)) {
    return false;
  }
  return !save || doSave();
}

bool Vortex::matchLedCount(ByteStream &stream, bool vtxMode)
{
  if (!stream.decompress()) {
    return false;
  }
  // reset the unserializer index before unserializing anything
  stream.resetUnserializer();
  uint8_t major = 0;
  uint8_t minor = 0;
  // unserialize the vortex version
  stream.unserialize8(&major);
  stream.unserialize8(&minor);
  // unserialize the global brightness
  if (!vtxMode) {
    uint8_t flags;
    stream.unserialize8(&flags);
    uint8_t brightness = 0;
    stream.unserialize8(&brightness);
    uint8_t numModes = 0;
    stream.unserialize8(&numModes);
  }
  uint8_t ledCount = 0;
  stream.unserialize8(&ledCount);
  // put the unserializer back where it was for the next thing
  stream.resetUnserializer();
  return setLedCount(ledCount);
}

bool Vortex::checkLedCount()
{
  Mode *mode = m_engine.modes().curMode();
  if (!mode) {
    return false;
  }
  uint8_t numLeds = mode->getLedCount();
  if (numLeds != LED_COUNT) {
    m_engine.leds().setLedCount(numLeds);
  }
  return true;
}

uint8_t Vortex::setLedCount(uint8_t count)
{
  // must change the 'leds' led count before changing a mode
  m_engine.leds().setLedCount(count);
  // if the current mdoe is instantiated then need to update it too
  if (m_engine.modes().curModeInstantiated()) {
    Mode *cur = m_engine.modes().curMode();
    if (cur && !cur->setLedCount(count)) {
      return false;
    }
  }
  return true;
}

uint8_t Vortex::getLedCount()
{
  return m_engine.leds().ledCount();
}

bool Vortex::getCurMode(ByteStream &outStream)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  // save to ensure we get the correct mode, not using doSave() because it causes
  // an undo buffer entry to be added
  if (!m_engine.modes().saveStorage()) {
    return false;
  }
  return m_engine.modes().curMode()->saveToBuffer(outStream);
}

// same as getCurMode but with just the raw mode buffer instead of the full mode save
bool Vortex::getCurModeRaw(ByteStream &outStream)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  // save to ensure we get the correct mode, not using doSave() because it causes
  // an undo buffer entry to be added
  if (!m_engine.modes().saveStorage()) {
    return false;
  }
  if (!m_engine.modes().curMode()->serialize(outStream)) {
    return false;
  }
  return outStream.recalcCRC();
}

void Vortex::clearModes()
{
  m_engine.modes().clearModes();
}

uint32_t Vortex::curModeIndex()
{
  return m_engine.modes().curModeIndex();
}

uint32_t Vortex::numModes()
{
  return m_engine.modes().numModes();
}

uint32_t Vortex::numLedsInMode()
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  return pMode->getLedCount();
}

bool Vortex::addMode(const Mode *mode, bool save)
{
  if (!m_engine.modes().addMode(mode)) {
    return false;
  }
  return !save || doSave();
}

bool Vortex::addNewMode(bool save)
{
  Colorset set;
  set.randomize(m_randCtx);
  // create a random pattern ID from all patterns
  PatternID randomPattern;
  do {
    // continuously re-randomize the pattern so we don't get solids
    randomPattern = (PatternID)m_randCtx.next16(PATTERN_FIRST, PATTERN_SINGLE_LAST);
  } while (randomPattern == PATTERN_SOLID);
  if (!m_engine.modes().addMode(randomPattern, nullptr, &set)) {
    return false;
  }
  return !save || doSave();
}

bool Vortex::addNewMode(ByteStream &stream, bool save)
{
  if (!m_engine.modes().addModeFromBuffer(stream)) {
    return false;
  }
  return !save || doSave();
}

// this function exists to aid in adding duo modes which don't come with
// the regular mode saveheader attached to them, they need to be loaded
// with mode.unserialize rather than mode.loadFromBuffer
bool Vortex::addNewModeRaw(ByteStream &stream, bool save)
{
  Mode tmp(m_engine, getLedCount());
  if (!tmp.unserialize(stream)) {
    return false;
  }
  if (!m_engine.modes().addMode(&tmp)) {
    return false;
  }
  return !save || doSave();
}

bool Vortex::setCurMode(uint32_t index, bool save)
{
  if (index >= m_engine.modes().numModes()) {
    return true;
  }
  if (!m_engine.modes().setCurMode(index)) {
    return false;
  }
  return !save || doSave();
}

bool Vortex::nextMode(bool save)
{
  if (!m_engine.modes().numModes()) {
    return true;
  }
  m_engine.modes().nextMode();
  return !save || doSave();
}

bool Vortex::delCurMode(bool save)
{
  if (!m_engine.modes().numModes()) {
    return true;
  }
  m_engine.modes().deleteCurMode();
  return !save || doSave();
}

bool Vortex::shiftCurMode(int8_t offset, bool save)
{
  if (!m_engine.modes().numModes()) {
    return true;
  }
  if (offset == 0) {
    return true;
  }
  m_engine.modes().shiftCurMode(offset);
  return !save || doSave();
}

bool Vortex::setPattern(PatternID id, const PatternArgs *args, const Colorset *set, bool save)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  if (!pMode->setPattern(id, LED_ANY, args, set)) {
    return false;
  }
  pMode->init();
  return !save || doSave();
}

PatternID Vortex::getPatternID(LedPos pos)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return PATTERN_NONE;
  }
  return pMode->getPatternID(pos);
}

string Vortex::getPatternName(LedPos pos)
{
  PatternID id = getPatternID(pos);
  PatternArgs args;
  if (getPatternArgs(pos, args) && !m_engine.patternBuilder().isDefaultArgs(id, args)) {
    return "custom";
  }
  return patternToString(id);
}

string Vortex::getModeName()
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return patternToString(PATTERN_NONE);
  }
  if (pMode->isMultiLed()) {
    return getPatternName(LED_MULTI);
  }
  if (pMode->hasSameSingleLed()) {
    return getPatternName(LED_FIRST);
  }
  return "custom";
}

bool Vortex::setPatternAt(LedPos pos, PatternID id,
  const PatternArgs *args, const Colorset *set, bool save)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  if (!pMode->setPattern(id, pos, args, set)) {
    return false;
  }
  pMode->init();
  return !save || doSave();
}

bool Vortex::getColorset(LedPos pos, Colorset &set)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  set = pMode->getColorset(pos);
  return true;
}

bool Vortex::setColorset(LedPos pos, const Colorset &set, bool save)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  if (!pMode->setColorset(set, pos)) {
    return false;
  }
  pMode->init();
  return !save || doSave();
}

bool Vortex::getPatternArgs(LedPos pos, PatternArgs &args)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  Pattern *pat = pMode->getPattern(pos);
  if (!pat) {
    return false;
  }
  pat->getArgs(args);
  return true;
}

bool Vortex::setPatternArgs(LedPos pos, PatternArgs &args, bool save)
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  Pattern *pat = nullptr;
  // Equivalent to cases LED_ANY, LED_ALL and LED_MULTI
  if (pos == LED_ANY || pos == LED_ALL || pos == LED_MULTI) {
#if VORTEX_SLIM == 0
    pat = pMode->getPattern(LED_MULTI);
    if (pat) {
      pat->setArgs(args);
    }
    if (pos == LED_MULTI) {
      // don't fallthrough if actually multi, it's possible
      // we got here by falling through from LED_ALL
      pMode->init();
      return !save || doSave();
    }
    // fall through if LED_ALL and change the single leds
    else if (pos == LED_ANY || pos == LED_ALL) {
      for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
        pat = pMode->getPattern(pos);
        if (pat) {
          pat->setArgs(args);
        }
      }
      pMode->init();
      // actually break here
      return !save || doSave();
    }
#endif
  }

  // equivalent to case LED_ALL_SINGLE
  else if (pos == LED_ALL_SINGLE) {
    for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
      pat = pMode->getPattern(pos);
      if (pat) {
        pat->setArgs(args);
      }
    }
    pMode->init();
    // actually break here
    return !save || doSave();
  }

  // equivalent to default case (covers any other pos)
  else {
    if (pos >= LED_COUNT) {
      return false;
    }
    pat = pMode->getPattern(pos);
    if (!pat) {
      return false;
    }
    pat->setArgs(args);
    pMode->init();
    return !save || doSave();
  }

  return false;
}

bool Vortex::isCurModeMulti()
{
  Mode *pMode = m_engine.modes().curMode();
  if (!pMode) {
    return false;
  }
  return pMode->isMultiLed();
}

string Vortex::patternToString(PatternID id)
{
  // this shouldn't happen but just in case somebody messes with stuff
  if (sizeof(patternNames) / sizeof(patternNames[0]) != PATTERN_COUNT) {
    // if you see this it means the list of strings above is not equal to
    // the number of patterns in the enum, so you need to update the list
    // above to match the enum.
    return "fix patternToString()";
  }
  if (id <= PATTERN_NONE || id >= PATTERN_COUNT) {
    return "none";
  }
  return patternNames[id];
}

PatternID Vortex::stringToPattern(const std::string &pattern)
{
  static map<string, PatternID> cachedNames;
  if (cachedNames.empty()) {
    for (PatternID i = PATTERN_FIRST; i < PATTERN_COUNT; ++i) {
      cachedNames[patternNames[i]] = (PatternID)i;
    }
  }
  // lowercase the name and look it up
  string lowerPat = pattern;
  transform(lowerPat.begin(), lowerPat.end(), lowerPat.begin(), [](unsigned char c) { return tolower(c); });
  if (cachedNames.find(pattern) == cachedNames.end()) {
    return PATTERN_NONE;
  }
  return cachedNames[pattern];
}

// this shouldn't change much so this is fine
string Vortex::ledToString(LedPos pos)
{
  return "led " + to_string(pos);
}

// the number of custom parameters for any given pattern id
uint32_t Vortex::numCustomParams(PatternID id)
{
  return m_engine.patternBuilder().numDefaultArgs(id);
}

vector<string> Vortex::getCustomParams(PatternID id)
{
  Pattern *pat = m_engine.patternBuilder().make(id);
  vector<string> params;
  if (!pat) {
    return params;
  }
  for (uint32_t i = 0; i < pat->getNumArgs(); ++i) {
    params.push_back(pat->getArgName(i));
  }
  delete pat;
  return params;
}

void Vortex::setUndoBufferLimit(uint32_t limit)
{
  m_undoLimit = limit;
}

bool Vortex::addUndoBuffer()
{
  if (!m_undoEnabled) {
    return true;
  }
  ByteStream modes;
  getModes(modes);
  if (!modes.size()) {
    return false;
  }
  // only save an undo if the buffer is different
  if (m_undoBuffer.size() > 0 && modes.CRC() == m_undoBuffer.back().CRC()) {
    //printf("Not saving duplicate undo buffer\n");
    return false;
  }
  // must rewind to that step before pushing next step
  while (m_undoIndex > 0) {
    m_undoBuffer.pop_back();
    m_undoIndex--;
  }
  m_undoBuffer.push_back(modes);
  //printf("Pushing undo buffer (pos: %u)\n", m_undoIndex);
  // make sure list doesn't grow too big
  if (m_undoLimit && m_undoBuffer.size() > m_undoLimit) {
    //printf("Popping front of undo buffer\n");
    m_undoBuffer.pop_front();
  }
#if 0
  printf("Buffer:\n");
  for (uint32_t i = 0; i < m_undoBuffer.size(); ++i) {
    printf("\t%u: %x", i, m_undoBuffer[i].CRC());
    if ((m_undoBuffer.size() - 1) - m_undoIndex == i) {
      printf(" <--\n");
    } else {
      printf("\n");
    }
  }
#endif
  return true;
}

bool Vortex::applyUndo()
{
  if (!m_undoBuffer.size()) {
    return false;
  }
  uint32_t highestIndex = (uint32_t)m_undoBuffer.size() - 1;
  if (m_undoIndex > highestIndex) {
    m_undoIndex = highestIndex;
  }
  //printf("Undo position: %u / %u\n", m_undoIndex, highestIndex);
  //printf("Buffer:\n");
#if 0
  for (uint32_t i = 0; i < m_undoBuffer.size(); ++i) {
    printf("\t%u: %x", i, m_undoBuffer[i].CRC());
    if ((m_undoBuffer.size() - 1) - m_undoIndex == i) {
      printf(" <--\n");
    } else {
      printf("\n");
    }
  }
#endif
  // index from the back instead of the front
  uint32_t backIndex = highestIndex - m_undoIndex;
  m_undoBuffer[backIndex].resetUnserializer();
  Vortex::setModes(m_undoBuffer[backIndex], false);
  return true;
}

bool Vortex::undo()
{
  if (!m_undoBuffer.size()) {
    return false;
  }
  uint32_t highestIndex = (uint32_t)m_undoBuffer.size() - 1;
  // cannot undo further into history
  if (m_undoIndex > highestIndex) {
    m_undoIndex = highestIndex;
  } else {
    m_undoIndex++;
  }
  return applyUndo();
}

bool Vortex::redo()
{
  if (!m_undoBuffer.size()) {
    return false;
  }
  // cannot undo further into history
  if (m_undoIndex > 0) {
    m_undoIndex--;
  }
  return applyUndo();
}

void Vortex::setTickrate(uint32_t tickrate)
{
  m_engine.time().setTickrate(tickrate);
}

uint32_t Vortex::getTickrate()
{
  return m_engine.time().getTickrate();
}

bool Vortex::doSave()
{
  return m_engine.modes().saveStorage() && addUndoBuffer();
}

void Vortex::handleInputQueue(Button *buttons, uint32_t numButtons)
{
  // if there's nothing in the queue just return
  if (!m_buttonEventQueue.size()) {
    return;
  }
  // pop the event from the front of the queue
  VortexButtonEvent buttonEvent = m_buttonEventQueue.front();
  m_buttonEventQueue.pop_front();
  // the target button for this event (no target if event is just 'wait')
  Button *pButton = buttons;
  if (buttonEvent.type != EVENT_WAIT && buttonEvent.type != EVENT_RAPID_CLICK) {
    // make sure the button that is targeted is actually a valid index
    if (buttonEvent.target >= numButtons) {
      return;
    }
    // assigned the button based on the array index target
    pButton = buttons + buttonEvent.target;
  }
  // switch on the type of event and then run the operation
  switch (buttonEvent.type) {
  case EVENT_NONE:
  default:
    break;
  case EVENT_SHORT_CLICK:
    pButton->m_newRelease = true;
    pButton->m_shortClick = true;
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = SHORT_CLICK_THRESHOLD_TICKS - 1;
    DEBUG_LOG("Injecting short click");
    break;
  case EVENT_LONG_CLICK:
    pButton->m_newRelease = true;
    pButton->m_longClick = true;
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = SHORT_CLICK_THRESHOLD_TICKS + 1;
    DEBUG_LOG("Injecting long click");
    break;
  case EVENT_MENU_ENTER_CLICK:
    // to do this we simply press the button and set the press time
    // to something more than the menu trigger threshold that will make
    // us immediately enter the menus. But we need to unset the pressed
    // button right after so we push a reset click event to reset the button
    pButton->m_pressTime = m_engine.time().getCurtime();
#if defined(SLEEP_ENTER_THRESHOLD_TICKS) && defined(SLEEP_WINDOW_THRESHOLD_TICKS)
    // microlight must hold longer (past sleep time)
    pButton->m_holdDuration = MENU_TRIGGER_THRESHOLD_TICKS + SLEEP_ENTER_THRESHOLD_TICKS + SLEEP_WINDOW_THRESHOLD_TICKS + 1;
#else
    pButton->m_holdDuration = MENU_TRIGGER_THRESHOLD_TICKS + 1;
#endif
    pButton->m_isPressed = true;
    m_buttonEventQueue.push_front(VortexButtonEvent(0, EVENT_RESET_CLICK));
    DEBUG_LOG("Injecting menu enter click");
    break;
  case EVENT_ADV_MENU_ENTER_CLICK:
    // to do this we simply press the button and set the press time
    // to something more than the menu trigger threshold that will make
    // us immediately enter the menus. But we need to unset the pressed
    // button right after so we push a reset click event to reset the button
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = ADV_MENU_DURATION_TICKS + 1;
    pButton->m_longClick = true;
    pButton->m_newRelease = true;
    DEBUG_LOG("Injecting adv menu enter click");
    break;
  case EVENT_DELETE_COL:
    // to do this we simply press the button and set the press time
    // to something more than the menu trigger threshold that will make
    // us immediately enter the menus. But we need to unset the pressed
    // button right after so we push a reset click event to reset the button
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = DELETE_THRESHOLD_TICKS + DELETE_CYCLE_TICKS + 1;
    pButton->m_longClick = true;
    pButton->m_newRelease = true;
    DEBUG_LOG("Injecting delete color click");
    break;
  case EVENT_SLEEP_CLICK:
    // to do this we simply press the button and set the press time
    // to something more than the menu trigger threshold that will make
    // us immediately enter the menus. But we need to unset the pressed
    // button right after so we push a reset click event to reset the button
#ifdef SLEEP_ENTER_THRESHOLD_TICKS
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = SLEEP_ENTER_THRESHOLD_TICKS + 1;
    pButton->m_longClick = true;
    pButton->m_newRelease = true;
    DEBUG_LOG("Injecting sleep click");
#endif
    break;
  case EVENT_FORCE_SLEEP_CLICK:
    // to do this we simply press the button and set the press time
    // to something more than the menu trigger threshold that will make
    // us immediately enter the menus. But we need to unset the pressed
    // button right after so we push a reset click event to reset the button
#ifdef FORCE_SLEEP_THRESHOLD_TICKS
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = FORCE_SLEEP_THRESHOLD_TICKS + 1;
    pButton->m_longClick = true;
    pButton->m_newRelease = true;
    DEBUG_LOG("Injecting force sleep click");
#endif
    break;
  case EVENT_WAIT:
    if (buttonEvent.target) {
      // backup the event queue and clear it
      deque<Vortex::VortexButtonEvent> backup;
      swap(backup, m_buttonEventQueue);
      // ticks the engine forward some number of ticks, the event queue is empty
      // so the engine won't process any input events while doing this
      for (uint32_t i = 0; i < buttonEvent.target; ++i) {
        m_engine.tick();
      }
      // then restore the event queue so that events are processed like normal
      swap(backup, m_buttonEventQueue);
    }
    break;
  case EVENT_TOGGLE_CLICK:
    if (pButton->isPressed()) {
      // re-calc all this stuff because there's no api in Button class to do it
      // I don't want to add the api there because it's useless besides for this
      pButton->m_buttonState = false;
      pButton->m_isPressed = false;
      pButton->m_holdDuration = (uint32_t)(m_engine.time().getCurtime() - pButton->m_pressTime);
      pButton->m_releaseTime = m_engine.time().getCurtime();
      pButton->m_newRelease = true;
      pButton->m_shortClick = (pButton->m_holdDuration <= SHORT_CLICK_THRESHOLD_TICKS);
      pButton->m_longClick = !pButton->m_shortClick;
      DEBUG_LOG("Injecting release");
    } else {
      pButton->m_buttonState = true;
      pButton->m_isPressed = true;
      pButton->m_releaseDuration = (uint32_t)(m_engine.time().getCurtime() - pButton->m_releaseTime);
      pButton->m_pressTime = m_engine.time().getCurtime();
      pButton->m_newPress = true;
      DEBUG_LOG("Injecting press");
    }
    break;
  case EVENT_RAPID_CLICK:
    pButton->m_consecutivePresses = buttonEvent.target;
    pButton->m_newRelease = true;
    pButton->m_shortClick = true;
    pButton->m_pressTime = m_engine.time().getCurtime();
    pButton->m_holdDuration = 1;
    DEBUG_LOGF("Injecting %u x rapid click", buttonEvent.target);
    break;
  case EVENT_QUIT_CLICK:
    // just uninitialize so tick returns false
    m_initialized = false;
    DEBUG_LOG("Injecting quit");
    break;
  case EVENT_RESET_CLICK:
    pButton->m_isPressed = false;
    break;
  }
}

uint32_t Vortex::getNumInputs()
{
  uint32_t numInputs = 0;
  // On linux we need to poll stdin for input to handle commands
#if !defined(_WIN32) && !defined(WASM)
  ioctl(STDIN_FILENO, FIONREAD, &numInputs);
#elif defined(_WIN32)
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

  if (GetFileType(hStdin) == FILE_TYPE_CHAR) {
    // Handle console input
    if (!GetNumberOfConsoleInputEvents(hStdin, (DWORD *)&numInputs)) {
      // Handle error here
    }
    INPUT_RECORD inputRecord;
    DWORD eventsRead;
    while (PeekConsoleInput(hStdin, &inputRecord, 1, &eventsRead) && eventsRead > 0) {
      // Discard non-character events
      if (inputRecord.EventType != KEY_EVENT || !inputRecord.Event.KeyEvent.bKeyDown) {
        numInputs--;
      }
      // Remove the event from the input buffer
      ReadConsoleInput(hStdin, &inputRecord, 1, &eventsRead);
    }
  } else {
    // Handle redirected input
    DWORD availableBytes;
    if (PeekNamedPipe(hStdin, NULL, 0, NULL, &availableBytes, NULL)) {
      numInputs = availableBytes;
    } else {
      // Handle error here
    }
  }
#endif
  return numInputs;
}

void Vortex::printlog(const char *file, const char *func, int line, const char *msg, va_list list)
{
  if (!Vortex::m_consoleHandle) {
    return;
  }
  string strMsg;
  if (file) {
    strMsg = file;
    if (strMsg.find_last_of('\\') != string::npos) {
      strMsg = strMsg.substr(strMsg.find_last_of('\\') + 1);
    }
    strMsg += ":";
    strMsg += to_string(line);
  }
  if (func) {
    strMsg += " ";
    strMsg += func;
    strMsg += "(): ";
  }
  strMsg += msg;
  strMsg += "\n";
  vfprintf(Vortex::m_consoleHandle, strMsg.c_str(), list);
#if LOG_TO_FILE == 1
  vfprintf(Vortex::m_logHandle, strMsg.c_str(), list);
#endif
}

void Vortex::setStorageFilename(const string &name)
{
  m_engine.storage().setStorageFilename(name);
}

string Vortex::getStorageFilename()
{
  return m_engine.storage().getStorageFilename();
}

// get the engine version as a string
std::string Vortex::getVersion() const {
  return VORTEX_VERSION;
}

// get the version as separated integers
uint8_t Vortex::getVersionMajor() const {
  return VORTEX_VERSION_MAJOR;
}

uint8_t Vortex::getVersionMinor() const {
  return VORTEX_VERSION_MINOR;
}

uint8_t Vortex::getVersionBuild() const {
  return VORTEX_BUILD_NUMBER;
}

json Vortex::modeToJson(const Mode *mode)
{
  if (!mode) {
    return nullptr;
  }

  json modeJson;
  modeJson["num_leds"] = mode->getLedCount();
  modeJson["flags"] = (uint8_t)mode->getFlags();

  const Pattern *multiPattern = mode->getPattern(LED_MULTI);
  if (multiPattern) {
    modeJson["multi_pat"] = patternToJson(multiPattern);
  }

  json singlePatterns = json::array();
  for (LedPos l = LED_FIRST; l < mode->getLedCount(); ++l) {
    const Pattern *pattern = mode->getPattern(l);
    if (pattern) {
      singlePatterns.push_back(patternToJson(pattern));
    } else {
      singlePatterns.push_back(nullptr);
    }
  }

  modeJson["single_pats"] = singlePatterns;
  return modeJson;
}

Mode *Vortex::modeFromJson(const json &modeJson)
{
  if (modeJson.is_null()) {
    return nullptr;
  }

  Mode *mode = new Mode(m_engine);
  if (!mode) {
    return nullptr;
  }

  if (modeJson.contains("num_leds") && modeJson["num_leds"].is_number_unsigned()) {
    mode->setLedCount(modeJson["num_leds"].get<uint8_t>());
  }

  // Extract and set multiPattern
  if (modeJson.contains("multi_pat") && modeJson["multi_pat"].is_object()) {
    const json &multiPatJson = modeJson["multi_pat"];
    Pattern *multiPattern = patternFromJson(multiPatJson);
    if (multiPattern) {
      PatternArgs args;
      multiPattern->getArgs(args);
      Colorset set = multiPattern->getColorset();
      mode->setPattern(multiPattern->getPatternID(), LED_MULTI, &args, &set);
      delete multiPattern;
    }
  }

  // Extract and set singlePatterns
  if (modeJson.contains("single_pats") && modeJson["single_pats"].is_array()) {
    LedPos pos = LED_FIRST;
    for (const json &patJson : modeJson["single_pats"]) {
      if (patJson.is_object()) {
        Pattern *pattern = patternFromJson(patJson);
        if (pattern) {
          PatternArgs args;
          pattern->getArgs(args);
          Colorset set = pattern->getColorset();
          mode->setPattern(pattern->getPatternID(), pos++, &args, &set);
          delete pattern;
        }
      }
    }
  }

  // fallback to load 1-led pattern data as the mode if multi_pat and single_pats are missing
  if (!modeJson.contains("multi_pat") && !modeJson.contains("single_pats")) {
    Pattern *pattern = patternFromJson(modeJson);
    if (!pattern) {
      return mode;
    }
    PatternArgs args;
    pattern->getArgs(args);
    Colorset set = pattern->getColorset();
    mode->setPattern(pattern->getPatternID(), LED_FIRST, &args, &set);
    delete pattern;
  }

  return mode;
}

json Vortex::patternToJson(const Pattern *pattern)
{
  if (!pattern) {
    return nullptr;
  }

  json patternJson;
  patternJson["pattern_id"] = pattern->getPatternID();
  patternJson["flags"] = pattern->getFlags();

  const Colorset &colorset = pattern->getColorset();
  patternJson["numColors"] = colorset.numColors();

  json colorsetArray = json::array();
  for (uint8_t c = 0; c < colorset.numColors(); ++c) {
    const RGBColor &color = colorset.get(c);
    std::stringstream colorString;
    colorString << "0x"
      << std::setfill('0') << std::setw(2) << std::hex << (int)color.red
      << std::setfill('0') << std::setw(2) << std::hex << (int)color.green
      << std::setfill('0') << std::setw(2) << std::hex << (int)color.blue;
    colorsetArray.push_back(colorString.str());
  }
  patternJson["colorset"] = colorsetArray;

  json argsArray = json::array();
  for (uint8_t a = 0; a < pattern->getNumArgs(); ++a) {
    argsArray.push_back(pattern->getArg(a));
  }
  patternJson["args"] = argsArray;

  return patternJson;
}

Pattern *Vortex::patternFromJson(const json &patternJson)
{
  if (patternJson.is_null()) {
    return nullptr;
  }

  // Get pattern ID
  PatternID id = PATTERN_NONE;
  if (patternJson.contains("pattern_id") && patternJson["pattern_id"].is_number_unsigned()) {
    id = static_cast<PatternID>(patternJson["pattern_id"].get<uint8_t>());
  }

  // Validate the pattern ID
  if (id >= PATTERN_COUNT) {
    return nullptr;
  }

  // Parse out the args
  PatternArgs args;
  if (patternJson.contains("args") && patternJson["args"].is_array()) {
    for (const auto &arg : patternJson["args"]) {
      if (arg.is_number_unsigned()) {
        args.addArgs(static_cast<uint8_t>(arg.get<uint8_t>()));
      }
    }
  }

  // Parse out the colorset
  Colorset set;
  if (patternJson.contains("colorset") && patternJson["colorset"].is_array()) {
    for (const auto &colorElement : patternJson["colorset"]) {
      if (colorElement.is_string()) {
        string strVal = colorElement.get<string>();
        if (strVal.find("0x") == 0) {
          strVal = strVal.substr(2);
        }
        uint32_t dwCol = strtoul(strVal.c_str(), NULL, 16);
        set.addColor(RGBColor(dwCol));
      }
    }
  }

  // Build the pattern with ID + args
  Pattern *pattern = m_engine.patternBuilder().make(id, &args);
  if (!pattern) {
    return nullptr;
  }

  // Apply colorset and init
  pattern->setColorset(set);
  pattern->init();

  return pattern;
}

json Vortex::saveModeToJson()
{
  json saveJson;
  Mode *cur = m_engine.modes().curMode();
  if (!cur) {
    return nullptr;
  }
  return modeToJson(cur);
}

bool Vortex::loadModeFromJson(const json &js)
{
  if (js.is_null()) {
    return false;
  }

  // TODO: reintroduce version to vtxmode format
  //uint8_t major = 0;
  //uint8_t minor = 0;
  //if (js.contains("version_major") && js["version_major"].is_number_unsigned()) {
  //  major = js["version_major"].get<uint8_t>();
  //}

  //if (js.contains("version_minor") && js["version_minor"].is_number_unsigned()) {
  //  minor = js["version_minor"].get<uint8_t>();
  //}

  //if (!m_engine.checkVersion(major, minor)) {
  //  return false;
  //}

  Mode *mode = modeFromJson(js);
  if (!mode) {
    return false;
  }
  bool rv = m_engine.modes().addMode(mode);
  delete mode;
  return rv;
}

json Vortex::saveToJson()
{
  json saveJson;

  saveJson["version_major"] = static_cast<uint8_t>(VORTEX_VERSION_MAJOR);
  saveJson["version_minor"] = static_cast<uint8_t>(VORTEX_VERSION_MINOR);
  saveJson["version_build"] = static_cast<uint8_t>(VORTEX_BUILD_NUMBER);
  saveJson["global_flags"] = m_engine.modes().globalFlags();
  saveJson["brightness"] = static_cast<uint8_t>(m_engine.leds().getBrightness());

  uint8_t numModes = m_engine.modes().numModes();
  saveJson["num_modes"] = numModes;

  json modesArray = json::array();
  m_engine.modes().setCurMode(0);
  for (uint8_t i = 0; i < numModes; ++i) {
    modesArray.push_back(saveModeToJson());
    m_engine.modes().nextMode();
  }
  saveJson["modes"] = modesArray;

  return saveJson;
}

bool Vortex::loadFromJson(const json& js)
{
  if (js.is_null()) {
    return false;
  }

  // if there is a version then load it and compare
  if (js.contains("version_major") && js["version_major"].is_number_unsigned() &&
      js.contains("version_minor") && js["version_minor"].is_number_unsigned()) {
    uint8_t major = js["version_major"].get<uint8_t>();
    uint8_t minor = js["version_minor"].get<uint8_t>();
    // this should change in the future if some new version that isn't compatible
    // it should probably be necessary rather than opportunistic
    if (!m_engine.checkVersion(major, minor)) {
      return false;
    }
  }

  // check if it's just a single mode being loaded
  if (js.contains("num_leds") && js["num_leds"].is_number_unsigned() &&
      js.contains("flags") && js["flags"].is_number_unsigned() &&
      js.contains("single_pats") && js["single_pats"].is_array()) {
    // clear existing modes? idk, yes for now
    clearModes();
    // the js is just a single mode, just load it
    return loadModeFromJson(js);
  }

  if (js.contains("brightness") && js["brightness"].is_number_unsigned()) {
    uint8_t brightness = js["brightness"].get<uint8_t>();
    m_engine.leds().setBrightness(brightness);
  }

  if (js.contains("global_flags") && js["global_flags"].is_number_unsigned()) {
    uint8_t global_flags = js["global_flags"].get<uint8_t>();
    m_engine.modes().setFlag(global_flags, true, false);
  }

  uint8_t num_modes = 0;
  if (js.contains("num_modes") && js["num_modes"].is_number_unsigned()) {
    num_modes = js["num_modes"].get<uint8_t>();
  }

  if (!js.contains("modes") || !js["modes"].is_array()) {
    return false;
  }

  clearModes();
  for (const auto &modeValue : js["modes"]) {
    if (modeValue.is_null() || !modeValue.is_object()) {
      continue;
    }
    if (!loadModeFromJson(modeValue)) {
      // error?
      return false;
    }
  }

  return m_engine.modes().numModes() == num_modes;
}

// print/parse the current mode json
std::string Vortex::printModeJson(bool pretty)
{
  std::string jsonStr = "{}";
  Mode *cur = m_engine.modes().curMode();
  if (!cur) {
    return jsonStr;
  }
  json modeJs = modeToJson(cur);
  return pretty ? modeJs.dump(4) : modeJs.dump();
}

bool Vortex::parseModeJson(const std::string &jsonStr)
{
  json jsonObj;
  try {
    jsonObj = json::parse(jsonStr);
  } catch (json::parse_error &e) {
    std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    return false;
  }
  Mode *tempMode = modeFromJson(jsonObj);
  if (!tempMode) {
    return false;
  }
  // update the current mode with the parsed json mode
  bool result = m_engine.modes().updateCurMode(tempMode);
  delete tempMode;
  return result;
}

// print/parse a pattern of the current mode json
std::string Vortex::printPatternJson(LedPos pos, bool pretty)
{
  std::string jsonStr = "{}";
  Mode *cur = m_engine.modes().curMode();
  if (!cur) {
    return jsonStr;
  }
  Pattern *pat = cur->getPattern(pos);
  if (!pat) {
    return jsonStr;
  }
  json patternJs = patternToJson(pat);
  return pretty ? patternJs.dump(4) : patternJs.dump();
}

bool Vortex::parsePatternJson(LedPos pos, const std::string &jsonStr)
{
  Mode *cur = m_engine.modes().curMode();
  if (!cur) {
    return false;
  }
  json jsonObj;
  try {
    jsonObj = json::parse(jsonStr);
  } catch (json::parse_error &e) {
    std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    return false;
  }
  Pattern *tempPat = patternFromJson(jsonObj);
  if (!tempPat) {
    return false;
  }
  // update the current mode with the parsed json mode
  PatternID patID = tempPat->getPatternID();
  PatternArgs args;
  tempPat->getArgs(args);
  Colorset set = tempPat->getColorset();
  delete tempPat;
  return cur->setPattern(patID, pos, &args, &set);
}

// dump the json to output
std::string Vortex::printJson(bool pretty)
{
  json json = saveToJson();
  return pretty ? json.dump(4) : json.dump();
}

bool Vortex::parseJson(const std::string &jsonStr)
{
  try {
    json jsonObj = json::parse(jsonStr);
    return loadFromJson(jsonObj);
  } catch (json::parse_error &e) {
    std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    return false;
  }
}

bool Vortex::printJsonToFile(const std::string &filename, bool pretty)
{
  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }
  std::string jsonStr = printJson(pretty);
  if (!jsonStr.length()) {
    return false;
  }
  file << jsonStr;
  file.close();
  return true;
}

bool Vortex::parseJsonFromFile(const std::string &filename)
{
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }
  try {
    json jsonObj = json::parse(file);
    return loadFromJson(jsonObj);
  } catch (json::parse_error &e) {
    std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    return false;
  }
}

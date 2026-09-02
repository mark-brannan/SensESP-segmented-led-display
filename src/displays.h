#ifndef DISPLAY_H
#define DISPLAY_H

#include <AceSegment.h>
#include <AceSegmentWriter.h>
#include <AceTMI.h>

#include <cmath>
#include <cstdio>
#include <memory>

#include "sensesp.h"
#include "temperature.h"

namespace segmented_led_display {

typedef const uint8_t NumDigits_t;
typedef const uint8_t ClkPin_t;
typedef const uint8_t DioPin_t;
typedef const uint8_t StbPin_t;

const uint8_t NUM_DIGITS_4 = 4;
const uint8_t NUM_DIGITS_6 = 6;
const uint8_t NUM_DIGITS_8 = 8;

// delays based on recommendions of ace_segment author;
// should be 'good enough' for SensESP.
const uint8_t TM1637_BIT_DELAY = 100;
const uint8_t TM1638_DELAY_MICROS = 1;

const uint8_t DEFAULT_BRIGHTNESS = 2;

/**
 * High-level writer API over an AceSegment LedModule. The writers hold
 * references to each other and to the module, so a facade is neither
 * copyable nor movable; create one with the factory functions below and
 * keep the returned std::shared_ptr.
 */
class DisplayFacade {
 public:
  explicit DisplayFacade(ace_segment::LedModule& ledModule)
      : ledModule_(ledModule),
        patternWriter_(ledModule),
        numberWriter_(patternWriter_),
        clockWriter_(numberWriter_),
        temperatureWriter_(numberWriter_) {}

  DisplayFacade(const DisplayFacade&) = delete;
  DisplayFacade& operator=(const DisplayFacade&) = delete;
  virtual ~DisplayFacade() = default;

  /**
   * Write a 'decimal' (int) value using similar rules as printf.
   * The display size is implicitly used for the print width (right
   * justified), and a negative sign will be prepended for negative values.
   * For example, for a display of size 4: printf("%4d", value).
   */
  void writeSignedDecimal(int value) {
    clear();
    numberWriter_.writeSignedDecimal(value, size());
    flush();
  }

  /**
   * Write a value using the same format as the Print class.
   * @note: Undefined behavior for displays with no decimal segment.
   */
  void writeFloat(float value, uint8_t prec = 2) {
    clear();
    numberWriter_.writeFloat(value, prec);
    flush();
  }

  /** Write a temperature given in kelvin as whole degrees C. */
  void writeTempDegC(float degreesK) {
    clear();
    temperatureWriter_.writeTempDegC(roundToInt16(convertDegreesKtoC(degreesK)),
                                     size());
    flush();
  }

  /** Write a temperature given in kelvin as whole degrees F. */
  void writeTempDegF(float degreesK) {
    clear();
    temperatureWriter_.writeTempDegF(roundToInt16(convertDegreesKtoF(degreesK)),
                                     size());
    flush();
  }

  /** Write the HH:MM part of an ISO 8601 timestamp (YYYY-MM-DDTHH:MM:SSZ). */
  void writeHourMinute24(const String& iso8601) {
    int hour, minute, second;
    if (!parseIso8601Time(iso8601, hour, minute, second)) {
      return;
    }
    clear();
    clockWriter_.writeHourMinute24(hour, minute);
    flush();
  }

  /** Write the MM:SS part of an ISO 8601 timestamp (YYYY-MM-DDTHH:MM:SSZ). */
  void writeMinutesSeconds(const String& iso8601) {
    int hour, minute, second;
    if (!parseIso8601Time(iso8601, hour, minute, second)) {
      return;
    }
    clear();
    clockWriter_.writeHourMinute24(minute, second);
    flush();
  }

  virtual void begin() = 0;
  virtual void flush() = 0;
  void clear() { patternWriter_.clear(); }
  void setBrightness(uint8_t brightness) { ledModule_.setBrightness(brightness); }
  uint8_t size() const { return ledModule_.size(); }

 protected:
  static int16_t roundToInt16(float value) {
    return static_cast<int16_t>(std::lround(value));
  }

  static bool parseIso8601Time(const String& iso8601, int& hour, int& minute,
                               int& second) {
    int year, month, day;
    int matched = std::sscanf(iso8601.c_str(), "%d-%d-%dT%d:%d:%d", &year,
                              &month, &day, &hour, &minute, &second);
    if (matched != 6) {
      ESP_LOGW(__FILE__, "Could not parse ISO 8601 time '%s'",
               iso8601.c_str());
      return false;
    }
    return true;
  }

  ace_segment::LedModule& ledModule_;
  ace_segment::PatternWriter<ace_segment::LedModule> patternWriter_;
  ace_segment::NumberWriter<ace_segment::LedModule> numberWriter_;
  ace_segment::ClockWriter<ace_segment::LedModule> clockWriter_;
  ace_segment::TemperatureWriter<ace_segment::LedModule> temperatureWriter_;
};

/**
 * Owns the TMI interface and the LED module. Kept as a separate base class
 * so that it is fully constructed before DisplayFacade binds to the module.
 */
template <template <typename, uint8_t> class T_TM163X_MODULE, typename T_TMII,
          uint8_t T_DIGITS>
struct Tm163xHardware {
  explicit Tm163xHardware(const T_TMII& tmiInterface)
      : interface(tmiInterface), ledModule(interface) {}

  T_TMII interface;
  T_TM163X_MODULE<T_TMII, T_DIGITS> ledModule;
};

template <template <typename, uint8_t> class T_TM163X_MODULE, typename T_TMII,
          uint8_t T_DIGITS>
class Tm163xFacade
    : private Tm163xHardware<T_TM163X_MODULE, T_TMII, T_DIGITS>,
      public DisplayFacade {
  using Hardware = Tm163xHardware<T_TM163X_MODULE, T_TMII, T_DIGITS>;

 public:
  explicit Tm163xFacade(const T_TMII& tmiInterface)
      : Hardware(tmiInterface), DisplayFacade(Hardware::ledModule) {
    begin();
    setBrightness(DEFAULT_BRIGHTNESS);
  }

  void begin() override {
    Hardware::interface.begin();
    Hardware::ledModule.begin();
  }

  void flush() override { Hardware::ledModule.flush(); }
};

// We are always using the 'Simple' interface for SensESP
// because the 'Fast' interfaces are specific to AVR processors
// and the performance wouldn't matter for ESP32 anyway.
using Tmi1637Interface = ace_tmi::SimpleTmi1637Interface;
using Tmi1638Interface = ace_tmi::SimpleTmi1638Interface;

template <uint8_t T_DIGITS>
using Tm1637Facade =
    Tm163xFacade<ace_segment::Tm1637Module, Tmi1637Interface, T_DIGITS>;
template <uint8_t T_DIGITS>
using Tm1638Facade =
    Tm163xFacade<ace_segment::Tm1638Module, Tmi1638Interface, T_DIGITS>;

template <uint8_t T_DIGITS>
inline std::shared_ptr<Tm1637Facade<T_DIGITS>> createTm1637Facade(
    DioPin_t dioPin, ClkPin_t clkPin) {
  return std::make_shared<Tm1637Facade<T_DIGITS>>(
      Tmi1637Interface(dioPin, clkPin, TM1637_BIT_DELAY));
}

template <uint8_t T_DIGITS>
inline std::shared_ptr<Tm1638Facade<T_DIGITS>> createTm1638Facade(
    DioPin_t dioPin, ClkPin_t clkPin, StbPin_t stbPin) {
  return std::make_shared<Tm1638Facade<T_DIGITS>>(
      Tmi1638Interface(dioPin, clkPin, stbPin, TM1638_DELAY_MICROS));
}

}  // namespace segmented_led_display

#endif /* DISPLAY_H */

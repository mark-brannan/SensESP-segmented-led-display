// Signal K application template file.
//
// This application demonstrates core SensESP concepts in a very
// concise manner. You can build and upload the application as is
// and observe the value changes on the serial port monitor.
//
// You can use this source file as a basis for your own projects.
// Remove the parts that are not relevant to you, and add your own code
// for external hardware libraries.

#include <memory>

#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp_app_builder.h"
#include <AceTMI.h> // SimpleTmi1637Interface
#include <AceSegment.h> // Tm1637Module
#include <AceSegmentWriter.h> // NumberWriter

using namespace sensesp;

const uint8_t NUM_DIGITS_4 = 4;
const uint8_t NUM_DIGITS_6 = 6;
const uint8_t NUM_DIGITS_8 = 8;

using Tmi1637Interface = ace_tmi::SimpleTmi1637Interface;
using Tmi1638Interface = ace_tmi::SimpleTmi1638Interface;

typedef typename ace_segment::Tm1637Module<Tmi1637Interface, NUM_DIGITS_4> Tm1637_4_digit;
typedef typename ace_segment::Tm1637Module<Tmi1637Interface, NUM_DIGITS_6> Tm1637_6_digit;
typedef typename ace_segment::Tm1638Module<Tmi1638Interface, NUM_DIGITS_8> Tm1638_8_digit;

const uint8_t TM1637_BIT_DELAY = 100;
const uint8_t TM1638_DELAY_MICROS = 1;
const uint8_t commonClk = 15;

// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);

  using ace_segment::LedModule;
  using ace_segment::PatternWriter;
  using ace_segment::NumberWriter;
  using ace_segment::TemperatureWriter;
  using ace_segment::ClockWriter;

  Tmi1637Interface interface1(16, commonClk, TM1637_BIT_DELAY);
  Tm1637_4_digit ledModule1(interface1);
  interface1.begin();
  ledModule1.begin();
  ledModule1.setBrightness(2);
  PatternWriter<LedModule> patternWriter1(ledModule1);
  NumberWriter<LedModule> numberWriter1(patternWriter1);
  numberWriter1.clear();
  ledModule1.flush();

  Tmi1637Interface interface2(17, commonClk, TM1637_BIT_DELAY);
  Tm1637_4_digit ledModule2(interface2);
  interface2.begin();
  ledModule2.begin();
  ledModule2.setBrightness(2);
  PatternWriter<LedModule> patternWriter2(ledModule2);
  NumberWriter<LedModule> numberWriter2(patternWriter2);
  TemperatureWriter<LedModule> temperatureWriter(numberWriter2);
  temperatureWriter.clear();
  ledModule2.flush();

  Tmi1637Interface interface3(18, commonClk, TM1637_BIT_DELAY);
  Tm1637_4_digit ledModule3(interface3);
  interface3.begin();
  ledModule3.begin();
  ledModule3.setBrightness(2);
  PatternWriter<LedModule> patternWriter3(ledModule3);
  NumberWriter<LedModule> numberWriter3(patternWriter3);
  ClockWriter<LedModule> clockWriterHoursMinutes(numberWriter3);
  clockWriterHoursMinutes.clear();
  ledModule3.flush();

  Tmi1637Interface interface4(19, commonClk, TM1637_BIT_DELAY);
  Tm1637_4_digit ledModule4(interface4);
  interface4.begin();
  ledModule4.begin();
  ledModule4.setBrightness(2);
  PatternWriter<LedModule> patternWriter4(ledModule4);
  NumberWriter<LedModule> numberWriter4(patternWriter4);
  ClockWriter<LedModule> clockWriterMinutesSeconds(numberWriter4);
  clockWriterMinutesSeconds.clear();
  ledModule4.flush();


  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
                    // Set a custom hostname for the app.
                    ->set_hostname("my-sensesp-project")
                    // Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    //->set_wifi_client("My WiFi SSID", "my_wifi_password")
                    //->set_wifi_access_point("My AP SSID", "my_ap_password")
                    //->set_sk_server("192.168.10.3", 80)
  ->get_app();

  auto* listener1 = new IntSKListener("environment.outside.relativeHumidity");

  listener1->connect_to(new LambdaConsumer<int>([&numberWriter1, &ledModule1](int data) {
    numberWriter1.clear();
    numberWriter1.writeSignedDecimal(data, 4);
    ledModule1.flush();
  }));

  auto* listener2 = new FloatSKListener("environment.outside.temperature");

  listener2->connect_to(new LambdaConsumer<float>([&temperatureWriter, &ledModule2](float data) {
    int degreesC = data - 273.15;
    int degreesF = (data - 273.15) * (9.0/5) + 32;
    temperatureWriter.clear();
    temperatureWriter.writeTempDegC(degreesC, 4);
    //temperatureWriter.writeTempDegF(degreesF, 4);
    ledModule2.flush();
  }));

  auto* timeListener = new StringSKListener("environment.time", 50);

  timeListener->connect_to(new LambdaConsumer<String>([
    &clockWriterHoursMinutes,
    &clockWriterMinutesSeconds,
    &ledModule3,
    &ledModule4
  ](String data) {

    int year, month, day, hour, minute;
    float second;
    std::sscanf(data.c_str(), "%d-%d-%dT%d:%d:%fZ", &year, &month, &day, &hour, &minute, &second);

    clockWriterHoursMinutes.clear();
    clockWriterHoursMinutes.writeHourMinute24(hour, minute);
    ledModule3.flush();
    // TODO: ask/PR for an upstream 'writeMinutesSeconds' method
    clockWriterMinutesSeconds.clear();
    clockWriterMinutesSeconds.writeHourMinute24(minute, second);
    ledModule4.flush();
  }));


  // To avoid garbage collecting all shared pointers created in setup(),
  // loop from here.
  while (true) {
    loop();
  }
}

void loop() {
  event_loop()->tick();
}


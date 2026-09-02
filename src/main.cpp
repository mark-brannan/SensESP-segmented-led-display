// Drive four 4-digit TM1637 LED displays from Signal K values:
// two showing the current time (HH:MM and MM:SS), one showing relative
// humidity and one showing outside temperature in degrees F.

#include <memory>

#include "displays.h"
#include "sensesp.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp_app_builder.h"

using namespace sensesp;
using namespace segmented_led_display;

// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);

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

  // works fine to reuse one pin for 'clk', for all displays
  const uint8_t commonClk = 15;
  auto clockDisplay1 = createTm1637Facade<NUM_DIGITS_4>(16, commonClk);
  auto clockDisplay2 = createTm1637Facade<NUM_DIGITS_4>(17, commonClk);
  auto humidityDisplay = createTm1637Facade<NUM_DIGITS_4>(18, commonClk);
  auto temperatureDisplay = createTm1637Facade<NUM_DIGITS_4>(19, commonClk);

  // The lambdas capture the display shared_ptrs by value, so the displays
  // stay alive as long as the consumers do.
  auto timeListener = std::make_shared<StringSKListener>("environment.time", 50);
  auto timeConsumer = std::make_shared<LambdaConsumer<String>>(
      [clockDisplay1, clockDisplay2](String data) {
        clockDisplay1->writeHourMinute24(data);
        clockDisplay2->writeMinutesSeconds(data);
      });
  timeListener->connect_to(timeConsumer);

  auto humidityListener = std::make_shared<IntSKListener>(
      "environment.outside.relativeHumidity");
  auto humidityConsumer = std::make_shared<LambdaConsumer<int>>(
      [humidityDisplay](int data) { humidityDisplay->writeSignedDecimal(data); });
  humidityListener->connect_to(humidityConsumer);

  auto temperatureListener = std::make_shared<FloatSKListener>(
      "environment.outside.temperature");
  auto temperatureConsumer = std::make_shared<LambdaConsumer<float>>(
      [temperatureDisplay](float degreesK) {
        temperatureDisplay->writeTempDegF(degreesK);
      });
  temperatureListener->connect_to(temperatureConsumer);

  // To avoid garbage collecting all shared pointers created in setup(),
  // loop from here.
  while (true) {
    loop();
  }
}

void loop() { event_loop()->tick(); }

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
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp_app_builder.h"
#include "displays.h"

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

  auto* timeListener = new StringSKListener("environment.time", 50);
  timeListener->connect_to(new LambdaConsumer<String>([&clockDisplay1, &clockDisplay2](String data) {
    clockDisplay1.writeHourMinute24(data);
    clockDisplay2.writeMinutesSeconds(data);
  }));

  auto* humidityListener = new IntSKListener("environment.outside.relativeHumidity");
  humidityListener->connect_to(new LambdaConsumer<int>([&humidityDisplay](int data) {
    humidityDisplay.writeSignedDecimal(data);
  }));

  auto* temperatureListener = new FloatSKListener("environment.outside.temperature");
  temperatureListener->connect_to(new LambdaConsumer<float>([&temperatureDisplay](float degreesK) {
    temperatureDisplay.writeTempDegF(degreesK);
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


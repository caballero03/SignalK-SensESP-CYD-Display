/**
 * @file main.cpp
 * @brief This is a custom CYD display for SK data on S/V Gone With The Wind
 *
 * Author: K. Youngblood <keithyoungblood@protonmail.com>
 * https://svgonewiththewind.com
 *
 */

#include <Arduino.h>

#include "sensesp/sensors/digital_output.h"
#include "sensesp/signalk/signalk_listener.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/sensors/constant_sensor.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/transforms/curveinterpolator.h"
#include "sensesp/transforms/typecast.h"
#include "sensesp/transforms/moving_average.h"
#include "sensesp/transforms/click_type.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp_app.h"
#include "sensesp_app_builder.h"

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "displays/CYDDisplay.h"
#include "transforms/kelvintofahrenheit.h"

using namespace sensesp;

#define TOUCH_CS

#define METERS_TO_FEET 3.281
#define METERS_PER_SEC_TO_KNOTS 1.9438444924

// LCD backlight control pin
#define LCD_BACK_LIGHT_PIN 21

// LDR input (light dependant resistor)
#define LDR_PIN 34

// RGB LED
#define CYD_LED_BLUE 17
#define CYD_LED_RED 4
#define CYD_LED_GREEN 16

// Touch Screen pins
// The CYD touch screen uses some non-standard SPI pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

class BacklightBrightnessInterpreter : public CurveInterpolator {
 public:
  BacklightBrightnessInterpreter(String config_path = "")
      : CurveInterpolator(NULL, config_path) {
    // Populate a lookup table to translate the (linear?) ADC readings  
    // returned from the LDR light sensor on the CYD and convert to a 
    // non-linear backlight brightness level.

    clear_samples();
    // addSample(CurveInterpolator::Sample(knownValue, knownResult));
    add_sample(CurveInterpolator::Sample(0, 4095.00));
    add_sample(CurveInterpolator::Sample(115, 2000.00));
    // add_sample(CurveInterpolator::Sample(230, 127.00));
    // add_sample(CurveInterpolator::Sample(345, 560.00));
    add_sample(CurveInterpolator::Sample(460, 10.0));
    add_sample(CurveInterpolator::Sample(1000, 6.0));
  }
};

TS_Point touch_screen_callback() { 
  if (ts.tirqTouched() && ts.touched()) {
    return ts.getPoint();
  }

  return TS_Point();
 }

 bool touch_button_callback() { 
  if (ts.tirqTouched() && ts.touched()) {
    // Return true if the touch pressure is greater than 900
    return (bool)(ts.getPoint().z > 900);
  }

  // Otherwise, return false
  return false;
 }

void setup() {
  // Some initialization boilerplate when in debug mode...
  SetupLogging(ESP_LOG_DEBUG);

  // Create a builder object
  SensESPAppBuilder builder;

  // Create the global SensESPApp() object.
  sensesp_app = builder.set_hostname("cyd-display")
                    ->set_sk_server("192.168.0.140", 3000)
                    ->set_wifi_client("Franklin T10 6907", "supersecretpassword")
                    ->get_app();
  
  // Use this to control the backlight and/or send displayMode changes to the display
  auto cydDisplayControl = new CYDDisplay();

  // Create the ConstantSensor object
  // auto* constant_sensor =
  //     new FloatConstantSensor(0.12, 10, "/Sensors/Fresh Water Tank Capacity");

  // ConfigItem(constant_sensor)
  //     ->set_title("Fresh Water Tank Capacity")
  //     ->set_description("Fresh water tank capacity in m3.")
  //     ->set_sort_order(1000);

  // // create and connect the output object

  // auto constant_sensor_output =
  //     new SKOutputFloat("tanks.freshWater.capacity", "",
  //                       new SKMetadata("m3", "Fresh Water Tank Capacity"));

  // ConfigItem(constant_sensor_output)
  //     ->set_title("Fresh Water Tank SK Path")
  //     ->set_description("Signal K path for the fresh water tank capacity.")
  //     ->set_sort_order(1100);

  // constant_sensor->connect_to(constant_sensor_output);

  //////////////////////////////////////////////
  // Handle touch screen touches
  auto* touch_input =
      new RepeatSensor<TS_Point>(100, touch_screen_callback);

  touch_input->connect_to(cydDisplayControl->touchPoint_);

  auto* touch_button =
      new RepeatSensor<bool>(100, touch_button_callback);

  // touch_button->connect_to(cydDisplayControl->touchButton_);

  const char* config_path_button_c = "/button/clicktime";
  auto click_type = new ClickType(config_path_button_c);

  ConfigItem(click_type)
      ->set_title("Click Type")
      ->set_sort_order(1000);

  touch_button
    ->connect_to(click_type)
    ->connect_to(cydDisplayControl->click_consumer_);
  

  /////////////////////////////////////////////
  // Depth below transducer
  const char* DBT_sk_path = "environment.depth.belowTransducer";
  
  auto* depthListener = new FloatSKListener(DBT_sk_path);

  depthListener
    ->connect_to(new CYDDisplay(&tft, 1, 3, "m", 2))
    // Convert meters to feet
    ->connect_to(new Linear(METERS_TO_FEET, 0.0))
    ->connect_to(new CYDDisplay(&tft, 5, 3, "ft", 2))
    // Adjust for transducer depth below surface (maybe)
    // ->connect_to(new Linear(1.0, 4.5))
    ->connect_to(new CYDDisplay(&tft, DisplayType::VBAR, 13, 10, 100, "ft", 2));

  /////////////////////////////////////////////
  // Speed through water
  auto* speedListener = new FloatSKListener("navigation.speedThroughWater");

  speedListener
    ->connect_to(new CYDDisplay(&tft, 15, 0, "m/s", 2))
    ->connect_to(new Linear(METERS_PER_SEC_TO_KNOTS, 0.0))
    ->connect_to(new CYDDisplay(&tft, 15, 8, "ks", 2));

  ////////////////////////////////////////////
  // Outdoor temperature
  // environment.outside.temperature

  auto* OutdoorTempListener = new FloatSKListener("environment.outside.temperature");

  OutdoorTempListener
    ->connect_to(new CYDDisplay(&tft, 0, 10, "K", 2))
    // Convert K to F
    ->connect_to(new KelvinToFahrenheit())
    ->connect_to(new CYDDisplay(&tft, 2, 10, "F", 2));
  
  ////////////////////////////////////////////
  // Battery voltage
  // electrical.batteries.houseBattery.voltage

  auto* batteryVoltageListener = new FloatSKListener("electrical.batteries.houseBattery.voltage");

  batteryVoltageListener->connect_to(new CYDDisplay(&tft, 8, 0, "V", 2));
  
  ////////////////////////////////////////////
  // Battery current
  // electrical.batteries.houseBattery.current

  auto* batteryCurrentListener = new FloatSKListener("electrical.batteries.houseBattery.current");

  batteryCurrentListener->connect_to(new CYDDisplay(&tft, 8, 4, "A", 2));

  ////////////////////////////////////////////
  // Automatic Backlight Brightness Control
  ////////////////////////////////////////////

  ///////////////////////////
  // LDR Light sensor
  auto* lightLevelAnalogInput = new AnalogInput(LDR_PIN, 1000, "/LDR/lightLevelIn/");

  ConfigItem(lightLevelAnalogInput)
    ->set_title("Light detecting resistor")
    ->set_description("LDR sensor in raw ADC value.")
    ->set_sort_order(1000);

  auto linearConfig_LDR = new Linear(1.0, -44.06000, "/ldr_ensor/brightness/linear");

  ConfigItem(linearConfig_LDR)
    ->set_title("LDR linear calibration")
    ->set_description("Light detecting resistor calibration.")
    ->set_sort_order(1100);

  lightLevelAnalogInput
    ->connect_to(linearConfig_LDR)
    // ->connect_to(new CYDDisplay(&tft, 8, 0, "", 2))
    ->connect_to(new BacklightBrightnessInterpreter("/backlight/brightness/curve"))
    // Smooth it out for gradual backlight fades and to avoid fast changes in screen brightness
    ->connect_to(new MovingAverage(10, 1.0))
    // ->connect_to(new Linear(0.0625, 0.0)) // divide by 16
    ->connect_to(new CYDDisplay(&tft, 12, 0, "<-- PWM", 2))
    // ->connect_to(new Linear(16.0, 0.0)) // multiply by 16
    ->connect_to(new RoundToInt())
    ->connect_to(cydDisplayControl->brightness_);

  /////////////////////////////////////////////////////////////////////////////
  // CYD Display Initial Setup
  /////////////////////////////////////////////////////////////////////////////

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();

  // Setting up the LEDC and configuring the Back light pin
  // NOTE: this needs to be done after tft.init()
#if ESP_IDF_VERSION_MAJOR == 5
  ledcAttach(LCD_BACK_LIGHT_PIN, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
#else
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LCD_BACK_LIGHT_PIN, LEDC_CHANNEL_0);
#endif

  tft.setRotation(1); //This is the display in landscape (USB to the right)
  
  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  // Dim the screen to 25%(ish)
  // ledcAnalogWrite(LEDC_CHANNEL_0, 64);
  // calculate duty, 4095 from 2 ^ 12 - 1
    uint32_t duty = (4095 / 255) * min((int)64, 255);

    // write duty to LEDC
    ledcWrite(LEDC_CHANNEL_0, duty);

  // Boot message
  tft.drawString("Please wait...", 5*(SCREEN_WIDTH/NUM_DISPLAY_COLS), 7*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS),4);

  // Testing out some TFT_eSPI functions
  // tft.fillCircle(50, 50, 10, TFT_RED);
  // tft.drawSpot(50, 100, 10, TFT_RED);
}

void loop() {
  event_loop()->tick();
}

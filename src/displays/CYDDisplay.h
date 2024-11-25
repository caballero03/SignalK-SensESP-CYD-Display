#ifndef CYD_DISPLAY_H_
#define CYD_DISPLAY_H_

#include <Arduino.h>

#include "sensesp/transforms/transform.h"
#include "sensesp/transforms/click_type.h"
#include <sensesp/system/valueconsumer.h>
#include <sensesp/system/valueproducer.h>
#include <sensesp/system/lambda_consumer.h>

#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

#include "gauge1.h"
#include "font.h"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT  12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// CYD display width and height, in pixels
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// CYD display rotation
#define DISPLAY_ROTATION 1

// TODO: FIX THIS DESCRIPTION Display data cells 128x64 pixels with font size 1 (6x8 chars) makes 4x8 data cell matrix
// Could be 16 rows of 10 char cells when rotated 90 degrees
#define NUM_DISPLAY_COLS 16
#define NUM_DISPLAY_ROWS 16

// Eight rows of four data cells
#define DATA_ROW_0 0
#define DATA_ROW_1 1
#define DATA_ROW_2 2
#define DATA_ROW_3 3
#define DATA_ROW_4 4
#define DATA_ROW_5 5
#define DATA_ROW_6 6
#define DATA_ROW_7 7
#define DATA_ROW_8 8
#define DATA_ROW_9 9
#define DATA_ROW_10 10
#define DATA_ROW_11 11
#define DATA_ROW_12 12
#define DATA_ROW_13 13
#define DATA_ROW_14 14
#define DATA_ROW_15 15

// Four data columns across the 128 pixels of the dsplay making 5-1/3 chars at font size 1 each
#define DATA_COL_0 0
#define DATA_COL_1 1
#define DATA_COL_2 2
#define DATA_COL_3 3

enum DisplayType {
  TEXT,
  HBAR,
  VBAR,
  BATT_VBAR,
  GAUGE
};

enum DisplayColorMode {
  NORMAL,
  NIGHT_VISION_RED,
  NIGHT_VISION_GREEN
};

struct DisplayCell {
    float value;
    int fontSize;
    String unit;
    bool hasData;
    DisplayType displayType;
    int length;
    float rangeMin;
    float rangeMax;
};

namespace sensesp {

class CYDDisplay  : public FloatConsumer, public FloatProducer
                     {
 public:
    CYDDisplay( TFT_eSPI* disp,
                    int8_t displayRow,
                    int8_t displayCol,
                    String unit = "",
                    uint8_t fontSize = 1);

    CYDDisplay( TFT_eSPI* disp,
                    DisplayType dispType,
                    int8_t displayRow,
                    int8_t displayCol,
                    int8_t len,
                    String unit = "",
                    uint8_t fontSize = 1);

    CYDDisplay( TFT_eSPI* disp,
                    DisplayType dispType,
                    int8_t displayRow,
                    int8_t displayCol,
                    int8_t len,
                    float rangeMin,
                    float rangeMax,
                    String unit = "");

    CYDDisplay();

    virtual void set(const float& new_value) override;

    // Set the brightness of the backlight LED of the display (0-4095)
    LambdaConsumer<int> brightness_{[this](int value) {
        this->setBacklight(value);
    }};

    // LambdaConsumer<SystemStatus> systemStatus_{[this](SystemStatus value) {
    //     this->updateSystemStatusLine(value);
    // }};

    // touch_input
    LambdaConsumer<TS_Point> touchPoint_{[this](TS_Point pos) {
        this->touched(pos);
    }};

    LambdaConsumer<bool> touchButton_{[this](bool state) {
        this->button(state);
    }};

    LambdaConsumer<ClickTypes> click_consumer_{[this](ClickTypes new_value) {
      // if (!ClickType::is_click(new_value)) {
      //   // Ignore button presses (we only want interpreted clicks)
      //   return;
      // }

      if (new_value == ClickTypes::UltraLongSingleClick) {
        // Long clicks reboot the system...
        ESP.restart();
        return;
      }

      // All other click types toggle the current state...
      // this->is_on_ = !this->is_on_;
      // this->emit(this->is_on_);

      // if (new_value == ClickTypes::DoubleClick) {
        // Sync any specified sync paths...
        // for (auto& path : sync_paths_) {
          // ESP_LOGD(__FILENAME__, "Sync status to %s", path.sk_sync_path_.c_str());
          // path.put_request_->set(this->is_on_);
        // }
      // }
    }};

    void clearDisplay();

 protected:
    void updateDisplay();
    void setBacklight(int backlightBrightness);
    uint16_t displayColor(uint16_t color);
    void touched(TS_Point pos);
    void button(bool state);

private:
    static TFT_eSPI* display;

    uint8_t cur_row;
    uint8_t cur_col;
    static DisplayCell row_col_data[NUM_DISPLAY_COLS][NUM_DISPLAY_ROWS];
    static bool firstRun;
    static bool firstUpdate;
    static DisplayColorMode displayColorMode;
};


} // End namespace sensesp
#endif // CYD_DISPLAY_H_

#include "sensesp_app.h"
#include "CYDDisplay.h"


//////////////////////////////////////////////////////////////
// Declare the static member variables storage 
// 

// Yeah, the rows and columns are flipped... Sue me.
DisplayCell sensesp::CYDDisplay::row_col_data[NUM_DISPLAY_COLS][NUM_DISPLAY_ROWS]; 

// A flag to make the display data init only happen once
bool sensesp::CYDDisplay::firstRun = false;

// There is only one display for all the data cells
TFT_eSPI* sensesp::CYDDisplay::display = NULL;

DisplayColorMode sensesp::CYDDisplay::displayColorMode = DisplayColorMode::NORMAL;

// A flag to clear the screen before initial update to be sure boot messages are gone, etc
bool sensesp::CYDDisplay::firstUpdate = false;

//////////////////////////////////////////////////////////////
// CONSTRUCTOR
//

sensesp::CYDDisplay::CYDDisplay(TFT_eSPI* disp, 
                                        int8_t displayRow, 
                                        int8_t displayCol,
                                        String unit,
                                        uint8_t fontSize) {

    // Check to see if the display class has ever been instantiated before this.
    // If not, start a periodic timer on the sensesp app to update the dislay.
    if( ! firstRun ) {
        // Do this stuff only once for all instances
        firstRun = true;
        display = disp;

        // Initialize the display cell array
        for(int col=0; col < NUM_DISPLAY_COLS; col++) {
            for(int row=0; row < NUM_DISPLAY_ROWS; row++) {
                this->row_col_data[col][row].value = 0.0;
                this->row_col_data[col][row].hasData = false;
                this->row_col_data[col][row].unit = "";
                this->row_col_data[col][row].fontSize = 1;
                this->row_col_data[col][row].displayType = DisplayType::TEXT;
                this->row_col_data[col][row].length = 0;
            }
        }

        SensESPBaseApp::get_event_loop()->onDelay(10000, [this]() { this->clearDisplay(); });

        // Start a periodic display update to show new data from all data cells
        SensESPBaseApp::get_event_loop()->onRepeat(750, [this]() { this->updateDisplay(); });
    }

    // Per this data cell info
    cur_row = displayRow;
    cur_col = displayCol;
    this->row_col_data[displayCol][displayRow].unit = unit;
    this->row_col_data[displayCol][displayRow].fontSize = fontSize;
}

sensesp::CYDDisplay::CYDDisplay( TFT_eSPI* disp,
                    DisplayType dispType,
                    int8_t displayRow,
                    int8_t displayCol,
                    int8_t len,
                    String unit,
                    uint8_t fontSize) {
    // Check to see if the display class has ever been instantiated before this.
    // If not, start a periodic timer on the sensesp app to update the dislay.
    if( ! firstRun ) {
        // Do this stuff only once for all instances
        firstRun = true;
        display = disp;

        // Initialize the display cell array
        for(int col=0; col < NUM_DISPLAY_COLS; col++) {
            for(int row=0; row < NUM_DISPLAY_ROWS; row++) {
                this->row_col_data[col][row].value = 0.0;
                this->row_col_data[col][row].hasData = false;
                this->row_col_data[col][row].unit = "";
                this->row_col_data[col][row].fontSize = 1;
                this->row_col_data[col][row].displayType =  DisplayType::TEXT;
                this->row_col_data[col][row].length = 0;
            }
        }

        SensESPBaseApp::get_event_loop()->onDelay(10000, [this]() { this->clearDisplay(); });

        // Start a periodic display update to show new data from all data cells
        SensESPBaseApp::get_event_loop()->onRepeat(750, [this]() { this->updateDisplay(); });
    }

    // Per this data cell info
    cur_row = displayRow;
    cur_col = displayCol;
    this->row_col_data[displayCol][displayRow].unit = unit;
    this->row_col_data[displayCol][displayRow].fontSize = fontSize;

    this->row_col_data[displayCol][displayRow].displayType = dispType;
    this->row_col_data[displayCol][displayRow].length = len;
}

sensesp::CYDDisplay::CYDDisplay( TFT_eSPI* disp,
                    DisplayType dispType,
                    int8_t displayRow,
                    int8_t displayCol,
                    int8_t len,
                    float rangeMin,
                    float rangeMax,
                    String unit) {
    // Check to see if the display class has ever been instantiated before this.
    // If not, start a periodic timer on the sensesp app to update the dislay.
    if( ! firstRun ) {
        // Do this stuff only once for all instances
        firstRun = true;
        display = disp;

        // Initialize the display cell array
        for(int col=0; col < NUM_DISPLAY_COLS; col++) {
            for(int row=0; row < NUM_DISPLAY_ROWS; row++) {
                this->row_col_data[col][row].value = 0.0;
                this->row_col_data[col][row].hasData = false;
                this->row_col_data[col][row].unit = "";
                this->row_col_data[col][row].fontSize = 1;
                this->row_col_data[col][row].displayType =  DisplayType::TEXT;
                this->row_col_data[col][row].length = 0;
            }
        }

        SensESPBaseApp::get_event_loop()->onDelay(10000, [this]() { this->clearDisplay(); });

        // Start a periodic display update to show new data from all data cells
        SensESPBaseApp::get_event_loop()->onRepeat(750, [this]() { this->updateDisplay(); });
    }

    // Per this data cell info
    cur_row = displayRow;
    cur_col = displayCol;
    this->row_col_data[displayCol][displayRow].unit = unit;
    this->row_col_data[displayCol][displayRow].fontSize = 1;

    this->row_col_data[displayCol][displayRow].displayType = dispType;
    this->row_col_data[displayCol][displayRow].length = len;

    this->row_col_data[displayCol][displayRow].rangeMin = rangeMin;
    this->row_col_data[displayCol][displayRow].rangeMax = rangeMax;
}

// sensesp::CYDDisplay::CYDDisplay(TFT_eSPI* disp, int8_t backlightBrightness) {
sensesp::CYDDisplay::CYDDisplay() {}

// This is called by a sensesp producer when this display instance is connected to it
void sensesp::CYDDisplay::set(const float& new_value) {
    // TODO: What do we need to do to display this data?
    //  -- Just change an array of the data and periodically update it?

    // Yep.
    row_col_data[cur_col][cur_row].value = new_value;

    // This is idempotent; it is assumed that when the cell has data once it needs updates always
    row_col_data[cur_col][cur_row].hasData = true;

    // Pass the value through to the next transform in the chain (if any)
    this->emit(new_value);
}

void sensesp::CYDDisplay::setBacklight(int backlightBrightness) {
    // calculate duty, 4095 from 2 ^ 12 - 1
    // uint32_t duty = (4095 / 255) * min(backlight_brigtness, 255);

    // TODO: check backlightBrightness is in range (0 to 4095), constrain

    if(backlightBrightness <= 1024) {
        displayColorMode = DisplayColorMode::NIGHT_VISION_RED;
    } else {
        displayColorMode = DisplayColorMode::NORMAL;
    }

    // write duty to LEDC
    ledcWrite(LEDC_CHANNEL_0, backlightBrightness);
}

void sensesp::CYDDisplay::touched(TS_Point pos) {
    // display->setTextColor(displayColor(TFT_GREEN), TFT_BLACK);
    display->setTextColor(TFT_GREEN, TFT_BLACK);
    
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->printf("%d, %d, %d           ",  pos.x, pos.y, pos.z);
}

void sensesp::CYDDisplay::button(bool state) {
    display->setTextColor(TFT_GREEN, TFT_BLACK);
    
    display->setCursor(0, 10);
    display->setTextSize(1);
    if(state) {
        display->printf("pressed: 1");
    } else {
        display->printf("pressed: 0");
    }
    
}

uint16_t sensesp::CYDDisplay::displayColor(uint16_t color) {
    if(displayColorMode == DisplayColorMode::NIGHT_VISION_RED) {
        return TFT_RED;
    }

    return color;
}

void sensesp::CYDDisplay::clearDisplay() {
    display->fillScreen(TFT_BLACK);

    // TODO: Then draw a bunch of lines and stuff to make the screen look good.
    // display->drawRect(0, 0, 319, 239, TFT_DARKGREY);
}

void sensesp::CYDDisplay::updateDisplay() {
    display->setRotation(DISPLAY_ROTATION);
    display->setTextSize(1); // Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
    display->setTextColor(TFT_WHITE, TFT_BLACK);

    // if( ! firstUpdate ) {
    //     // Do this stuff only once for all instances
    //     firstUpdate = true;

    //     // Clear the scren for future updates
    //     this->clearDisplay();
    // }
    
    // Run through the data cells and draw values into the corresponding display cells
    for(int col=0; col < NUM_DISPLAY_COLS; col++) {
        for(int row=0; row < NUM_DISPLAY_ROWS; row++) {
            // Only activate a data cell for writing only if it has data from the set() function
            if(this->row_col_data[col][row].hasData) {
                switch(this->row_col_data[col][row].displayType) {
                    case DisplayType::TEXT: {
                        display->setTextColor(displayColor(TFT_WHITE), TFT_BLACK);
                        
                        display->setCursor( col*(SCREEN_WIDTH/NUM_DISPLAY_COLS), 
                                        row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS));
                        display->setTextSize(this->row_col_data[col][row].fontSize);
                        display->printf("%0.2f%s  ",  this->row_col_data[col][row].value, 
                                                    this->row_col_data[col][row].unit);
                        
                        break;
                    }

                    case DisplayType::HBAR: {
                        break;
                    }

                    case DisplayType::VBAR: {
                        display->setTextColor(displayColor(TFT_BLUE), TFT_BLACK);

                        display->setCursor( col*(SCREEN_WIDTH/NUM_DISPLAY_COLS), 
                                        row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS));
                        // display->setTextSize(this->row_col_data[col][row].fontSize);
                        display->setTextSize(1); // Small font for VBAR
                        display->printf("%0.2f%s  ",  this->row_col_data[col][row].value, 
                                                    this->row_col_data[col][row].unit);
                        
                        // char buffer[20];
                        // sprintf(buffer, "%0.2f%s  ",  this->row_col_data[col][row].value, 
                        //                             this->row_col_data[col][row].unit);
                        // String buf = buffer;
                        // // display->drawCentreString(buf, col*(SCREEN_WIDTH/NUM_DISPLAY_COLS), row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS), 1);
                        // display->drawString(buf, col*(SCREEN_WIDTH/NUM_DISPLAY_COLS), row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS),4);

                        // Draw vertical bar
                        display->fillRect((col*(SCREEN_WIDTH/NUM_DISPLAY_COLS)), 
                                        (row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS)) - this->row_col_data[col][row].value - 2, 
                                        32, this->row_col_data[col][row].value, displayColor(TFT_BLUE));

                        // Blanking the rest above the bar
                        display->fillRect((col*(SCREEN_WIDTH/NUM_DISPLAY_COLS)), 
                                        (row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS)) - this->row_col_data[col][row].length - 2,
                                        32, this->row_col_data[col][row].length - this->row_col_data[col][row].value, TFT_BLACK);

                        // Draw a border around the VBAR
                        display->drawRect((col*(SCREEN_WIDTH/NUM_DISPLAY_COLS)), 
                                        (row*(SCREEN_HEIGHT/NUM_DISPLAY_ROWS)) - this->row_col_data[col][row].length - 2, 
                                        32, 100, displayColor(TFT_NAVY));

                        break;
                    }

                    case DisplayType::BATT_VBAR: {
                        // Here we will draw a little battery icon with a VBAR(MBAR?) Segmented? Discrete? (and text? on it? below?) TODO: What about tanks?

                        break;
                    }

                    case DisplayType::GAUGE: {
                        //Here we could draw a dial gauge showing the value visually. Math is hard LOL
                        break;
                    }
                }
            }
        }
    }

    // Apply the display updates
    // display->display();
}

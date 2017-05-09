#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#define RECEIVER_CHANGE_DELAY 10
#define MIN_RENDER_INTERVAL 100 // If nothing happens we render the screen at least this often. Couunt of updates
#define MIN_INPUT_DELAY 200 // To avoid repeated presses key presses aren't registered after MIN_BUTTON_PRESS_INTERVAL of the last press

// Input buttons
#define MODE_BUTTON_PIN 14
#define CHANNEL_BUTTON_PIN 15
// Video switch 1, switches between receiver 0 and 1
#define CONTROL_PIN_1_1 16
#define CONTROL_PIN_1_2 17
#define CONTROL_PIN_1_3 18
#define CONTROL_PIN_1_4 19
// Video switch 2, switches between receiver 2 and 3
#define CONTROL_PIN_2_1 20
#define CONTROL_PIN_2_2 21
#define CONTROL_PIN_2_3 22
#define CONTROL_PIN_2_4 23
// Video switch 3, switches between the inputs from video switches 1 and 2 
#define CONTROL_PIN_3_1 24
#define CONTROL_PIN_3_2 25
#define CONTROL_PIN_3_3 26
#define CONTROL_PIN_3_4 27

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#define SSD1306_LCDHEIGHT 64
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Adafruit_SSD1306 display(OLED_RESET);
bool manual_mode = false;
int selected_receiver = 0;
int init_values[] = {0.0, 0.0, 0.0, 0.0};
int values[] = {0.0, 0.0, 0.0, 0.0};
int skip_updates = 0; //To prevent constant switching. How many updates we are still going to skip.
int skip_input = 0;
int last_rendered = 0;

void setup()   {                
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);


  // Init input and output pins
  pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(CHANNEL_BUTTON_PIN, INPUT);

  pinMode(CONTROL_PIN_1_1, OUTPUT);
  pinMode(CONTROL_PIN_1_2, OUTPUT);
  pinMode(CONTROL_PIN_1_3, OUTPUT);
  pinMode(CONTROL_PIN_1_4, OUTPUT);
  
  pinMode(CONTROL_PIN_2_1, OUTPUT);
  pinMode(CONTROL_PIN_2_2, OUTPUT);
  pinMode(CONTROL_PIN_2_3, OUTPUT);
  pinMode(CONTROL_PIN_2_4, OUTPUT);

  pinMode(CONTROL_PIN_3_1, OUTPUT);
  pinMode(CONTROL_PIN_3_2, OUTPUT);
  pinMode(CONTROL_PIN_3_3, OUTPUT);
  pinMode(CONTROL_PIN_3_4, OUTPUT);
  
  // Find the maximum value (the worst signal value) of rssi pins. Remember to start the receiver without tx on!
  for (int i = 0 ; i < 4 ; i++) {
      init_values[i] = 1024 - analogRead(i);
  }
}

// Do the actual video switching to 
void switchReceiver() {
  //TODO
}

void render() {
  last_rendered = 0;
    // Draw box around the selected receiver
  display.drawRect(selected_receiver * display.width() / 4, display.height()/2 + 5, display.width()/4, display.height()/2 -5, WHITE);
  display.setTextSize(1);

  // Draw selected mode
  display.setCursor(0,0);
  if (manual_mode) {
    display.println("Manual");
  } else {
    display.println("Auto");
  }

  // Draw rssi's of receivers
  for (int i = 0 ; i < 4 ; i++) {
    display.setCursor(i * display.width() / 4 + 4,display.height()/2 + 7);
    
    display.println(String(values[i]) + "%");
  }
  
  display.display();
  display.clearDisplay();
}

void loop() {
  if (skip_input > 0) {
    skip_input--;
  } else {
      // Check if mode change button was pressed
      int buttonState = digitalRead(MODE_BUTTON_PIN);
      if (buttonState == HIGH) {
        // If mode change was pressed, switch mode and return
        manual_mode = !manual_mode;
        return; 
      }

      // If manual mode is on check if channel change button was pressed
      if (manual_mode) {
        buttonState = digitalRead(CHANNEL_BUTTON_PIN);
        if (buttonState == HIGH) {
          // If button was pressed update selected_receiver, change receiver, render screen and return
          skip_input = MIN_INPUT_DELAY;
          selected_receiver = selected_receiver + 1 % 4;
          switchReceiver();
          render;
          return
        }

      }
  }

  // Read new rssi values from analog pins 0-3
  for (int i = 0 ; i < 4 ; i++) {
      float scaled_val = ((float) analogRead(i)) / ((float) 1024 - init_values[i]);
      float inverted_val = 1.0 - scaled_val;
      values[i] = (int) 100.0 * inverted_val;
  }

  // If necessary to switch receiver. If it was just changed, skip.
  if (skip_updates > 0) {
    skip_updates--;
  } else if (!manual_mode) {
    int best_receiver_index = 0;

    // Find receiver with best signal
    for (int i = 1; i++ ; i < 4) {
      if (values[i] > values[best_receiver_index]) {
        best_receiver_index = i;
      }
    }
    // Switch if better than the current receiver was found
    if (best_receiver_index != selected_receiver) {
      skip_updates = RECEIVER_CHANGE_DELAY;
      selected_receiver = best_receiver_index;
      switchReceiver();
      render();
      return;
    }
    
  }
  if (last_rendered > MIN_RENDER_INTERVAL) {
    render();
  } else {
    last_rendered++;
  }
  
  
}


// Libraries
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <Wire.h>
#include <hd44780.h>                        // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header
#include <OneButton.h>
#include "RC_Payload.h"
#include "OmniRemote_Config.h"

#pragma region SERIAL FUNCTIONS

  // In your code, rather than using Serial.print() or Serial.println(), use their aliases defined below (e.g., SERIAL_PRINTLN()):

#define USE_SERIAL 0  // 1 or 0 to tell whether or not to print stuff to the serial monitor.
                      // Set to 1 when testing and back to 0 when done testing.

#if USE_SERIAL
  #define SERIAL_PRINT(x)     Serial.print(x)
  #define SERIAL_PRINTLN(x)   Serial.println(x)
  #define SERIAL_BEGIN(baud)  Serial.begin(baud)

  void printSerial() {
    Serial.print(F("lx: "));
    printWithSpaces(payload.lx, 4);
    Serial.print(F(", ly: "));
    printWithSpaces(payload.ly, 4);
    Serial.print(F(", rx: "));
    printWithSpaces(payload.rx, 4);
    Serial.print(F(", ry: "));
    printWithSpaces(payload.ry, 4);
    Serial.print(F(", lb1: "));
    printWithSpaces(payload.lb1, 1);
    Serial.print(F(", rb1: "));
    printWithSpaces(payload.rb1, 1);
    Serial.println();
  }

void printWithSpaces(int16_t val, int16_t spaces) {
  String str = String(val);
  for (int16_t i = str.length(); i < spaces; i++) {
    Serial.print(" ");
  }
  Serial.print(str);
}

#else
  #define SERIAL_PRINT(x)
  #define SERIAL_PRINTLN(x)
  #define SERIAL_BEGIN(baud)
  #define printSerial()
  #define printWithSpaces(val, spaces)
#endif

#pragma endregion SERIAL FUNCTIONS

#pragma region LedDisplayClass
#ifdef USELEDS

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
This is a class that contains all the functions and data required to handle the LED display bar.
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <FastLED.h>                        //Controls the RGB LEDs
const int MAX_BRIGHTNESS = 40;  //Brightness values are 8-bit for a max of 255 (the range is [0-255]), this sets default maximum to 40 out of 255.

class LedDisplay {
private:
  CRGB leds[NUM_LEDS];        //array that holds the state of each LED

public:
  //This is the constructor. It's called when a new instance of the class is created, and handles setting things up for use.
  LedDisplay() {              
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.clear();
  }

/**
 * @brief Animates an LED bouncing pattern during the homing process and flashes green when homing is complete.
 *
 * This function animates a bouncing light pattern on the LEDs to indicate that the gantry is in the process of homing. 
 * Once homing is complete, the LEDs flash green to signal completion. The function can block execution briefly during the 
 * flashing portion after homing is done.
 *
 * @param homingComplete A boolean flag indicating whether the homing process is complete. If set to false, the animation continues. 
 * If set to true, the LEDs flash green to indicate completion.
 *
 * The animation consists of a bouncing light pattern with a color that changes over time. When the gantry finishes homing, 
 * the LEDs flash green in a blocking manner for a brief period.
 */
  void homingSequence(bool homingComplete = false) {
    static unsigned long lastUpdate = 0;

    const byte fadeAmount = 150;
    const int ballWidth = 2;
    const int deltaHue  = 4;

    static byte hue = HUE_RED;
    static int direction = 1;
    static int position = 0;
    static int multiplier = 1;

    FastLED.setBrightness(MAX_BRIGHTNESS);

    if (!homingComplete) {                      //If the homing sequence is not complete, animate this pattern.
      if (millis() - lastUpdate >= 100) {
        hue += deltaHue;
        position += direction;

        if (position == (NUM_LEDS - ballWidth) || position == 0) direction *= -1;

        for (int i = 0; i < ballWidth; i++) {
          leds[position + i].setHue(hue);
        }

        // Randomly fade the LEDs
        for (int j = 0; j < NUM_LEDS; j++) {
          //if (random(10) > 3)
          leds[j] = leds[j].fadeToBlackBy(fadeAmount);  
        }
        FastLED.show();
        lastUpdate = millis();
      }
    } else {                                    //if the homing sequence is complete, indicate that by flashing the LEDs briefly.
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Green;
      }
      
      for (int j = 0; j < 8; j++) {
        FastLED.setBrightness(constrain(MAX_BRIGHTNESS * multiplier, 0, MAX_BRIGHTNESS));
        multiplier *= -1;
        FastLED.show();
        delay(100);
      }
    }
  }
};

LedDisplay display;  // Create an instance of the LedDisplay class that controls the RGB LEDs.

#endif
#pragma endregion LedDisplayClass

// Objects
OneButton leftButton1;
OneButton rightButton1;

hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip

RF24 radio(CE_PIN, CSN_PIN); // CE, CSN


void setup() {
  SERIAL_BEGIN(115200);
  lcd.begin(16, 2);
  lcd.print("Remote Control");
  
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe(address);
  radio.stopListening();
  #ifdef USELEDS
    FastLED.clear();            //clear the LEDs
    FastLED.show();
  #endif
  pinMode(LEFT_JOYSTICK_X_PIN, INPUT);
  pinMode(LEFT_JOYSTICK_Y_PIN, INPUT);
  pinMode(RIGHT_JOYSTICK_X_PIN, INPUT);
  pinMode(RIGHT_JOYSTICK_Y_PIN, INPUT);
  leftButton1.setup(LEFT_BUTTON_1_PIN, INPUT_PULLUP, true);
  leftButton1.attachPress(leftButton1Press);
  rightButton1.setup(RIGHT_BUTTON_1_PIN, INPUT_PULLUP, true);
  rightButton1.attachPress(rightButton1Press);
}

void loop() {
  leftButton1.tick();
  rightButton1.tick();
  #ifdef USELEDS
    display.homingSequence(false);
  #endif
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  const unsigned long interval = 50;
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readJoysticks();
    radio.write(&payload, sizeof(payload));
    printSerial();
    payload.lb1 = 0;
    payload.rb1 = 0;
  }
}

void readJoysticks() {
  payload.lx = analogRead(LEFT_JOYSTICK_X_PIN);
  payload.ly = analogRead(LEFT_JOYSTICK_Y_PIN);
  payload.rx = analogRead(RIGHT_JOYSTICK_X_PIN);
  payload.ry = analogRead(RIGHT_JOYSTICK_Y_PIN);
  if (LEFT_JOYSTICK_X_FLIPED) payload.lx = map(payload.lx, 0, 1023, 1023, 0);
  if (LEFT_JOYSTICK_Y_FLIPED) payload.ly = map(payload.ly, 0, 1023, 1023, 0);
  if (RIGHT_JOYSTICK_X_FLIPED) payload.rx = map(payload.rx, 0, 1023, 1023, 0);
  if (RIGHT_JOYSTICK_Y_FLIPED) payload.ry = map(payload.ry, 0, 1023, 1023, 0);
}

void leftButton1Press() {
  payload.lb1 = 1;
}

void rightButton1Press() {
  payload.rb1 = 1;
}


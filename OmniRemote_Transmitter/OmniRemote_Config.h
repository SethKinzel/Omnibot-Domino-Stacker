#ifndef SK_REMOTEPINDEFINITIONS_h
#define SK_REMOTEPINDEFINITIONS_h

#include <Arduino.h>


// Pin Definitions
constexpr int8_t LEFT_JOYSTICK_X_PIN = A3;
constexpr int8_t LEFT_JOYSTICK_Y_PIN = A2;
constexpr int8_t RIGHT_JOYSTICK_X_PIN = A1;
constexpr int8_t RIGHT_JOYSTICK_Y_PIN = A0;
constexpr int8_t LEFT_BUTTON_1_PIN = 7;
constexpr int8_t RIGHT_BUTTON_1_PIN = 8;
constexpr int8_t LED_DATA_PIN = 2;
constexpr int8_t CE_PIN = 10;
constexpr int8_t CSN_PIN = 9;
constexpr int8_t BUZZER_PIN = 3;
constexpr int8_t RANDOM_PIN = A6;

// Configuration
constexpr bool LEFT_JOYSTICK_X_FLIPED = true;
constexpr bool LEFT_JOYSTICK_Y_FLIPED = false;
constexpr bool RIGHT_JOYSTICK_X_FLIPED = false;
constexpr bool RIGHT_JOYSTICK_Y_FLIPED = true;

constexpr int NUM_LEDS = 8;          //Number of LEDs in the bar.
// #define USELEDS // uncomment this line if you have an led bar.

#endif
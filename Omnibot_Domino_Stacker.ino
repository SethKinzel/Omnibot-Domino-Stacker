
/*
A hack to the CrunchLabs Hack Pack Omnibot Forklift that allows it to stack dominoes.  
It uses an arduino and two joysticks as the remote control to allow for variable-speed movement and rotation  
*/

#include "config.h"
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <SK_RC_Payload.h>
#include <Servo.h>
#include <MapFloat.h>

#define SIN60 0.86602540378443864676372317075294

//>>>>>>>>>>>>>>>>>>>>>>>>>> KEY ROBOT VARIABLES <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// drive vector (in velocities) {v_x, v_y, omega}
struct VECTOR {
  float x, y, o;
};

VECTOR vSpeed;
// for forklift, -1 for lower, 0 for stay, 1 for move up
int moveFork = 0;

// stores each calculated wheel speed for a given commanded vector {w1, w2, w3}.
float wheelSpeeds[3];

// receiver
RF24 radio(CE_PIN, CSN_PIN); // CE, CSN
// servo
Servo servo1;

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> SETUP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void setup() {
  servo1.attach(SERVO_PIN);
  servo1.write(SERVO_RESET);

  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setPayloadSize(sizeof(payload));
  radio.openReadingPipe(0, address);
  radio.startListening();
}



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LOOP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void loop() {
  handleRadio();
  moveBot();
}



// >>>>>>>>>>>>>>>>>>>>> RADIO COMMAND PROCESSING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void handleRadio() {
  if (!radio.available()) return; // only run the rest of the function if there is new data
  radio.read(&payload, sizeof(payload));

  //set x, y, and omega vectors
  int32_t x = payload.lx - 512; // needs int32 for radius calculation
  int32_t y = payload.ly - 512;
  int32_t radius = sqrt(x*x + y*y); // pythagorean theorem
  if (abs(radius < joystickCenterThreshold)) {
    x = 0;
    y = 0;
  }
  int o = payload.rx - 512;
  if (abs(o) < joystickCenterThreshold) o = 0;

  const int xyScaling = 430;
  const int oScaling = 500;
  vSpeed.x = mapFloat(x, -xyScaling, xyScaling, 1, -1);
  vSpeed.y = mapFloat(y, -xyScaling, xyScaling, 1, -1);
  vSpeed.o = mapFloat(o, -oScaling, oScaling, -1, 1);

  if(payload.ry > 512 + joystickCenterThreshold) { //Fork Up
    moveFork = 1;
  } else if(payload.ry < 512 - joystickCenterThreshold) { //Fork Down
    moveFork = -1;
  } else {
    moveFork = 0;
  }

  if(payload.rb1) {
    servo1.write((servo1.read() == SERVO_RESET) ? SERVO_DROP : SERVO_RESET);
  }
}


// >>>>>>>>>>>>>>>>>>>>>>>> ROBOT ACTION FUNCTIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// prepare motors to coorinate bot movements
void moveBot(){
  getWheelSpeeds();
  driveWheels();
  driveLift();
}

// >>>>>>>>>>>>>>>>>>>>>>>>>> INVERSE KINEMATICS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// calculates speed of each wheel based of commanded speed vector. Forumlas are in the linked reading!
void getWheelSpeeds() {
  wheelSpeeds[2] = -0.5 * vSpeed.x + SIN60 * vSpeed.y + vSpeed.o;
  wheelSpeeds[0] = -0.5 * vSpeed.x - SIN60 * vSpeed.y + vSpeed.o;
  wheelSpeeds[1] =  1.0 * vSpeed.x + vSpeed.o;
}

// Addresses motor controllers to drive wheels at caclulated speeds
void driveWheels(){  
  // drive all motors at calculated speeds
  float topWheelSpeed = max(max(abs(wheelSpeeds[0]), abs(wheelSpeeds[1])), abs(wheelSpeeds[2]));
  int16_t intSpeeds[3];

  for(int8_t i = 0; i < 3; i++) {
    if (topWheelSpeed > 1) {
      wheelSpeeds[i] = wheelSpeeds[i] / topWheelSpeed;
    }
    wheelSpeeds[i] = mapFloat(wheelSpeeds[i], -1, 1, -255, 255);
    intSpeeds[i] = constrain(wheelSpeeds[i], -255, 255);
    if((flipM1 && i == 0) || (flipM2 && i == 1) || (flipM3 && i == 2)) {
      intSpeeds[i] = - intSpeeds[i];
    }
  }

  if(intSpeeds[0] < 0){
    digitalWrite(M1_DIR, LOW);
  } else {
    digitalWrite(M1_DIR, HIGH);
  }
  analogWrite(M1_PWM, abs(intSpeeds[0]));

  if(intSpeeds[1] < 0){
    digitalWrite(M2_DIR, LOW);
  } else {
    digitalWrite(M2_DIR, HIGH);
  }
  analogWrite(M2_PWM, abs(intSpeeds[1]));

  if(intSpeeds[2] < 0){
    digitalWrite(M3_DIR, LOW);
  } else {
    digitalWrite(M3_DIR, HIGH);
  }
  analogWrite(M3_PWM, abs(intSpeeds[2]));
}

// Moves lift motor. full forward, full back, or stopped.
void driveLift(){
  if(flipM4){
    if(moveFork > 0){
      digitalWrite(LIFT_DIR, LOW);
      analogWrite(LIFT_PWM, 255);
    } else if(moveFork < 0){
      digitalWrite(LIFT_DIR, HIGH);
      analogWrite(LIFT_PWM, 255);
    } else{
      digitalWrite(LIFT_DIR, HIGH);
      analogWrite(LIFT_PWM, 0);
    }
  } else {
    if(moveFork > 0){
      digitalWrite(LIFT_DIR, HIGH);
      analogWrite(LIFT_PWM, 255);
    } else if(moveFork < 0){
      digitalWrite(LIFT_DIR, LOW);
      analogWrite(LIFT_PWM, 255);
    } else{
      digitalWrite(LIFT_DIR, LOW);
      analogWrite(LIFT_PWM, 0);
    }
  }
}


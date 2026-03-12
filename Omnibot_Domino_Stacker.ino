
/*
Huge thanks to those who have derived the kinematic conrrols for omni-directional robots. This hackpack comes with an optional educational reading that I highly reccomend: (Siradjuddin, Indrazno. "Kinematics and control a three wheeled omnidirectional mobile robot." Int. J. Electr. Electron. Eng 6.12 (2019): 1-6.) [https://www.internationaljournalssrg.org/IJEEE/2019/Volume6-Issue12/IJEEE-V6I12P101.pdf] 

Amazingly the relationship of the wheel speeds of the robot to its overall speed vector is algebraically linear.  The formula also works for any relative wheel angle and can easily be expanded to include more wheels. This gives a mathematically deterministic way to get the robot from A to B defined by two translation variables and one rotation variable, a powerful tool. 

Combining this with a gyroscope allows for a true field oriented drive-- where  joystick commands always will move the robot forwards relative to you, despite which way the robot is facing. For those of you willing to take on the challenge, this is a great hack to try!
*/

#include "config.h"
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <SK_RC_Payload.h>
#include <Servo.h>

#define M1_DIR 7
#define M1_PWM 6
#define M2_DIR 2
#define M2_PWM 3
#define M3_DIR 4
#define M3_PWM 5

#define LIFT_DIR 8
#define LIFT_PWM 9

#define CE_PIN A0
#define CSN_PIN A1

#define SERVO_PIN A2

//>>>>>>>>>>>>>>>>>>>>>>>>>> KEY ROBOT VARIABLES <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// drive vector (in velocities) {v_x, v_y, omega}
int vSpeed[3];
// for forklift, -1 for lower, 0 for stay, 1 for move up
int moveFork = 0;
// How fast the fastest motor will go. other motors will be scaled down accordingly
int16_t fastestMotor = 255;

//Robot Physical Parameters -------------------------------------
double angleWheel1 = 90.0;
double angleWheel2 = 210.0;
double angleWheel3 = 330.0;

// Equation puts wheel 1 facing forwards. This rotates the robot's "front" ccw.
double localAngle = 60.0;
//double gyroAngle;
// Radius of wheel center to robot center
double botRadius = 100.0;
// Radius of omniwheel
double wheelRadius = 35.0;
// stores each calculated wheel speed for a given commanded vector {w1, w2, w3}.
double wheelSpeeds[3];
// tuning paratmeter to get linearized motor speed.
double tune = 1; 

// receiver
RF24 radio(CE_PIN, CSN_PIN); // CE, CSN
// servo
Servo servo1;

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> SETUP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void setup() {
  Serial.begin(115200);
  Serial.println("OMNIB");

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
    //set x and y vectors
  vSpeed[0] = map(payload.lx, 0, 1023, 511, -512);
  vSpeed[1] = map(payload.ly, 0, 1023, 511, -512);
    //set speed
  int32_t x = vSpeed[0];
  int32_t y = vSpeed[1];
  int32_t radius = sqrt(x*x + y*y); // pythagorean theorem
  if(radius < MIN_RADIUS) {
    fastestMotor = 0;
  }
  else {
    fastestMotor = map(radius, MIN_RADIUS, MAX_RADIUS, MIN_SPEED, MAX_SPEED);
    fastestMotor = constrain(fastestMotor, MIN_SPEED, MAX_SPEED);
  }

  if(payload.rx > 612) { //Rotate Right (CW)
    vSpeed[2] = 1;
  } else if(payload.rx < 412) { //Rotate Left (CCW)
    vSpeed[2] = -1;
  } else {
    vSpeed[2] = 0;
  }

  if(payload.ry > 612) { //Fork Up
    moveFork = 1;
  } else if(payload.ry < 412) { //Fork Down
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
  float wheelSpeeds[3];
  getWheelSpeeds(wheelSpeeds);
  driveWheels(wheelSpeeds, fastestMotor);
  driveLift();
}

// Addresses motor controllers to drive wheels at caclulated speeds
void driveWheels(float* speeds, float fastestMotor){  
  //drive all motors at calculated speeds
  float topWheelSpeed = max(max(abs(speeds[0]), abs(speeds[1])), abs(speeds[2]));

  wheelSpeeds[0] = speeds[0] / topWheelSpeed;
  wheelSpeeds[1] = speeds[1] / topWheelSpeed; 
  wheelSpeeds[2] = speeds[2] / topWheelSpeed;

  if(speeds[0] < 0){
    wheelSpeeds[0] = - pow(abs(wheelSpeeds[0]), tune) * fastestMotor;
  } else {
    wheelSpeeds[0] = pow(wheelSpeeds[0], tune) * fastestMotor;
  }
  if(speeds[1] < 0){
    wheelSpeeds[1] = - pow(abs(wheelSpeeds[1]), tune) * fastestMotor;
  } else {
    wheelSpeeds[1] = pow(wheelSpeeds[1], tune) * fastestMotor;
  }
  if(speeds[2] < 0){
    wheelSpeeds[2] = - pow(abs(wheelSpeeds[2]), tune) * fastestMotor;
  } else {
    wheelSpeeds[2] = pow(wheelSpeeds[2], tune) * fastestMotor;
  }

  if(flipM1){
     wheelSpeeds[0] = - wheelSpeeds[0];
  }
  if(flipM2){
     wheelSpeeds[1] = - wheelSpeeds[1];
  }
  if(flipM3){
     wheelSpeeds[2] = - wheelSpeeds[2];
  }
  

  if(wheelSpeeds[0] < 0){
    digitalWrite(M1_DIR, LOW);
  } else {
    digitalWrite(M1_DIR, HIGH);
  }
  analogWrite(M1_PWM, int(abs(wheelSpeeds[0])));

  if(wheelSpeeds[1] < 0){
    digitalWrite(M2_DIR, LOW);
  } else {
    digitalWrite(M2_DIR, HIGH);
  }
  analogWrite(M2_PWM, int(abs(wheelSpeeds[1])));

  if(wheelSpeeds[2] < 0){
    digitalWrite(M3_DIR, LOW);
  } else {
    digitalWrite(M3_DIR, HIGH);
  }
  analogWrite(M3_PWM, int(abs(wheelSpeeds[2])));
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



// >>>>>>>>>>>>>>>>>>>>>>>>>> INVERSE KINEMATICS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// calculates speed of each wheel based of commanded speed vector. Forumlas are in the linked reading!
void getWheelSpeeds(float* wheelList) {
  double botAngle = localAngle; //+ gyroAngle;

  // 0.0174533 converts deg to rad 
  wheelList[0] = (-sin((botAngle + angleWheel1) * 0.0174533) 
                * cos(botAngle * 0.0174533) * vSpeed[0] + cos((botAngle + angleWheel1) * 0.0174533) * cos(botAngle * 0.0174533) * vSpeed[1] + botRadius * vSpeed[2]) / wheelRadius;
  
  wheelList[1] = (-sin((botAngle + angleWheel2) * 0.0174533) 
                * cos(botAngle * 0.0174533) * vSpeed[0] + cos((botAngle + angleWheel2) * 0.0174533) * cos(botAngle * 0.0174533) * vSpeed[1] + botRadius * vSpeed[2])/ wheelRadius;
  wheelList[2] = (-sin((botAngle + angleWheel3) * 0.0174533) 
                * cos(botAngle * 0.0174533) * vSpeed[0] + cos((botAngle + angleWheel3) * 0.0174533) * cos(botAngle * 0.0174533) * vSpeed[1] + botRadius * vSpeed[2])/ wheelRadius;         
}
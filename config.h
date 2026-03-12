
const int MIN_SPEED = 128;  // the smallest value that fastestMotor can be set to
const int MAX_SPEED = 255;  // the largest value that fastestMotor can be set to
const int MIN_RADIUS = 100; // if the joystick is closer to the center than this, the robot stops
const int MAX_RADIUS = 245; // if the joystick is farther from the center than this, the robot goes at full speed

// motor flipping
bool flipM1 = false;
bool flipM2 = false;
bool flipM3 = false;
bool flipM4 = true;

// servo positions
const int SERVO_DROP = 95;
const int SERVO_RESET = 70;

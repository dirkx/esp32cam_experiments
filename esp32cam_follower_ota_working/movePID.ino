#include <PID_v2.h>

// Aggressive / fast
//    Kp = 4, Ki = 0.2, Kd = 1;
// Gentle / slow
//  Kp = 1, Ki = 0.05, Kd = 0.25;

// Default/standard baseline
double Kp = 2, Ki = 0.1, Kd = 0.5;

PID_v2 pidControllerPanAxis(Kp, Ki, Kd, PID::Direct);
PID_v2 pidControllerTiltAxis(Kp, Ki, Kd, PID::Direct);

void setup_pid() {
  pidControllerPanAxis.Start(0 /* current input */, pan_pos /* current servo position */, 0 /* desired input value */);
  pidControllerTiltAxis.Start(0 /* current input */, tilt_pos/* current servo position */, 0  /* desired input value */);
}

void move_pid(float cogX, float cogY)
{
  // run the PID controller and get the new position. We keep the P/I/D
  // simple by telling our current relative error; i.e how far we
  // are off CENTER_CAM_X. And expect the PID to try to get this
  // error as close to zero as possible.
  // note that we have a speed issue here - if both X and Y
  // move with the same speed - then the head moves a square root\
  // of two faster. 
  //
  float new_pan_pos = pidControllerPanAxis.Run(cogX - CENTER_CAM_X);
  float new_tilt_pos = pidControllerTiltAxis.Run(cogY - CENTER_CAM_Y);

  Serial.print("Moving to x ");
  Serial.print((int)(new_pan_pos));
  Serial.print(" and y ");
  Serial.println((int)(new_tilt_pos));

  servo_pan.write(new_pan_pos);
  servo_tilt.write(new_tilt_pos);
}

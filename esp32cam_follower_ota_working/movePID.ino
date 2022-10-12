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
  pidControllerPanAxis.Start(CENTER_CAM_X, NEUTRAL_POS_X, CENTER_CAM_X);
  pidControllerTiltAxis.Start(CENTER_CAM_Y, NEUTRAL_POS_Y, CENTER_CAM_Y);
}

void move_pid(float cogX, float cogY)
{
  float new_pan_pos = pidControllerPanAxis.Run(cogX);
  float new_tilt_pos = pidControllerTiltAxis.Run(cogY);

  Serial.print("Moving to x ");
  Serial.print((int)(new_pan_pos));
  Serial.print(" and y ");
  Serial.println((int)(new_tilt_pos));

  servo_pan.write(new_pan_pos);
  servo_tilt.write(new_tilt_pos);
}

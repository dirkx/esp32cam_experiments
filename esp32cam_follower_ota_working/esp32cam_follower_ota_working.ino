#include <Servo.h>
Servo servo_pan;
Servo servo_tilt;

#include <esp_camera.h>
#include <WiFi.h>
#include "esp_http_server.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// (Initial) servo position - in degrees
int pos_pan = 90;
int pos_tilt = 125;

// Neutral position - in degrees
const float NEUTRAL_POS_X = 90;
const float NEUTRAL_POS_Y = 125;

const float CENTER_CAM_X = 0.5;
const float CENTER_CAM_Y = 0.5;

// servos - how fast they travel; i.e. how
// long to wait (at least) between each update
// of one degree.
const  int wait = 15; // mSeconds *

// Change YOUR_AP_NAME and YOUR_AP_PASSWORD to your WiFi credentials
const char *ssid = "XXXXXXX";       // Put your SSID here
const char *password = "XXXXXXX"; // Put your PASSWORD here

// Postition of the server - remembered.
//
float m_attentionX = NEUTRAL_POS_X;
float m_attentionY = NEUTRAL_POS_Y;


void setup() {
  Serial.begin(115200);
  Serial.println("starting");
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  setup_servo();
  setup_camera();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected..!");
  delay(100);

  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());

  // Start streaming web server
  startCameraServer();
  ota_setup();
  setup_pid();
}

void camera_and_movement_loop() {
  camera_fb_t * current_frame = esp_camera_fb_get();
  if (!current_frame)  {
    Serial.println("Camera capture failed");
    return;
  }

  // diff() will also update previous_frame with some accomodation of
  // slowly changing light conditions.
  //
  camera_fb_t * difference = diff(current_frame);
  if (difference == NULL)
    return; // no difference yet - is the first image captured.

  float cogX, cogY; // center of gravity

  stream_next_image(current_frame);

  // Calculate the center of gravity.
  //
  int perc = calculate_cog(difference, &cogX, &cogY);

  if (perc < 2) {
    // low confidence in what we see - let the servos move slowy to the center.
    m_attentionX = m_attentionX + (((NEUTRAL_POS_X - m_attentionX)) / 20);
    m_attentionY = m_attentionY + (((NEUTRAL_POS_Y - m_attentionY)) / 20);

    pan((int)pos_pan, (int)(m_attentionX), wait);
    tilt((int)pos_tilt, (int)(m_attentionY), wait);
    return;
  }
  Serial.printf("%d percent changed pixels: ", perc);

  // Camera is mounted fixed; pan and tilt above it and
  // with roughly same coordinate system.
  //
  move_straight_away(cogX, cogY);

  // Camera is mounted on the pan and tilt. We move the camera
  // when we move pan and tilt.
  //
  // move_relative(cogX, cogY);

  // Use a PID controller.
  //
  // move_pid(cogX, cogY);
}


void loop() {
  ota_loop();
  camera_and_movement_loop();
}

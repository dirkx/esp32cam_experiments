#include <Servo.h>

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

// position of where the camera looks; 0 is left/bottom; 1 is top/right
const float CENTER_POS_X = 0.5;
const float CENTER_POS_Y = 0.5;

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

void processFrame(camera_fb_t * current_frame) {

  // diff() will also update previous_frame with some accomodation of
  // slowly changing light conditions.
  //
  camera_fb_t * difference = diff(current_frame);
  if (difference == NULL)
    return; // no difference yet - is the first image captured.
  float cogX, cogY; // center of gravity

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

  Serial.printf("COG is %.2f, %.2f\r\n", cogX, cogY);
  Serial.printf("Servo attention is %.2f, %.2f\r\n", m_attentionX, m_attentionY);

  // map from 0.0--1.0 normalized to servo degree space.
  // 
  m_attentionX = cogX * 180;
  m_attentionY = cogY * 180;
  
  // simple mode - move to the right place - we assume that
  // the camera is fixed.
  Serial.print("Moving to x ");
  Serial.print((int)(m_attentionX));
  Serial.print(" and y ");
  Serial.println((int)(m_attentionY));

  pan((int)pos_pan, (int)(m_attentionX), wait);
  tilt((int)pos_tilt, (int)(m_attentionY), wait);
}

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
  startCameraServer(&processFrame);
  ota_setup();
}


void loop() {
  ota_loop();
  // camera does not need to be in the loop - processFrame is called each time there is a new image.
}

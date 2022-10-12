#include <Servo.h>

#include <esp_camera.h>
#include <WiFi.h>
#include "esp_http_server.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

Servo servo_pan;
Servo servo_tilt;

//servo position
int pos_pan = 90;
int pos_tilt = 90;

// position in servo speak
const float NEUTRAL_POS_X = 0.5;
const float NEUTRAL_POS_Y = 0.7;

// position of where the camera looks; 0 is left/bottom; 1 is top/right
const float CENTER_POS_X = 0.5;
const float CENTER_POS_Y = 0.5;

// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
// you have to use a timer higher than 2, see https://www.esp32.com/viewtopic.php?t=11379
int servoPin_pan = 14;
int servoPin_tilt = 2;

// servos - how fast they travel; i.e. how
// long to wait (at least) between each update
// of one degree.
const  int wait = 15; // mSeconds *

// Change YOUR_AP_NAME and YOUR_AP_PASSWORD to your WiFi credentials
const char *ssid = "XXXXXXX";       // Put your SSID here
const char *password = "XXXXXXX"; // Put your PASSWORD here

// AI Thinker
// https://github.com/SeeedDocument/forum_doc/raw/master/reg/ESP32_CAM_V1.6.pdf

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_PIN           33 // Status led
#define LED_ON           LOW // - Pin is inverted.
#define LED_OFF         HIGH //
#define LAMP_PIN           4 // LED FloodLamp.

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,

  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,

  .ledc_timer = LEDC_TIMER_0,

  .ledc_channel = LEDC_CHANNEL_0,

  //https://forum.arduino.cc/t/esp32_cam-acces-and-process-image/677628/6
  //"you get 19200 bytes with is correct for 160x120 grayscale pixels on 8 bits"
  .pixel_format = PIXFORMAT_GRAYSCALE,//YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_QQVGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG
  .jpeg_quality = 10, //0-63 lower number means higher quality
  .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
  //.grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled [doesn't work]
};

esp_err_t camera_init() {
  //power up the camera if PWDN pin is defined
  if (PWDN_GPIO_NUM != -1) {
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, LOW);
  }

  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }

  Serial.println("Cameria initialised");
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  return ESP_OK;
}

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
    //0.5 and 0.7 are neutral positions of the servos, 0-1
    //20 is what fraction of the distance to move
    m_attentionX = m_attentionX + (((NEUTRAL_POS_X - m_attentionX)) / 20);
    m_attentionY = m_attentionY + (((NEUTRAL_POS_Y - m_attentionY)) / 20);

    pan((int)pos_pan, (int)(m_attentionX * 180), wait);
    tilt((int)pos_tilt, (int)(m_attentionY * 180), wait);
    return;
  }
  Serial.printf("%d percent changed pixels: ", perc);

  Serial.printf("COG is %.2f, %.2f\r\n", cogX, cogY);
  Serial.printf("Servo attention is %.2f, %.2f\r\n", m_attentionX, m_attentionY);

  // simple mode - move to the right place - we assume that
  // the camera is fixed.
  Serial.print("Moving to x ");
  Serial.print((int)(m_attentionX * 180));
  Serial.print(" and y ");
  Serial.println((int)(m_attentionY * 180));

  pan((int)pos_pan, (int)(m_attentionX * 180), wait);
  tilt((int)pos_tilt, (int)(m_attentionY * 180), wait);
}

void setup() {

  Serial.begin(115200);
  Serial.println("starting");
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  pinMode(servoPin_pan, OUTPUT);
  pinMode(servoPin_tilt, OUTPUT);

  //https://stackoverflow.com/questions/64402915/esp32-cam-with-servo-control-wont-work-arduino-ide
  //and
  //https://www.esp32.com/viewtopic.php?t=11379
  servo_pan.attach(servoPin_pan, 4);
  servo_tilt.attach(servoPin_tilt, 5);

  servo_pan.write(90);
  servo_tilt.write(90);

  Serial.begin(115200);

  camera_init();


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

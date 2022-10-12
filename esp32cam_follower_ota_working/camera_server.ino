#include <mutex>
std::mutex mutex;

//from https://www.electrorules.com/esp32-cam-video-streaming-web-server-works-with-home-assistant/
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;
bool _jpg_new = false;

void stream_next_image(camera_fb_t * current_frame) {
  size_t new_jpg_buf_len = 0;
  uint8_t * new_jpg_buf = NULL;

  if (!frame2jpg(current_frame, 80, &new_jpg_buf, &new_jpg_buf_len)) {
    Serial.println("JPEG compression failed");
    return;
  }

  {
    // we protect the next lines with this mutex - so the image cannot
    // be changed while a stream handler is still writing it out to
    // a user.
    std::lock_guard<std::mutex> lck(mutex);

    if (_jpg_buf) free(_jpg_buf);

    _jpg_buf = new_jpg_buf;
    _jpg_buf_len = new_jpg_buf_len;
    _jpg_new = true;
  }
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  while (res == ESP_OK) {

    // Wait for a new jpeg - yield()ing to other tasks; come back here when convenient.
    while (!_jpg_new) {
      yield();
    }

    // Protect the next lines with a mutex - to make sure that set_next_image()
    // cannot change out image under our feet.
    //
    {
      std::lock_guard<std::mutex> lck(mutex);
      char * part_buf[128];
      
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      if (ESP_OK != (res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen)))
        break;
      if (ESP_OK != (res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len)))
        break;
      _jpg_new = false;
    }
    if (ESP_OK != (res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY))))
      break;
  }
  return res;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}

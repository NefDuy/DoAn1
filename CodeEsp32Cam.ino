// ================= BLYNK SETUP ==================
#define BLYNK_TEMPLATE_ID "TMPL6Qgra3XPS"
#define BLYNK_TEMPLATE_NAME "face"
#define BLYNK_AUTH_TOKEN "9nONsgWbPX-k51zPFmHfatS8CBEqVy3R"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi thông tin
char ssid[] = "SSID";
char pass[] = "PASSWORD";

// ================= CAMERA & FACE RECOGNITION ==================
#include "esp_camera.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "fr_flash.h"

unsigned long currentMillis = 0;
unsigned long openedMillis = 0;
long interval = 5000;

bool faceDetectionEnabled = true;  // Cho phép nhận diện hay không

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

mtmn_config_t app_mtmn_config() {
  mtmn_config_t mtmn_config = {0};
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
  return mtmn_config;
}

mtmn_config_t mtmn_config = app_mtmn_config();
static face_id_list id_list = {0};
dl_matrix3du_t *image_matrix = NULL;
camera_fb_t *fb = NULL;
dl_matrix3du_t *aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);

// ============== SETUP ==============
void setup() {
  Serial.begin(9600); // UART dùng giao tiếp với PIC
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Cấu hình camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  read_face_id_from_flash(&id_list);
}

// ============== BLYNK BUTTON (V0) ==============
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1) {
    Serial.println("U"); // Blynk yêu cầu mở
    faceDetectionEnabled = false;
    openedMillis = millis();
  } else {
    Serial.println("L"); // Blynk yêu cầu đóng
    faceDetectionEnabled = true;
  }
}

// ============== MAIN LOOP ==============
void loop() {
  Blynk.run();

  // Nhận lệnh từ PIC qua UART
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'U') {
      Blynk.virtualWrite(V0, 1);       // Đồng bộ trạng thái
      faceDetectionEnabled = false;
      openedMillis = millis();
    } else if (cmd == 'L') {
      Blynk.virtualWrite(V0, 0);
      faceDetectionEnabled = true;
    }
  }

  // Nếu được phép, thực hiện nhận diện
  if (faceDetectionEnabled) {
    rzoCheckForFace();
  }

  // Tự gửi "L" sau 5 giây nếu trước đó đã gửi "U"
  currentMillis = millis();
  if (!faceDetectionEnabled && (currentMillis - openedMillis > interval)) {
    Serial.println("L");
    Blynk.virtualWrite(V0, 0);
    faceDetectionEnabled = true;
  }
}

// ============== FACE CHECK ==============
void rzoCheckForFace() {
  if (run_face_recognition()) {
    Serial.println("U");            // Gửi mở cửa cho PIC
    Blynk.virtualWrite(V0, 1);      // Cập nhật nút Blynk
    faceDetectionEnabled = false;   // Tạm dừng nhận diện
    openedMillis = millis();        // Bắt đầu tính giờ
  }
}

// ============== FACE RECOGNITION FUNCTION ==============
bool run_face_recognition() {
  bool faceRecognised = false;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  uint32_t res = fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  if (!res) {
    Serial.println("RGB conversion failed");
    dl_matrix3du_free(image_matrix);
    esp_camera_fb_return(fb);
    return false;
  }

  esp_camera_fb_return(fb);

  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);
  if (net_boxes) {
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK) {
      int matched_id = recognize_face(&id_list, aligned_face);
      if (matched_id >= 0) {
        faceRecognised = true;
      } else {
        Serial.println("No Match Found");
      }
    } else {
      Serial.println("Face Not Aligned");
    }

    free(net_boxes->box);
    free(net_boxes->landmark);
    free(net_boxes);
  }

  dl_matrix3du_free(image_matrix);
  return faceRecognised;
}

/******************************************************************************
 * MÃ GIÁM SÁT BÃI ĐỖ XE CHO WEMOS D1 R2 (ESP8266) - ĐÃ SỬA LỖI
 * Tác giả: Đối tác lập trình (AI)
 * Ngày: 30-06-2025
 * Chức năng:
 * - Đã sửa lỗi sai tên chân (pin) cho Wemos D1 R2.
 * - Đã sửa lỗi tương thích với thư viện FirebaseESP8266 v4.x+.
 ******************************************************************************/

// Thư viện cần thiết
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CẤU HÌNH CỦA BẠN ---

// 1. Thông tin Wi-Fi
#define WIFI_SSID "WifiName"
#define WIFI_PASSWORD "WifiPass"

// 2. Thông tin Firebase
#define FIREBASE_HOST "Realtime DB URL"
#define FIREBASE_AUTH "Firebase RDB secret"

// 3. Cấu hình chân (Pin) cho Wemos D1 R2 - ĐÃ ĐƯỢC SỬA LẠI CHO ĐÚNG
// Cảm biến hồng ngoại
#define SENSOR_1_PIN D5 // OK
#define SENSOR_2_PIN D6 // OK
#define SENSOR_3_PIN D7 // Sửa từ D10 -> D7 (bạn cần cắm lại dây)

// Màn hình OLED (SDA, SCL) - Chân I2C tiêu chuẩn của Wemos D1 R2
#define OLED_SDA_PIN D2 // Sửa từ D14 -> D2 (bạn cần cắm lại dây)
#define OLED_SCL_PIN D1 // Sửa từ D15 -> D1 (bạn cần cắm lại dây)

// 4. Cấu hình màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// --- KHỞI TẠO ĐỐI TƯỢNG TOÀN CỤC ---

FirebaseData fbdo;
// Thêm các đối tượng config và auth cho Firebase phiên bản mới
FirebaseConfig config;
FirebaseAuth auth;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Mảng để quản lý các chân cảm biến
const int sensorPins[3] = {SENSOR_1_PIN, SENSOR_2_PIN, SENSOR_3_PIN};

// --- HÀM KHỞI TẠO CHÍNH ---

void setup() {
  Serial.begin(115200);
  delay(100);

  // Khởi tạo các chân cảm biến
  for (int i = 0; i < 3; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Khởi tạo màn hình OLED với các chân I2C tiêu chuẩn
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Khởi tạo SSD1306 thất bại"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Dang khoi dong...");
  display.display();

  // Kết nối Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Dang ket noi Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nDa ket noi Wi-Fi!");

  // ** SỬA LỖI **: Cấu hình và kết nối Firebase theo cách mới
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Da ket noi Firebase.");
}

// --- VÒNG LẶP CHÍNH ---

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("PARKING");
  display.setTextSize(1);
  display.println("---------------------");

  // Duyệt qua từng cảm biến
  for (int i = 0; i < 3; i++) {
    int sensorValue = digitalRead(sensorPins[i]);
    bool isOccupied = (sensorValue == LOW);
    String statusText = isOccupied ? "Occupied" : "No one yet";
    
    String line = "Slot " + String(i + 1) + ": " + statusText;
    display.println(line);

    String firebasePath = "/PARKSLOT/" + String(i);

    if (Firebase.setBool(fbdo, firebasePath, isOccupied)) {
      Serial.println("Slot " + String(i + 1) + " updated to: " + (isOccupied ? "true" : "false"));
    } else {
      Serial.println("Failed to update Slot " + String(i + 1) + ". Reason: " + fbdo.errorReason());
    }
  }

  display.display();
  delay(2000);
}
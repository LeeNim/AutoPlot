/******************************************************************************
 * MÃ HOÀN CHỈNH - PHIÊN BẢN HOẠT ĐỘNG CUỐI CÙNG v2
 * Tác giả: Đối tác lập trình (AI)
 * Ngày: 30-06-2025
 * Chức năng:
 * - Đã sửa lỗi cuối cùng: Sử dụng đúng hàm jsonArray() để duyệt mảng
 * dữ liệu từ Firebase, giải quyết vấn đề "duyệt 0 bản ghi".
 ******************************************************************************/

// Thư viện cần thiết
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// --- CẤU HÌNH CỦA BẠN ---

// 1. Thông tin Wi-Fi
#define WIFI_SSID "WifiName"
#define WIFI_PASSWORD "WifiPass"

// 2. Thông tin Firebase
#define FIREBASE_HOST "Realtime DB URL"
#define FIREBASE_AUTH "Firebase RDB secret"

// 3. Cấu hình chân (Pin) cho Wemos D1 R32
#define SS_PIN    5
#define SCK_PIN   18
#define MOSI_PIN  23
#define MISO_PIN  19
#define RST_PIN   22
#define SERVO_PIN 25

// --- KHỞI TẠO ĐỐI TƯỢNG TOÀN CỤC ---

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo servo;

// --- HÀM KHỞI TẠO CHÍNH ---

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Khởi tạo các thiết bị ngoại vi
  servo.attach(SERVO_PIN);
  servo.write(0);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();
  Serial.println("Hệ thống RFID đã sẵn sàng.");

  // Kết nối mạng và dịch vụ
  connectWiFi();
  connectFirebase();
}

// --- VÒNG LẶP CHÍNH ---

void loop() {
  // Chỉ tập trung vào việc xử lý thẻ RFID
  handleRFIDCard();
}

// --- CÁC HÀM CHỨC NĂNG ---

/**
 * @brief Kết nối vào mạng Wi-Fi.
 */
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nĐã kết nối Wi-Fi!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Cấu hình và kết nối tới Firebase.
 */
void connectFirebase() {
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Đã kết nối Firebase.");
}

/**
 * @brief Lắng nghe và xử lý khi có thẻ RFID được quét.
 */
void handleRFIDCard() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Chuyển đổi UID dạng byte sang số nguyên
    unsigned long scannedUID = 0;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        scannedUID = (scannedUID << 8) | mfrc522.uid.uidByte[i];
    }
    
    Serial.println("\n--------------------------");
    Serial.println(">>> Phát hiện thẻ mới. Bắt đầu xử lý...");
    Serial.print("UID dạng số: ");
    Serial.println(scannedUID);

    // Gọi hàm xử lý dữ liệu với Firebase
    processCardData(scannedUID);

    // Chuẩn bị cho lần quét tiếp theo
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println(">>> Kết thúc xử lý. Đang chờ thẻ tiếp theo...");
  }
}


// ======================================================================================
// HÀM DƯỚI ĐÂY ĐÃ ĐƯỢC SỬA LẠI ĐỂ DÙNG ĐÚNG CÔNG CỤ CHO DỮ LIỆU DẠNG MẢNG (ARRAY)
// ======================================================================================

/**
 * @brief Lấy dữ liệu từ Firebase và xử lý logic vào/ra. (PHIÊN BẢN SỬA LỖI CUỐI CÙNG)
 * @param scannedUID UID của thẻ vừa được quét.
 */
void processCardData(unsigned long scannedUID) {
  if (Firebase.get(firebaseData, "/CARD")) {
    
    // Chỉ kiểm tra xem kiểu dữ liệu có phải là Array không
    if (firebaseData.dataTypeEnum() == fb_esp_rtdb_data_type_array) {
      
      // *** SỬA LỖI QUAN TRỌNG NHẤT: Dùng đúng đối tượng và hàm cho Array ***
      FirebaseJsonArray &arr = firebaseData.jsonArray(); // Dùng jsonArray() để lấy đối tượng mảng
      size_t len = arr.size(); // Dùng .size() để lấy độ dài của mảng
      // *****************************************************************

      bool cardFound = false;
      Serial.println("Bắt đầu duyệt " + String(len) + " bản ghi từ Firebase...");

      for (size_t i = 0; i < len; i++) {
        FirebaseJsonData result;
        arr.get(result, i); // Lấy phần tử từ đối tượng Array
        
        Serial.println("  - Đang kiểm tra bản ghi index " + String(i));

        if(result.typeNum == FirebaseJson::JSON_OBJECT) {
            FirebaseJson cardData;
            cardData.setJsonData(result.stringValue);
            FirebaseJsonData numberValue;
            cardData.get(numberValue, "number");

            if (numberValue.typeNum == FirebaseJson::JSON_STRING) {
              String firebaseUID_str = numberValue.stringValue;
              String scannedUID_str = String(scannedUID);
              firebaseUID_str.trim();

              Serial.println("    + So sánh chuỗi: Firebase='" + firebaseUID_str + "' <==> Thẻ Quét='" + scannedUID_str + "'");

              if (firebaseUID_str == scannedUID_str) {
                  Serial.println("    --> KẾT QUẢ: TRÙNG KHỚP!");
                  cardFound = true;
                  
                  FirebaseJsonData statusValue;
                  cardData.get(statusValue, "status");
                  bool currentStatus = (statusValue.typeNum == FirebaseJson::JSON_BOOL) ? statusValue.boolValue : false;
                  bool newStatus = !currentStatus;
                  String updatePath = "/CARD/" + String(i) + "/status";

                  Serial.print("    Trạng thái hiện tại: ");
                  Serial.print(currentStatus ? "Trong bãi (true)" : "Ngoài bãi (false)");
                  Serial.println(". Cập nhật thành: " + String(newStatus ? "Trong bãi (true)" : "Ngoài bãi (false)"));

                  if (Firebase.setBool(firebaseData, updatePath, newStatus)) {
                      Serial.println("    Cập nhật Firebase thành công. Mở cổng...");
                      servo.write(90); delay(4000); servo.write(0);
                      Serial.println("    Cổng đã đóng.");
                  } else {
                      Serial.println("    Lỗi! Không thể cập nhật trạng thái lên Firebase.");
                  }
                  break; 
              } else {
                  Serial.println("    --> KẾT QUẢ: KHÔNG TRÙNG.");
              }
            } else {
               Serial.println("    + Bỏ qua bản ghi này vì trường 'number' không phải là chuỗi (string).");
            }
        } else {
            Serial.println("    + Bỏ qua bản ghi này vì không phải là một đối tượng thẻ (JSON Object).");
        }
      }

      if (!cardFound) {
        Serial.println("Quét hết database nhưng không tìm thấy thẻ phù hợp.");
        Serial.println("=> TRUY CẬP BỊ TỪ CHỐI.");
      }

    } else {
      Serial.println("!!! LỖI CẤU TRÚC DỮ LIỆU !!! Kiểu dữ liệu không phải Array như mong đợi.");
      Serial.print("Kiểu dữ liệu thực tế: "); Serial.println(firebaseData.dataType());
    }
  } else {
    Serial.println("Lỗi khi đọc dữ liệu từ Firebase.");
    Serial.println("Lý do: " + firebaseData.errorReason());
  }
}
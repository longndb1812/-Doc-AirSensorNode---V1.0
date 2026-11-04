#include <Arduino.h>

// Định nghĩa chân kết nối
#define DERE_PIN 12
#define TX485_PIN 13
#define RX485_PIN 14

#define SLAVE_ID 0x02 // Đổi Slave ID tại đây nếu cần
#define BAUDRATE 9600 

HardwareSerial RS485(2); // Sử dụng UART2 của ESP32

// Hàm tính Modbus CRC16
uint16_t calculateCRC(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo Serial cho RS485
  RS485.begin(BAUDRATE, SERIAL_8N1, RX485_PIN, TX485_PIN);
  
  // Khởi tạo chân DERE
  pinMode(DERE_PIN, OUTPUT);
  digitalWrite(DERE_PIN, LOW); // Mặc định ở chế độ nhận (RX)
  
  Serial.println("Khởi động hệ thống đọc cảm biến RS485...");
}

void readSensor() {
  // 1. Chuẩn bị Frame dữ liệu (Read Input Registers - 0x04, Start: 0x0000, Length: 6)
  uint8_t request[8];
  request[0] = SLAVE_ID;
  request[1] = 0x04;
  request[2] = 0x00; // Start Address MSB
  request[3] = 0x00; // Start Address LSB
  request[4] = 0x00; // Quantity MSB
  request[5] = 0x06; // Quantity LSB
  
  // Tính CRC16 cho 6 byte đầu
  uint16_t crc = calculateCRC(request, 6);
  request[6] = crc & 0xFF;        // CRC LSB
  request[7] = (crc >> 8) & 0xFF; // CRC MSB

  // 2. Chuyển sang chế độ truyền (TX) và gửi lệnh
  digitalWrite(DERE_PIN, HIGH);
  RS485.write(request, sizeof(request));
  RS485.flush(); // Đợi truyền xong toàn bộ dữ liệu
  
  // 3. Chuyển về chế độ nhận (RX)
  digitalWrite(DERE_PIN, LOW);

  // 4. Đợi phản hồi (Timeout 500ms)
  uint32_t startTime = millis();
  while (RS485.available() < 17 && millis() - startTime < 500) {
    delay(1);
  }

  // 5. Đọc và xử lý dữ liệu
  if (RS485.available() >= 17) {
    uint8_t response[17];
    RS485.readBytes(response, 17);

    // Kiểm tra tính hợp lệ (Slave ID, Function code, Số byte dữ liệu)
    if (response[0] == SLAVE_ID && response[1] == 0x04 && response[2] == 0x0C) {
      // Xác minh CRC của chuỗi nhận được
      uint16_t receivedCRC = (response[16] << 8) | response[15];
      uint16_t calculatedCRC = calculateCRC(response, 15);
      
      if (receivedCRC == calculatedCRC) {
        // Trích xuất dữ liệu từ các thanh ghi (MSB trước, LSB sau)
        uint16_t lux          = (response[3] << 8) | response[4];
        int16_t  bmpTemp_raw  = (response[5] << 8) | response[6];
        uint16_t bmpPress_raw = (response[7] << 8) | response[8];
        uint16_t scdCO2       = (response[9] << 8) | response[10];
        int16_t  scdTemp_raw  = (response[11] << 8) | response[12];
        uint16_t scdHum_raw   = (response[13] << 8) | response[14];

        // In kết quả (Lưu ý: nhiệt độ/độ ẩm/áp suất thường cần chia cho 10 hoặc 100 theo datasheet)
        Serial.println("--- Dữ liệu Cảm biến ---");
        Serial.print("Chuỗi Hex nhận được: ");
        for (int i = 0; i < 17; i++) {
          Serial.printf("%02X ", response[i]);
        }
        Serial.println();

        Serial.printf("BH1750 Lux       : %d lx\n", lux / 10.0);
        Serial.printf("BMP280 Temp      : %.1f °C\n", bmpTemp_raw / 100.0); 
        Serial.printf("BMP280 Pressure  : %d hPa\n", bmpPress_raw / 10.0);
        Serial.printf("SCD4x CO2        : %d ppm\n", scdCO2);
        Serial.printf("SCD4x Temp       : %.1f °C\n", scdTemp_raw / 100.0); 
        Serial.printf("SCD4x Hum        : %.1f %%\n", scdHum_raw / 100.0);
      } else {
        Serial.println("Lỗi: Sai mã CRC phản hồi.");
      }
    } else {
      Serial.println("Lỗi: Sai định dạng gói tin hoặc Slave ID.");
    }
  } else {
    Serial.println("Lỗi: Timeout, không nhận đủ dữ liệu từ cảm biến.");
  }
  
  // Dọn dẹp buffer tránh rác dữ liệu ở chu kỳ sau
  while(RS485.available()) RS485.read(); 
}

void loop() {
  readSensor();
  delay(5000); // Chu kỳ đọc dữ liệu: 5 giây/lần
}
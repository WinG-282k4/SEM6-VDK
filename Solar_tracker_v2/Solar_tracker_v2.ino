#define BLYNK_TEMPLATE_ID "TMPL626Up_29-"
#define BLYNK_TEMPLATE_NAME "Solar tracker"
#define BLYNK_AUTH_TOKEN "_sekmk35_3gSPuLM2WmRGgyBVUvOKP-x"

#include <ESP32Servo.h>
#include <DHT.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "MY COFFEE";
// char pass[] = "44hg";
char pass[] = "44hotuong";


// ==== CONFIGURATIONS ==== //
#define DHTPIN 14
#define DHTTYPE DHT11

#define SERVO_LR 18
#define SERVO_UD 19

#define LDR_TOP 34
#define LDR_BOTTOM 35
#define LDR_LEFT 32
#define LDR_RIGHT 33

// ==== LED MATRIX CONFIG ==== //
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define DATA_PIN  23
#define CLK_PIN   21
#define CS_PIN    22

#define SOLAR_VOLT_PIN 36  // GPIO36 là chân ADC1_CH0

bool trackingEnabled = true;  // Mặc định là bật auto-tracking

// Thời gian gửi thông báo
unsigned long lastEventTime = 0;
const unsigned long eventCooldown = 30000; // 30 giây

DHT dht(DHTPIN, DHTTYPE);
MD_Parola display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

Servo servoLR;
Servo servoUD;

int posLR = 90;  // bắt đầu ở giữa khoảng 0-180
int posUD = 90;

int valTop, valBottom, valLeft, valRight;
const int threshold = 250;
const int stepSize = 1; // step độ để servo di chuyển mượt hơn

static unsigned long lastDHT = 0;
const unsigned long dhtInterval = 2000;

bool newDisplayMsg = false;
char currentMsg[128];

// Kiểm soát thời gian đọc LDR và cập nhật servo (ví dụ 50ms)
static unsigned long lastLDR = 0;
const unsigned long ldrInterval = 100;
static unsigned long lastsolarVol = 0;
static unsigned long log_LDR = 0;

void read_LDR_and_updateServo() {
  valTop = analogRead(LDR_TOP);
  valBottom = analogRead(LDR_BOTTOM);
  valLeft = analogRead(LDR_LEFT);
  valRight = analogRead(LDR_RIGHT);

  int vertDiff = valTop - valBottom;
  int horizDiff = valLeft - valRight;

  unsigned long now = millis();
  if (now - log_LDR > 2000) {
    log_LDR = now;
    Serial.print("Top: "); Serial.print(valTop);
    Serial.print(" | Bottom: "); Serial.print(valBottom);
    Serial.print(" | Left: "); Serial.print(valLeft);
    Serial.print(" | Right: "); Serial.print(valRight);

    Serial.print(" | vertDiff: "); Serial.print(vertDiff);
    Serial.print(" | horizDiff: "); Serial.print(horizDiff);

    Serial.print(" | posUD: "); Serial.print(posUD);
    Serial.print(" | posLR: "); Serial.println(posLR);
  }

  // Cập nhật posUD nếu khác biệt đủ lớn
  if (abs(vertDiff) > threshold) {
    if (vertDiff > 0) posUD += stepSize;
    else posUD -= stepSize;
    posUD = constrain(posUD, 20, 160);
    servoUD.write(posUD);
  }

  // Cập nhật posLR nếu khác biệt đủ lớn
  if (abs(horizDiff) > threshold + 150) {
    if (horizDiff > 0) posLR += stepSize;
    else posLR -= stepSize;
    posLR = constrain(posLR, 20, 160);
    servoLR.write(posLR);
  }
}

float readSolarVoltage() {
  int adcVal = analogRead(SOLAR_VOLT_PIN);
  float vMeasured = (adcVal / 4095.0) * 3.3;  // Điện áp tại điểm chia (V_out)

  // Mạch phân áp: R1 = 100Ω (trên), R2 = 30kΩ (dưới)
  // Vout = Vin * (R2 / (R1 + R2)) → Vin = Vout * ((R1 + R2) / R2)
  float vin = vMeasured * ((100.0 + 33000.0) / 33000.0);

  return vin;
}

BLYNK_WRITE(V8) {
  int value = param.asInt();  // 1: cố định, 0: auto tracking

  if (value == 1) {
    trackingEnabled = false;
    Serial.println("🔒 Tracking OFF - Servo giữ nguyên vị trí");
  } else {
    trackingEnabled = true;
    Serial.println("✅ Tracking ON - Servo bắt đầu dò sáng");
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Solar Tracker ===");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  // Gán trước giá trị góc khởi tạo
  posLR = 90;
  posUD = 0;

  // Khởi tạo tần số PWM và attach sau khi có góc đúng
  servoLR.setPeriodHertz(50);
  servoUD.setPeriodHertz(50);
  servoLR.attach(SERVO_LR, 500, 2400);
  servoUD.attach(SERVO_UD, 500, 2400);

  // Sau khi attach thì mới write
  servoLR.write(posLR);
  servoUD.write(posUD);

  analogReadResolution(12);  // ESP32 mặc định 12-bit (0 - 4095)

  display.begin();
  display.setIntensity(5);
  display.displayClear();
}


String lastWarningMsg = "";  // Lưu cảnh báo trước đó

void sendToBlynk(float temp, float humi, float solarVolt) {
    Blynk.virtualWrite(V0, temp);
    Blynk.virtualWrite(V1, humi);
    Blynk.virtualWrite(V6, solarVolt);

    Serial.printf("Sending to Blynk - Temp: %.1f, Humi: %.1f, SolarVolt: %.2f\n", temp, humi, solarVolt);

    String warningMsg;
    if (solarVolt > 8.0) {
        warningMsg = "⚠️ Cảnh báo: Công suất vượt ngưỡng!";
        if (lastWarningMsg != warningMsg) {
            Blynk.virtualWrite(V7, warningMsg);
            lastWarningMsg = warningMsg;
        }
        if (millis() - lastEventTime > eventCooldown) {
            Serial.println("Logging event: cng_sut_vt_ngng");
            Blynk.logEvent("cng_sut_vt_ngng", warningMsg);
            lastEventTime = millis();
        }
    } else if (solarVolt < 1.0) {
        warningMsg = "⚠️ Cảnh báo: Công suất quá thấp!";
        Serial.println("Condition met: solarVolt < 1.0");
        if (lastWarningMsg != warningMsg) {
            Blynk.virtualWrite(V7, warningMsg);
            lastWarningMsg = warningMsg;
        }
        if (millis() - lastEventTime > eventCooldown) {
            Serial.println("Logging event: cng_sut_qu_thp");
            Blynk.logEvent("cng_sut_qu_thp", warningMsg);
            lastEventTime = millis();
        } else {
            Serial.printf("Event cooldown active: %lu ms remaining\n", eventCooldown - (millis() - lastEventTime));
        }
    } else {
        warningMsg = "✅ An toàn: Công suất ổn định.";
        if (lastWarningMsg != warningMsg) {
            Blynk.virtualWrite(V7, warningMsg);
            lastWarningMsg = warningMsg;
        }
    }
}

void loop() {
  unsigned long now = millis();

  // Đọc và xử lý LDR với tần suất 50ms
  if (now - lastLDR >= ldrInterval) {
    lastLDR = now;
    if (trackingEnabled) {
      read_LDR_and_updateServo();
    }
  }

  if (now - lastsolarVol >= dhtInterval) {
    lastsolarVol = now;
    float solarV = readSolarVoltage();
    Serial.printf("Solar Panel Voltage: %.2f V\n", solarV);
  }

  static unsigned long lastLDRSend = 0;
  const unsigned long ldrSendInterval = 2000;

  if (now - lastLDRSend >= ldrSendInterval) {
    lastLDRSend = now;
    Blynk.virtualWrite(V2, valTop);
    Blynk.virtualWrite(V3, valBottom);
    Blynk.virtualWrite(V4, valLeft);
    Blynk.virtualWrite(V5, valRight);
  }

  // Đọc DHT11 với tần suất 2000ms
  if (now - lastDHT >= dhtInterval) {
    lastDHT = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float solarV = readSolarVoltage();

    if (!isnan(t) && !isnan(h)) {
      Serial.printf("Temp: %.1f°C | Humi: %.1f%% | Solar: %.2fV\n", t, h, solarV);
      snprintf(currentMsg, sizeof(currentMsg), "T:%.1fC H:%.1f%%", t, h);
      newDisplayMsg = true;

      sendToBlynk(t, h, solarV);  // Gửi dữ liệu lên Blynk
    } else {
      Serial.println("Lỗi đọc cảm biến DHT!");
    }
  }

  Blynk.run();  // Luôn gọi Blynk.run trong loop

  // Cập nhật hiển thị LED matrix nếu có tin nhắn mới
  if (newDisplayMsg) {
    display.displayZoneText(0, currentMsg, PA_LEFT, 20, 15000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    display.displayReset(0);
    newDisplayMsg = false;
  }

  display.displayAnimate();
}

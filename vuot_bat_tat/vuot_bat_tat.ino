const int LDR[4] = {2, 3}; // 4 cảm biến quang trở (phải dùng chân có ngắt ngoài)
const int LED[4] = {6, 7}; // 4 LED chỉ thị

volatile int lastTriggered = -1;  // LDR cuối cùng bị tác động
volatile unsigned long lastTime = 0; // Lưu thời điểm quét gần nhất

void setup() {
    Serial.begin(9600);

    // Cấu hình LED là OUTPUT
    for (int i = 0; i < 4; i++) {
        pinMode(LED[i], OUTPUT);
        digitalWrite(LED[i], LOW);
    }

    // Cấu hình LDR là INPUT và kích hoạt ngắt
    for (int i = 0; i < 4; i++) {
        pinMode(LDR[i], INPUT);
        // attachInterrupt(digitalPinToInterrupt(LDR[i]), handleSwipe, CHANGE);
        attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode)
    }
}

void handleSwipe() {
    unsigned long currentTime = millis();
    if (currentTime - lastTime < 300) return; // Tránh nhiễu ngắt (debounce)
    lastTime = currentTime;

    int triggeredLDR = -1;

    // Xác định cảm biến nào vừa bị tác động
    for (int i = 0; i < 4; i++) {
        if (digitalRead(LDR[i]) == LOW) { // Nếu có vật cản
            triggeredLDR = i;
            break;
        }
    }

    if (triggeredLDR == -1) return; // Không có LDR nào bị kích hoạt

    // Xác định hướng quét dựa trên LDR trước đó
    if (lastTriggered != -1) {
        if (triggeredLDR > lastTriggered) {
            Serial.println("Quét từ trái sang phải -> BẬT LED");
            turnOnLEDs();
        } 
        else if (triggeredLDR < lastTriggered) {
            Serial.println("Quét từ phải sang trái -> TẮT LED");
            turnOffLEDs();
        }
    }

    lastTriggered = triggeredLDR; // Cập nhật cảm biến cuối cùng bị tác động
}

void turnOnLEDs() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(LED[i], HIGH);
    }
}

void turnOffLEDs() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(LED[i], LOW);
    }
}

void loop() {
    // Chương trình chính không cần làm gì, tất cả xử lý qua ngắt
}

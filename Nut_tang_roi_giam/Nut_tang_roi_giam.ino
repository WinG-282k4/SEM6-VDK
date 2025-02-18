int led[] = {9, 10, 11, 12};
int pin = 6;  // Chân nút nhấn
int value;  // Biến đọc nút nhấn
int dem;    // Biến trạng thái LED

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(led[i], OUTPUT);
  }

  pinMode(pin, INPUT_PULLUP); // Sử dụng PULLUP để tránh nhiễu
}

void loop() {
  value = digitalRead(pin); // Đọc trạng thái nút nhấn

  while (value == LOW) { // Nếu nhấn nút
    dem = HIGH;
    
    for (int i = 0; i < 4; i++) {
      digitalWrite(led[i], dem);
      delay(1000);
    }

    dem = LOW;
    for (int i = 3; i >= 0; i--) {
      digitalWrite(led[i], dem);
      delay(1000);
    }

    value = digitalRead(pin); // Cập nhật lại trạng thái nút
  }
}


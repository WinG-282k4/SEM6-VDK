int led[] = {9, 10, 11}; // Chân điều khiển LED
int pin = 7;  // Chân nút nhấn

void setup() {
  // Đặt tất cả chân LED làm OUTPUT
  for (int i = 0; i < 3; i++) {
    pinMode(led[i], OUTPUT);
  }

  pinMode(pin, INPUT_PULLUP); // Kéo lên để tránh nhiễu
}

void loop() {
  int value = digitalRead(pin); // Đọc trạng thái nút nhấn

  while (value == LOW) { // Khi nhấn nút
    for (int i = 0; i < 3; i++) {
      digitalWrite(led[i], HIGH);
      delay(1000);
      digitalWrite(led[i], LOW);
    }
    value = digitalRead(pin);
  }
}

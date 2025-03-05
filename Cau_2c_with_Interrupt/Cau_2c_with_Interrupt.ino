int led[] = {9, 10, 11, 12};
int button[] = {6, 7};
int mode = 0;  // 0: Không chạy, 1: Xoay vòng, 2: Tăng giảm

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(led[i], OUTPUT);
  }

  for (int i = 0; i < 2; i++) {
    pinMode(button[i], INPUT_PULLUP);  // Dùng pull-up để tránh nhiễu
  }

  attachInterrupt(digitalPinToInterrupt(6), xoay_vong(), FALLING);
  attachInterrupt(digitalPinToInterrupt(7), tang_giam(), FALLING);
}

void xoay_vong() {
  for (int i = 0; i < 4; i++) {  
    digitalWrite(led[i], HIGH);
    delay(500);
    digitalWrite(led[i], LOW);
  }
}

void tang_giam() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(led[i], HIGH);
    delay(500);
  }

  for (int i = 3; i >= 0; i--) {
    digitalWrite(led[i], LOW);
    delay(500);
  }
}

void loop() {
  
}

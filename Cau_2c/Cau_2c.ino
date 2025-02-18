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
  int value1 = digitalRead(button[0]);  // Đọc trạng thái nút 6
  int value2 = digitalRead(button[1]);  // Đọc trạng thái nút 7

  if (value1 == LOW) {
    mode = 1;  // Chạy kiểu xoay vòng
  } 
  else if (value2 == LOW) {
    mode = 2;  // Chạy kiểu tăng giảm
  }

  // Chạy LED theo mode hiện tại
  if (mode == 1) {
    xoay_vong();
  } 
  else if (mode == 2) {
    tang_giam();
  }

  value1 = digitalRead(button[0]);  // Đọc trạng thái nút 6
  value2 = digitalRead(button[1]);  // Đọc trạng thái nút 7

  delay(50);
}

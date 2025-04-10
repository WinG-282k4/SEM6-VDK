int buzzer = 11;

void setup() {
  pinMode(buzzer, OUTPUT);
  playIphoneStyleMelody();
}

void loop() {
  delay(8000); // Lặp lại sau 8 giây
  playIphoneStyleMelody();
}

void playIphoneStyleMelody() {
  // Nhịp đều, tiếng nhẹ, tăng độ dài dần như đang "mở ra"
  beep(100, 200); // ting
  beep(120, 180); // ting
  beep(140, 160); // ting
  beep(160, 140); // ting
  beep(180, 120); // ting
  beep(200, 100); // ting
  beep(220, 100); // ting
  beep(300, 120); // tinggg
  beep(180, 200); // hạ nhẹ
  beep(150, 300); // tắt nhẹ
}

void beep(int duration, int pause) {
  digitalWrite(buzzer, HIGH);
  delay(duration);
  digitalWrite(buzzer, LOW);
  delay(pause);
}

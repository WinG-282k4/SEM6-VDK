#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  if (!rtc.begin()) {
    Serial.println("Không tìm thấy DS3231!");
    while (1);
  }

  // Chỉ cần chạy 1 lần để đặt thời gian hiện tại
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  Serial.println("DS3231 đã sẵn sàng!");
}

void loop() {
  DateTime now = rtc.now();

  Serial.print("Giờ hiện tại: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());

  delay(60000); // Đọc mỗi giây
}

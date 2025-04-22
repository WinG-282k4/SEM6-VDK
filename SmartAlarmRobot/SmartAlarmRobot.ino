#include <Wire.h>
#include <RTClib.h>
#include <NewPing.h>

// --- Cấu hình ---
// Cảm biến siêu âm (HC-SR04)
#define TRIGGER_PIN_1 7  // Siêu âm 1 (Trái) - Trig
#define ECHO_PIN_1    8  // Siêu âm 1 (Trái) - Echo
#define TRIGGER_PIN_2 4  // Siêu âm 2 (Giữa) - Trig
#define ECHO_PIN_2    5  // Siêu âm 2 (Giữa) - Echo
#define TRIGGER_PIN_3 2  // Siêu âm 3 (Phải) - Trig
#define ECHO_PIN_3    3  // Siêu âm 3 (Phải) - Echo
#define TRIGGER_PIN_4 A2 // Siêu âm 4 (Dưới đáy) - Trig
#define ECHO_PIN_4    A3 // Siêu âm 4 (Dưới đáy) - Echo
#define MAX_DISTANCE 200 // Khoảng cách tối đa cảm biến siêu âm (cm)

// Cảm biến chuyển động PIR
#define PIR_PIN 6        // Chân tín hiệu PIR

// Loa Active
#define SPEAKER_PIN 9    // Chân tín hiệu Loa

// Điều khiển động cơ L298N
#define ENA 10           // Chân PWM điều tốc độ động cơ A (Trái) - PHẢI LÀ CHÂN PWM
#define IN1 11           // Chân điều hướng động cơ A
#define IN2 12           // Chân điều hướng động cơ A
#define IN3 13           // Chân điều hướng động cơ B (Phải)
#define IN4 A0           // Chân điều hướng động cơ B
#define ENB A1           // Chân PWM điều tốc độ động cơ B (Phải) - //*** LƯU Ý: A1 trên Arduino Uno KHÔNG phải là chân PWM mặc định. Có thể cần dùng digitalWrite(ENB, HIGH) hoặc thư viện Software PWM nếu cần điều tốc *

// Ngưỡng khoảng cách
#define OBSTACLE_THRESHOLD 10 // Ngưỡng phát hiện vật cản phía trước (cm)
#define GROUND_THRESHOLD   20 // Ngưỡng phát hiện bị nhấc lên (cm)

// Tốc độ động cơ (0-255 cho PWM, hoặc HIGH/LOW nếu không dùng PWM)
#define MOTOR_SPEED 150      // Tốc độ di chuyển cơ bản
#define TURN_SPEED  120      // Tốc độ khi quay

// Thời gian quay khi né vật cản (milliseconds)
#define TURN_DURATION 350

// --- Khởi tạo đối tượng ---
RTC_DS3231 rtc; // Đối tượng module thời gian thực DS3231

// Khởi tạo các cảm biến siêu âm với NewPing (Tốt hơn cho nhiều cảm biến)
NewPing sonarLeft(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonarCenter(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonarRight(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);
NewPing sonarDown(TRIGGER_PIN_4, ECHO_PIN_4, MAX_DISTANCE);

// --- Biến trạng thái ---
bool isAlarmRinging = false; // Cờ báo trạng thái chuông đang kêu
bool pirDetected = false;    // Cờ báo đã phát hiện người phía sau
bool isMoving = false;       // Cờ báo robot đang di chuyển
bool isLifted = false;       // Cờ báo robot bị nhấc lên

// Mảng lưu trữ các thời điểm báo thức (Giờ, Phút)
// Thêm hoặc sửa các thời điểm báo thức ở đây
const int alarmTimes[][2] = {
  {16, 01},
  {16, 38},
  {16 , 35 }
  // Thêm các mốc thời gian khác nếu cần
};
const int numAlarms = sizeof(alarmTimes) / sizeof(alarmTimes[0]);

// Biến cho Bluetooth (chưa sử dụng trong logic chính, để dành mở rộng)
String bluetoothCommand = "";
bool commandComplete = false;

// --- Hàm cài đặt ban đầu ---
void setup() {
  Serial.begin(9600); // Bắt đầu giao tiếp Serial để debug và Bluetooth
  Serial.println("He thong Robot Bao thuc Khoi dong...");

  // Khởi tạo I2C cho DS3231
  if (!rtc.begin()) {
    Serial.println("Khong tim thay module DS3231!");
    while (1); // Dừng nếu không có module RTC
  }

  // Nếu RTC mất nguồn, đặt lại thời gian (Lấy thời gian lúc biên dịch code)
  // Bỏ comment dòng dưới nếu muốn set lại thời gian mỗi khi upload code
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   // Hoặc đặt thời gian cụ thể: rtc.adjust(DateTime(2024, 1, 1, 15, 59, 50)); // Năm, Tháng, Ngày, Giờ, Phút, Giây

  // Cấu hình chân I/O
  pinMode(PIR_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Dừng động cơ lúc khởi động
  stopMotors();

  Serial.println("Cai dat hoan tat.");
  printCurrentTime();
}

// --- Vòng lặp chính ---
void loop() {
  // 0. Kiểm tra xem robot có bị nhấc lên không (Ưu tiên cao nhất)
  checkGroundSensor();
  if (isLifted) {
    if (isMoving || isAlarmRinging) {
       Serial.println("!!! Robot da bi nhac len - DUNG LAI !!!");
       stopMotors();
       stopAlarmSound();
       isMoving = false;
       isAlarmRinging = false;
       pirDetected = false; // Reset trạng thái PIR
    }
    // Giữ trạng thái dừng khi đang bị nhấc
    delay(100); // Giảm tải CPU
    return; // Thoát khỏi vòng lặp nếu đang bị nhấc
  }

  // 1. Đọc thời gian hiện tại
  DateTime now = rtc.now();

  // 2. Kiểm tra báo thức (chỉ khi chưa di chuyển và chưa bị nhấc)
  if (!isMoving && !isLifted) {
    checkAlarms(now);
  }

  // 3. Xử lý khi chuông đang kêu
  if (isAlarmRinging && !isLifted) {
    // Phát âm báo (có thể làm phức tạp hơn nếu muốn)
    playAlarmSound(); // Gọi hàm này liên tục để duy trì âm thanh nếu cần

    // Kiểm tra cảm biến PIR
    if (checkPIR()) {
      Serial.println("Phat hien chuyen dong phia sau!");
      pirDetected = true;
      isAlarmRinging = false; // Tắt cờ báo thức
      stopAlarmSound();       // Tắt loa
      isMoving = true;        // Bắt đầu di chuyển
      Serial.println("Bat dau di chuyen va tranh vat can...");
    }
  }

  // 4. Xử lý khi robot đang di chuyển
  if (isMoving && !isLifted) {
    handleMovement();
  }

  // 5. Xử lý lệnh Bluetooth (Nếu có) - Phần mở rộng
  // handleBluetooth(); // Bỏ comment khi cần dùng Bluetooth

  // Delay nhỏ để tránh vòng lặp quá nhanh
  delay(50);
}

// --- Các hàm chức năng ---

// Kiểm tra và kích hoạt báo thức
void checkAlarms(DateTime now) {
  if (isAlarmRinging) return; // Nếu đang kêu rồi thì không kiểm tra nữa

  for (int i = 0; i < numAlarms; i++) {
    // So sánh Giờ và Phút
    if (now.hour() == alarmTimes[i][0] && now.minute() == alarmTimes[i][1] && now.second() == 0) { // Kích hoạt đúng giây 00
      Serial.print("!!! BAO THUC !!! Luc: ");
      printTime(now);
      Serial.println();
      isAlarmRinging = true;
      pirDetected = false; // Reset cờ PIR khi có báo thức mới
      break; // Thoát vòng lặp khi tìm thấy báo thức trùng khớp
    }
  }
}

// Phát âm báo đơn giản
void playAlarmSound() {
  // Nhạc chuông nhẹ nhàng gồm 3 nốt
  tone(SPEAKER_PIN, 800, 150);  // Nốt 1
  delay(180);
  tone(SPEAKER_PIN, 1000, 120); // Nốt 2
  delay(150);
  tone(SPEAKER_PIN, 900, 100);  // Nốt 3
  delay(120);

  noTone(SPEAKER_PIN); // Dừng tone để chắc chắn loa tắt
}

// Tắt âm báo
void stopAlarmSound() {
  // noTone(SPEAKER_PIN); // Nếu dùng tone()
  digitalWrite(SPEAKER_PIN, LOW); // Nếu dùng digitalWrite()
}

// Kiểm tra cảm biến PIR
bool checkPIR() {
  int pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH) {
    // Có thể thêm logic chống nhiễu hoặc chờ một khoảng thời gian xác nhận
    return true;
  } else {
    return false;
  }
}

// Đọc khoảng cách từ 3 cảm biến phía trước
void readFrontSensors(unsigned int &distL, unsigned int &distC, unsigned int &distR) {
  // Sử dụng ping_cm() của NewPing, trả về 0 nếu không đo được hoặc ngoài tầm MAX_DISTANCE
  distL = sonarLeft.ping_cm();
  delay(30); // Chờ một chút giữa các lần ping để tránh nhiễu
  distC = sonarCenter.ping_cm();
  delay(30);
  distR = sonarRight.ping_cm();

  // Xử lý trường hợp trả về 0 (ngoài tầm hoặc lỗi) -> coi như không có vật cản
  if (distL == 0) distL = MAX_DISTANCE;
  if (distC == 0) distC = MAX_DISTANCE;
  if (distR == 0) distR = MAX_DISTANCE;

  // In giá trị debug (có thể bỏ đi sau này)
  Serial.print("Dist: L="); Serial.print(distL);
  Serial.print(" C="); Serial.print(distC);
  Serial.print(" R="); Serial.println(distR);
}

// Kiểm tra cảm biến dưới đáy
void checkGroundSensor() {
  unsigned int distDown = sonarDown.ping_cm();
   //Serial.print("Dist Down: "); Serial.println(distDown); // Debug
  if (distDown == 0 || distDown > GROUND_THRESHOLD) { // Nếu không đo được HOẶC khoảng cách lớn
     if (!isLifted) { // Chỉ in thông báo khi trạng thái thay đổi
        Serial.println("Phat hien robot bi nhac len!");
     }
    isLifted = true;
  } else {
     if (isLifted) { // Chỉ in thông báo khi trạng thái thay đổi
        Serial.println("Robot da duoc dat xuong.");
     }
    isLifted = false;
  }
}

// Xử lý di chuyển và tránh vật cản
void handleMovement() {
  unsigned int distL, distC, distR;
  readFrontSensors(distL, distC, distR);

  // Ưu tiên kiểm tra vật cản ở giữa
  if (distC <= OBSTACLE_THRESHOLD) {
    Serial.println("Vat can o Giua!");
    stopMotors(); // Dừng tạm thời
    delay(100);
    // Quyết định hướng rẽ dựa trên cảm biến trái và phải
    if (distL > distR && distL > OBSTACLE_THRESHOLD) {
      Serial.println("-> Re Trai");
      turnLeft(TURN_DURATION);
    } else if (distR > distL && distR > OBSTACLE_THRESHOLD) {
      Serial.println("-> Re Phai");
      turnRight(TURN_DURATION);
    } else {
      // Cả hai bên đều vướng hoặc bằng nhau -> Rẽ phải (hoặc trái, tùy chọn)
      Serial.println("-> Ca hai ben deu vuong/gan -> Re Phai");
      turnRight(TURN_DURATION * 1.5); // Quay lâu hơn một chút
    }
  }
  // Kiểm tra vật cản bên trái (mà không có vật cản ở giữa)
  else if (distL <= OBSTACLE_THRESHOLD) {
    Serial.println("Vat can ben Trai -> Re Phai");
    stopMotors();
    delay(100);
    turnRight(TURN_DURATION);
  }
  // Kiểm tra vật cản bên phải (mà không có vật cản ở giữa hoặc trái)
  else if (distR <= OBSTACLE_THRESHOLD) {
    Serial.println("Vat can ben Phai -> Re Trai");
    stopMotors();
    delay(100);
    turnLeft(TURN_DURATION);
  }
  // Không có vật cản gần
  else {
    //Serial.println("Duong quang -> Di thang"); // Bỏ comment nếu muốn xem log này
    moveForward();
  }
}

// --- Các hàm điều khiển động cơ ---

void moveForward() {
  // Động cơ trái quay tới
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, MOTOR_SPEED); // Đặt tốc độ cho động cơ trái

  // Động cơ phải quay tới
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  // Do ENB nối với A1 (không phải PWM trên Uno), dùng digitalWrite
  // digitalWrite(ENB, HIGH); // Chạy tốc độ tối đa (hoặc dùng Software PWM nếu cần)
  // Nếu A1 có thể là PWM trên board khác: 
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {
  // Động cơ trái quay lùi
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, MOTOR_SPEED);

  // Động cơ phải quay lùi
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  // digitalWrite(ENB, HIGH);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnRight(int duration) {
   Serial.print("Quay Phai "); Serial.print(duration); Serial.println("ms");
  // Động cơ trái quay tới (tốc độ quay)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, TURN_SPEED);

  // Động cơ phải quay lùi (tốc độ quay)
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  digitalWrite(ENB, HIGH); // Hoặc analogWrite(ENB, TURN_SPEED); nếu A1 là PWM

  delay(duration); // Giữ trạng thái quay trong 'duration' ms
  stopMotors(); // Dừng lại sau khi quay xong
}

void turnLeft(int duration) {
   Serial.print("Quay Trai "); Serial.print(duration); Serial.println("ms");
  // Động cơ trái quay lùi (tốc độ quay)
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, TURN_SPEED);

  // Động cơ phải quay tới (tốc độ quay)
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, HIGH); // Hoặc analogWrite(ENB, TURN_SPEED); nếu A1 là PWM

  delay(duration); // Giữ trạng thái quay trong 'duration' ms
  stopMotors(); // Dừng lại sau khi quay xong
}

void stopMotors() {
  //Serial.println("Dung dong co"); // Bỏ comment nếu muốn xem log này
  // Dừng động cơ trái
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0); // Tắt PWM

  // Dừng động cơ phải
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, LOW); // Tắt ENB
}

// --- Hàm tiện ích ---

// In thời gian hiện tại ra Serial Monitor
void printCurrentTime() {
  DateTime now = rtc.now();
  Serial.print("Thoi gian hien tai: ");
  printTime(now);
  Serial.println();
}

// In đối tượng DateTime theo định dạng HH:MM:SS
void printTime(DateTime dt) {
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  if (dt.minute() < 10) Serial.print('0');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  if (dt.second() < 10) Serial.print('0');
  Serial.print(dt.second(), DEC);
}

// --- Phần mở rộng Bluetooth (Ví dụ cơ bản) ---
/*
void handleBluetooth() {
  while (Serial.available()) {
    char receivedChar = (char)Serial.read();
    if (receivedChar == '\n') { // Kết thúc lệnh khi nhận ký tự xuống dòng
      commandComplete = true;
    } else {
      bluetoothCommand += receivedChar;
    }
  }

  if (commandComplete) {
    Serial.print("Nhan lenh Bluetooth: ");
    Serial.println(bluetoothCommand);

    // Xử lý lệnh ở đây
    if (bluetoothCommand == "STOP") {
      Serial.println("Nhan lenh STOP tu Bluetooth");
      stopMotors();
      stopAlarmSound();
      isMoving = false;
      isAlarmRinging = false;
      pirDetected = false;
    } else if (bluetoothCommand == "STATUS") {
       Serial.println("--- Trang Thai Robot ---");
       printCurrentTime();
       Serial.print("Bao thuc dang keu: "); Serial.println(isAlarmRinging ? "Co" : "Khong");
       Serial.print("Da phat hien PIR: "); Serial.println(pirDetected ? "Co" : "Khong");
       Serial.print("Dang di chuyen: "); Serial.println(isMoving ? "Co" : "Khong");
       Serial.print("Dang bi nhac len: "); Serial.println(isLifted ? "Co" : "Khong");
       Serial.println("----------------------");
    }
    // Thêm các lệnh khác: FORWARD, BACKWARD, LEFT, RIGHT, SET_ALARM HH:MM, ...

    // Reset bộ đệm lệnh
    bluetoothCommand = "";
    commandComplete = false;
  }
}
*/
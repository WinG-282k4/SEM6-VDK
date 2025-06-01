#include <Wire.h>
#include <RTClib.h>
#include <NewPing.h>
#include <SoftwareSerial.h>

#define TRIGGER_PIN_1 7 
#define ECHO_PIN_1    8
#define TRIGGER_PIN_2 4
#define ECHO_PIN_2    5
#define TRIGGER_PIN_3 2
#define ECHO_PIN_3    3
#define MAX_DISTANCE 200

// PIR, loa, động cơ L298N
#define PIR_PIN       A0    // Chân A0 cho cảm biến PIR
#define SPEAKER_PIN   A1    // Chân A1 cho loa
#define ENA           10    // Chân ENA (PWM)
#define IN1           11
#define IN2           12
#define IN3           13
#define IN4           6     // Đổi chân IN4 thành chân 6
#define ENB           9     // Chân ENB (PWM)

#define OBSTACLE_THRESHOLD 10
#define MOTOR_SPEED        150
#define TURN_SPEED         120
#define TURN_DURATION      350

RTC_DS3231 rtc;
NewPing sonarLeft(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonarCenter(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonarRight(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

// trạng thái
bool isAlarmRinging = false;
bool isMoving       = false;

String bluetoothCommand = "";
int  alarmTimes[10][2];
int  numAlarms = 0;

// Bluetooth HC-06
SoftwareSerial BT(A3, A2);

// đọc ba cảm biến HC‑SR04: trái, giữa, phải
void readFrontSensors(unsigned int &distL, unsigned int &distC, unsigned int &distR) {
  distL = sonarLeft.ping_cm(); 
  distC = sonarCenter.ping_cm(); 
  distR = sonarRight.ping_cm();
  if (distL == 0) distL = MAX_DISTANCE;
  if (distC == 0) distC = MAX_DISTANCE;
  if (distR == 0) distR = MAX_DISTANCE;
}

void setup() {
  BT.begin(9600);
  BT.println("He thong Robot Bao thuc Khoi dong...");

  if (!rtc.begin()) {
    BT.println("Khong tim thay module DS3231!");
    while (1);
  }
  // rtc.adjust(DateTime(F(_DATE_), F(_TIME_)));

  pinMode(PIR_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  stopMotors();
  randomSeed(analogRead(A5));    // seed cho random turn khi cần
  BT.println("Cai dat hoan tat.");
  printCurrentTime();
}

void loop() {
  handleBluetooth();

  DateTime now = rtc.now();
  if (!isMoving && !isAlarmRinging) {
    checkAlarms(now);
  }

  if (isAlarmRinging) {
    playAlarmAndCheckPIR();
  }

  if (isMoving) {
    handleMovement();
  }

  delay(50);
}

// --- Xử lý Bluetooth ---
void handleBluetooth() {
  if (BT.available()) {
    bluetoothCommand = BT.readStringUntil('\n');
    bluetoothCommand.trim();
    BT.print("Nhận lệnh Bluetooth: ");
    BT.println(bluetoothCommand);

    if (bluetoothCommand == "STOP") {
      stopMotors();
      digitalWrite(SPEAKER_PIN, LOW);
      isMoving = isAlarmRinging = false;
      BT.println("DA_DUNG_ROBOT");
    }
    else if (bluetoothCommand == "STATUS") {
      BT.println("TRANG_THAI_ROBOT");
    }
    else if (bluetoothCommand.length() == 5 && bluetoothCommand[2] == ':') {
      int h = bluetoothCommand.substring(0,2).toInt();
      int m = bluetoothCommand.substring(3,5).toInt();
      if (numAlarms < 10 && h>=0 && h<24 && m>=0 && m<60) {
        alarmTimes[numAlarms][0] = h;
        alarmTimes[numAlarms][1] = m;
        numAlarms++;
        BT.print("ALARM_ADDED ");
        if (h<10) BT.print('0'); BT.print(h);
        BT.print(':');
        if (m<10) BT.print('0'); BT.println(m);
      } else {
        BT.println("INVALID_TIME");
      }
    } else {
      BT.println("UNKNOWN_COMMAND");
    }
  }
}

// --- Kiểm tra giờ báo thức ---
void checkAlarms(DateTime now) {
  for (int i=0; i<numAlarms; i++) {
    if (now.hour()==alarmTimes[i][0] && now.minute()==alarmTimes[i][1] && now.second()==0) {
      BT.print("!!! BAO THUC !!! Luc: ");
      printTime(now);
      BT.println();
      isAlarmRinging = true;
      break;
    }
  }
}

// --- Phát beep và kiểm tra PIR ---
void playAlarmAndCheckPIR() {
  const int tones[] = {800, 1000, 900};
  const int durs[]  = {150, 120, 100};
  for (int i = 0; i < 3 && isAlarmRinging; i++) {
    int f = tones[i], d = durs[i];
    unsigned long cycles     = (unsigned long)f * d / 1000;
    unsigned long halfPeriod = 1000000UL / f / 2;
    for (unsigned long c = 0; c < cycles && isAlarmRinging; c++) {
      digitalWrite(SPEAKER_PIN, HIGH);
      delayMicroseconds(halfPeriod);
      digitalWrite(SPEAKER_PIN, LOW);
      delayMicroseconds(halfPeriod);
      if (digitalRead(PIR_PIN) == HIGH) {
        BT.println("PIR detected → start moving");
        isAlarmRinging = false;
        isMoving = true;
        digitalWrite(SPEAKER_PIN, LOW);
        break;
      }
    }
    delay(30);
  }
}

// Xử lý di chuyển và tránh vật cản
void handleMovement() {
  unsigned int distL, distC, distR;
  readFrontSensors(distL, distC, distR);

  // Ưu tiên kiểm tra vật cản ở giữa
  if (distC <= OBSTACLE_THRESHOLD) {
    stopMotors(); // Dừng tạm thời
    delay(100);
    // Quyết định hướng rẽ dựa trên cảm biến trái và phải
    if (distL > distR && distL > OBSTACLE_THRESHOLD) {
      turnLeft(TURN_DURATION);
    } else if (distR > distL && distR > OBSTACLE_THRESHOLD) {
      turnRight(TURN_DURATION);
    } else {
      // Cả hai bên đều vướng hoặc bằng nhau -> Rẽ phải (hoặc trái, tùy chọn)
      turnRight(TURN_DURATION * 1.5); // Quay lâu hơn một chút
    }
  }
  // Kiểm tra vật cản bên trái (mà không có vật cản ở giữa)
  else if (distL <= OBSTACLE_THRESHOLD) {
    stopMotors();
    delay(100);
    turnRight(TURN_DURATION);
  }
  // Kiểm tra vật cản bên phải (mà không có vật cản ở giữa hoặc trái)
  else if (distR <= OBSTACLE_THRESHOLD) {
    stopMotors();
    delay(100);
    turnLeft(TURN_DURATION);
  }
  // Không có vật cản gần
  else {
    moveForward();
  }
}

// --- Điều khiển động cơ ---
void moveForward() {
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); analogWrite(ENA,MOTOR_SPEED);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
  analogWrite(ENA,0); analogWrite(ENB, 0);
}

void turnRight(int ms) {
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); analogWrite(ENA,TURN_SPEED);
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH); analogWrite(ENB, TURN_SPEED);
  delay(ms);
  moveForward();
}

void turnLeft(int ms) {
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH); analogWrite(ENA,TURN_SPEED);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);  analogWrite(ENB, TURN_SPEED);
  delay(ms);
  moveForward();
}

// --- In thời gian ---
void printCurrentTime() {
  DateTime t = rtc.now();
  BT.print("Thoi gian hien tai: ");
  printTime(t);
  BT.println();
}

void printTime(DateTime t) {
  if (t.hour()<10) BT.print('0'); BT.print(t.hour()); BT.print(':');
  if (t.minute()<10) BT.print('0'); BT.print(t.minute()); BT.print(':');
  if (t.second()<10) BT.print('0'); BT.print(t.second());
}

#include <Wire.h>
#include <RTClib.h>
#include <NewPing.h>
#include <SoftwareSerial.h>

#define TRIGGER_PIN_1 7
#define ECHO_PIN_1 8
#define TRIGGER_PIN_2 4
#define ECHO_PIN_2 5
#define TRIGGER_PIN_3 2
#define ECHO_PIN_3 3
#define MAX_DISTANCE 200

#define SPEAKER_PIN 15 // Chân A1
#define ENA 10
#define IN1 11
#define IN2 12
#define IN3 13
#define IN4 6
#define ENB 9

#define STOP_BUTTON_PIN A0 // Pin để đọc nút bấm dừng xe

#define OBSTACLE_THRESHOLD 10
#define TURN_SPEED 80
#define TURN_DURATION 800

RTC_DS3231 rtc;
NewPing sonarLeft(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonarCenter(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonarRight(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

bool isAlarmRinging = false;
bool isMoving = false;

String bluetoothCommand = "";
int alarmTimes[10][2];
int numAlarms = 0;
int alarmLevel = 1; // 0: LOW, 1: NORMAL, 2: HIGH

SoftwareSerial BT(A3, A2); // RX, TX

unsigned long lastBeepTime = 0;
bool isBeepOn = false;
unsigned long beepInterval = 400;
int MOTOR_SPEED = 60;

// Biến để theo dõi trạng thái nút bấm
bool lastButtonState = HIGH; // Giả sử nút bấm là pull-up và sẽ là LOW khi được nhấn
unsigned long lastButtonDebounceTime = 0;
const unsigned long buttonDebounceDelay = 50; // 50ms để debounce nút bấm

unsigned long lastDisplayTime = 0;
const unsigned long displayInterval = 1000; // Hiển thị mỗi 1 giây

// Function declarations
void readFrontSensors(unsigned int &L, unsigned int &C, unsigned int &R, bool printValues = false);
void displayAllSensorValues();
void checkStopButton();
void handleBluetooth();
void checkAlarms(DateTime now);
void handleBeep();
void handleMovement();
void moveForward();
void stopMotors();
void turnRight(int ms);
void turnLeft(int ms);
void printCurrentTime();
void printTime(DateTime t);

// ----------------- SETUP -----------------
void setup()
{
    BT.begin(9600);
    Serial.begin(9600);
    BT.println("He thong Robot khoi dong...");

    if (!rtc.begin())
    {
        BT.println("Khong tim thay DS3231");
        while (1)
            ;
    }
    // rtc.adjust(DateTime(F(_DATE_), F(_TIME_)));

    pinMode(SPEAKER_PIN, OUTPUT);
    digitalWrite(SPEAKER_PIN, LOW);

    pinMode(ENA, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(ENB, OUTPUT);

    // Thiết lập nút bấm với pull-up
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);

    stopMotors();
    randomSeed(analogRead(A5));
    BT.println("Cai dat hoan tat.");
    printCurrentTime();
}

// ----------------- LOOP -----------------
void loop()
{
    handleBluetooth();
    checkStopButton();

    // Hiển thị thông số tất cả cảm biến định kỳ
    if (millis() - lastDisplayTime >= displayInterval)
    {
        displayAllSensorValues();
        lastDisplayTime = millis();
    }

    DateTime now = rtc.now();
    if (!isMoving && !isAlarmRinging)
    {
        checkAlarms(now);
    }

    if (isAlarmRinging)
    {
        handleBeep();
    }

    if (isMoving)
        handleMovement();
}

// ----------------- Hiển thị tất cả giá trị cảm biến -----------------
void displayAllSensorValues()
{
    unsigned int L, C, R;
    readFrontSensors(L, C, R, false); // Đọc giá trị mà không in ra

    // Chuẩn bị chuỗi dữ liệu để gửi qua Bluetooth
    String sensorData = "--- THONG SO CAM BIEN ---\n";
    sensorData += "Thoi gian hien tai: ";
    DateTime t = rtc.now();
    if (t.hour() < 10) sensorData += '0';
    sensorData += t.hour();
    sensorData += ':';
    if (t.minute() < 10) sensorData += '0';
    sensorData += t.minute();
    sensorData += ':';
    if (t.second() < 10) sensorData += '0';
    sensorData += t.second();
    sensorData += "\n";

    sensorData += "Cam bien trai: ";
    sensorData += L;
    sensorData += " cm\n";
    sensorData += "Cam bien giua: ";
    sensorData += C;
    sensorData += " cm\n";
    sensorData += "Cam bien phai: ";
    sensorData += R;
    sensorData += " cm\n";

    // Trạng thái nút bấm dừng
    sensorData += "Nut bam dung: ";
    sensorData += (digitalRead(STOP_BUTTON_PIN) == LOW) ? "DANG NHAN\n" : "KHONG NHAN\n";

    // Hiển thị trạng thái hệ thống
    sensorData += "Trang thai: ";
    if (isAlarmRinging)
    {
        sensorData += "Dang bao thuc ";
    }
    if (isMoving)
    {
        sensorData += "DangКлиент di chuyen ";
    }
    if (!isAlarmRinging && !-isMoving)
    {
        sensorData += "Cho lenh ";
    }
    sensorData += "\n-------------------------\n";

    // Gửi dữ liệu qua Bluetooth
    BT.print(sensorData);

    // Hiển thị trên Serial Monitor để debug
    Serial.print(sensorData);
}

// ----------------- Kiểm tra nút bấm -----------------
void checkStopButton()
{
    // Đọc trạng thái nút bấm hiện tại (LOW khi được nhấn vì dùng INPUT_PULLUP)
    bool currentButtonState = digitalRead(STOP_BUTTON_PIN);

    // Kiểm tra thay đổi trạng thái với debounce
    if (currentButtonState != lastButtonState) {
        lastButtonDebounceTime = millis();
    }

    // Nếu trạng thái ổn định trong khoảng thời gian debounce
    if ((millis() - lastButtonDebounceTime) > buttonDebounceDelay) {
        // Nếu nút được nhấn (LOW vì INPUT_PULLUP)
        if (currentButtonState == LOW) {
            if (isAlarmRinging || isMoving) {
                stopMotors();
                isMoving = false;
                isAlarmRinging = false;
                digitalWrite(SPEAKER_PIN, LOW);
                isBeepOn = false;
                BT.println("DA_DUNG_ROBOT_VI_NUT_BAM");
                Serial.println("Da dung robot do nut bam");
            }
        }
    }

    lastButtonState = currentButtonState;
}

// ----------------- Bluetooth -----------------
void handleBluetooth()
{
    if (BT.available())
    {
        bluetoothCommand = BT.readStringUntil('\n');
        bluetoothCommand.trim();

        Serial.print("Bluetooth: ");
        Serial.println(bluetoothCommand);

        if (bluetoothCommand == "STOP")
        {
            stopMotors();
            isMoving = isAlarmRinging = false;
            BT.println("DA_DUNG_ROBOT");
        }
        else if (bluetoothCommand == "STATUS")
        {
            // Hiển thị tất cả thông số cảm biến ngay lập tức
            displayAllSensorValues();
            lastDisplayTime = millis(); // Reset thời gian để đảm bảo không gửi trùng
        }
        else if (bluetoothCommand.startsWith("DEL:"))
        {
            String timeStr = bluetoothCommand.substring(4);
            int h = timeStr.substring(0, 2).toInt();
            int m = timeStr.substring(3, 5).toInt();

            bool deleted = false;
            for (int i = 0; i < numAlarms; i++)
            {
                if (alarmTimes[i][0] == h && alarmTimes[i][1] == m)
                {
                    for (int j = i; j < numAlarms - 1; j++)
                    {
                        alarmTimes[j][0] = alarmTimes[j + 1][0];
                        alarmTimes[j][1] = alarmTimes[j + 1][1];
                    }
                    numAlarms--;
                    deleted = true;
                    break;
                }
            }

            if (deleted)
            {
                BT.print("ALARM_DELETED ");
                if (h < 10)
                    BT.print('0');
                BT.print(h);
                BT.print(':');
                if (m < 10)
                    BT.print('0');
                BT.println(m);
            }
            else
            {
                BT.println("ALARM_NOT_FOUND");
            }
        }
        else if (bluetoothCommand.length() == 5 && bluetoothCommand[2] == ':')
        {
            int h = bluetoothCommand.substring(0, 2).toInt();
            int m = bluetoothCommand.substring(3, 5).toInt();
            if (numAlarms < 10 && h >= 0 && h < 24 && m >= 0 && m < 60)
            {
                alarmTimes[numAlarms][0] = h;
                alarmTimes[numAlarms][1] = m;
                numAlarms++;
                BT.print("ALARM_ADDED ");
                if (h < 10)
                    BT.print('0');
                BT.print(h);
                BT.print(':');
                if (m < 10)
                    BT.print('0');
                BT.println(m);
            }
            else
            {
                BT.println("INVALID_TIME");
            }
        }
        else if (bluetoothCommand.startsWith("LEVEL:"))
        {
            String levelStr = bluetoothCommand.substring(6);
            if (levelStr == "LOW")
            {
                alarmLevel = 0;
                MOTOR_SPEED = 40;
                beepInterval = 600;
                BT.println("LEVEL_SET_LOW");
            }
            else if (levelStr == "NORMAL")
            {
                alarmLevel = 1;
                MOTOR_SPEED = 80;
                beepInterval = 400;
                BT.println("LEVEL_SET_NORMAL");
            }
            else if (levelStr == "HIGH")
            {
                alarmLevel = 2;
                MOTOR_SPEED = 120;
                beepInterval = 250;
                BT.println("LEVEL_SET_HIGH");
            }
            else
            {
                BT.println("INVALID_LEVEL");
            }
        }
        else
        {
            BT.println("UNKNOWN_COMMAND");
        }
    }
}

// ----------------- Báo thức -----------------
void checkAlarms(DateTime now)
{
    for (int i = 0; i < numAlarms; i++)
    {
        if (now.hour() == alarmTimes[i][0] && now.minute() == alarmTimes[i][1] && now.second() == 0)
        {
            BT.println("!!! BAO THUC !!!");
            isAlarmRinging = true;
            isMoving = true;
            break;
        }
    }
}

// ----------------- Beep không chặn -----------------
void handleBeep()
{
    if (millis() - lastBeepTime >= beepInterval)
    {
        lastBeepTime = millis();
        isBeepOn = !isBeepOn;
        digitalWrite(SPEAKER_PIN, isBeepOn ? HIGH : LOW);
    }
}

// ----------------- Di chuyển -----------------
void handleMovement()
{
    unsigned int L, C, R;
    readFrontSensors(L, C, R, false); // Không in giá trị cảm biến
    if (C <= OBSTACLE_THRESHOLD)
    {
        if (L > R && L > OBSTACLE_THRESHOLD)
            turnLeft(TURN_DURATION);
        else
            turnRight(TURN_DURATION);
    }
    else if (L <= OBSTACLE_THRESHOLD)
    {
        turnRight(TURN_DURATION);
    }
    else if (R <= OBSTACLE_THRESHOLD)
    {
        turnLeft(TURN_DURATION);
    }
    else
    {
        moveForward();
    }
}

// ----------------- Cảm biến siêu âm -----------------
void readFrontSensors(unsigned int &L, unsigned int &C, unsigned int &R, bool printValues)
{
    L = sonarLeft.ping_cm();
    C = sonarCenter.ping_cm();
    R = sonarRight.ping_cm();
    if (!L)
        L = MAX_DISTANCE;
    if (!C)
        C = MAX_DISTANCE;
    if (!R)
        R = MAX_DISTANCE;
}

// ----------------- Điều khiển động cơ -----------------
void moveForward()
{
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, MOTOR_SPEED);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors()
{
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

void turnRight(int ms)
{
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, TURN_SPEED);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, TURN_SPEED);
    delay(ms);
    moveForward();
    delay(200);
}

void turnLeft(int ms)
{
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, TURN_SPEED);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, TURN_SPEED);
    delay(ms);
    moveForward();
    delay(200);
}

// ----------------- In thời gian -----------------
void printCurrentTime()
{
    DateTime t = rtc.now();
    BT.print("Thoi gian hien tai: ");
    printTime(t);
    BT.println();
}

void printTime(DateTime t)
{
    if (t.hour() < 10)
        BT.print('0');
    BT.print(t.hour());
    BT.print(':');
    if (t.minute() < 10)
        BT.print('0');
    BT.print(t.minute());
    BT.print(':');
    if (t.second() < 10)
        BT.print('0');
    BT.print(t.second());
}
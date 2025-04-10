// Định nghĩa các chân và hằng số
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BUTTON_PIN 7
#define LED_PIN 11
#define MODE_MANUAL 1
#define MODE_AUTO 0
#define ANALOG_LDR_PIN A0 // Chân analog để đọc giá trị cho tính toán Lux

// Mảng chứa các chân LDR
const int LDR[2] = {2, 3};

// Khởi tạo LCD với địa chỉ I2C là 0x27, 16 cột và 2 hàng
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Biến trạng thái
int mode = MODE_MANUAL;
bool ledState = false;
volatile bool needLCDUpdate = false;
volatile int swipeDirection = 0; // 1: trái sang phải, -1: phải sang trái, 0: không có

// Biến cho xử lý nút nhấn
byte lastButtonState = HIGH;
unsigned long debounceDuration = 50;
unsigned long lastTimeButtonStateChanged = 0;

// Biến cho xử lý quét tay
volatile int lastTriggeredLDR = -1;
volatile unsigned long lastInterruptTime = 0;
const unsigned long swipeTimeout = 1500;     // Thời gian tối đa giữa các lần quét (ms)
const unsigned long debounceInterrupt = 300; // Thời gian chống dội cho interrupt (ms)

// Biến cho cập nhật LCD
unsigned long lastLCDUpdate = 0;
const unsigned long lcdUpdateInterval = 5000; // Cập nhật LCD mỗi 5 giây

void setup()
{
    // Khởi tạo cổng serial
    Serial.begin(9600);
    Serial.println("Hệ thống điều khiển LED khởi động...");

    // Khởi tạo LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Khoi dong...");

    // Cấu hình các chân I/O
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ANALOG_LDR_PIN, INPUT);

    // Cấu hình các chân LDR
    for (int i = 0; i < 2; i++)
    {
        pinMode(LDR[i], INPUT_PULLUP);
    }

    // Khởi tạo trạng thái interrupt theo chế độ hiện tại
    setupInterrupts();

    Serial.print("Chế độ hiện tại: ");
    Serial.println(mode == MODE_MANUAL ? "Thủ công (LED theo ánh sáng)" : "Tự động (LED theo quét tay)");

    // Cập nhật màn hình LCD ban đầu
    updateLCD();
}

// Hàm tính giá trị Lux từ cảm biến LDR analog
float calculateLux()
{
    int rawValue = analogRead(ANALOG_LDR_PIN);

    // Chuyển đổi giá trị analog sang điện áp
    float voltage = rawValue * (5.0 / 1023.0);

    // Công thức tính Lux: Lux = (2500/Volt - 500) / 3.3
    // Công thức này dựa trên đặc tính của LDR và điện trở kéo lên
    float lux = (2500 / voltage - 500) / 3.3;

    return lux;
}

// Cập nhật thông tin hiển thị trên LCD cho chế độ tự động
void updateAutoModeLCD()
{
    lcd.clear();

    // Dòng 1: Hiển thị chế độ và trạng thái LED
    lcd.setCursor(0, 0);
    lcd.print("Tu dong ");
    lcd.print("LED:");
    if (digitalRead(LED_PIN) == HIGH)
    {
        lcd.print("BAT");
        ledState = true;
    }
    else
    {
        lcd.print("TAT");
        ledState = false;
    }

    // Dòng 2: Hiển thị độ rọi
    lcd.setCursor(0, 1);
    float currentLux = calculateLux();
    Serial.print("Lux:");
    Serial.println(currentLux);
    lcd.print("Do roi: ");
    lcd.print(currentLux, 1);
    lcd.print(" lx");
}

// Cập nhật thông tin hiển thị trên LCD cho chế độ lướt tay
void updateManualModeLCD()
{
    lcd.clear();

    // Dòng 1: Hiển thị chế độ và trạng thái LED
    lcd.setCursor(0, 0);
    lcd.print("Vuot tay ");
    lcd.print("LED:");
    if (digitalRead(LED_PIN) == HIGH)
    {
        lcd.print("BAT");
        ledState = true;
    }
    else
    {
        lcd.print("TAT");
        ledState = false;
    }

    // Dòng 2: Hiển thị thông tin về lướt tay
    lcd.setCursor(0, 1);
    if (swipeDirection != 0)
    {
        if (swipeDirection > 0)
        {
            lcd.print("Luot: Trai->Phai");
        }
        else if (swipeDirection < 0)
        {
            lcd.print("Luot: Phai->Trai");
        }
        swipeDirection = 0; // Reset sau khi hiển thị
    }
    else
    {
        lcd.print("San sang luot tay");
    }
}

// Cập nhật thông tin hiển thị trên LCD dựa vào chế độ
void updateLCD()
{
    if (mode == MODE_MANUAL)
    {
        updateAutoModeLCD();
    }
    else
    {
        updateManualModeLCD();
    }
    needLCDUpdate = false;
}

// Cấu hình các interrupt dựa trên chế độ hiện tại
void setupInterrupts()
{
    if (mode == MODE_AUTO)
    {
        attachInterrupt(digitalPinToInterrupt(LDR[0]), handleSwipe, CHANGE);
        attachInterrupt(digitalPinToInterrupt(LDR[1]), handleSwipe, CHANGE);
        Serial.println("Đã kích hoạt chế độ quét tay");
    }
    else
    {
        detachInterrupt(digitalPinToInterrupt(LDR[0]));
        detachInterrupt(digitalPinToInterrupt(LDR[1]));
        Serial.println("Đã vô hiệu hóa chế độ quét tay");
    }
}

// Xử lý nút nhấn để chuyển đổi chế độ
void checkButton()
{
    byte currentButtonState = digitalRead(BUTTON_PIN);
    unsigned long currentTime = millis();

    if (currentButtonState != lastButtonState)
    {
        if (currentTime - lastTimeButtonStateChanged > debounceDuration)
        {
            lastTimeButtonStateChanged = currentTime;

            if (currentButtonState == LOW)
            { // Nút được nhấn (LOW do INPUT_PULLUP)
                // Chuyển đổi chế độ
                mode = (mode == MODE_MANUAL) ? MODE_AUTO : MODE_MANUAL;

                Serial.print("Đã chuyển sang chế độ: ");
                Serial.println(mode == MODE_MANUAL ? "Tự động (LED theo ánh sáng)" : "Thủ công (LED theo quét tay)");

                // Reset các biến quét tay khi chuyển chế độ
                if (mode == MODE_AUTO)
                {
                    lastTriggeredLDR = -1;
                    swipeDirection = 0;
                }

                // Cập nhật cấu hình interrupt
                setupInterrupts();

                // Đánh dấu cần cập nhật LCD
                needLCDUpdate = true;
            }
        }
        lastButtonState = currentButtonState;
    }
}

// Chế độ tự động: Bật LED khi trời tối, tắt LED khi trời sáng
void autoMode()
{
    int lightLevel = digitalRead(LDR[0]); // Sử dụng LDR[0] để đọc mức ánh sáng
    static int lastLightLevel = -1;

    if (lightLevel != lastLightLevel)
    {
        if (lightLevel == HIGH)
        {                                // HIGH = Ít ánh sáng (do INPUT_PULLUP)
            digitalWrite(LED_PIN, HIGH); // Trời tối -> Bật LED
            Serial.println("Phát hiện ánh sáng yếu -> BẬT LED");
        }
        else
        {
            digitalWrite(LED_PIN, LOW); // Trời sáng -> Tắt LED
            Serial.println("Phát hiện ánh sáng mạnh -> TẮT LED");
        }
        lastLightLevel = lightLevel;

        // Đánh dấu cần cập nhật LCD
        needLCDUpdate = true;
    }
}

// Xử lý sự kiện quét tay (được gọi bởi interrupt)
void handleSwipe()
{
    if (mode != MODE_AUTO)
    {
        return; // Chỉ xử lý khi ở chế độ tự động
    }

    unsigned long currentTime = millis();
    int triggeredLDR = -1;

    // Xác định LDR được kích hoạt
    for (int i = 0; i < 2; i++)
    {
        if (digitalRead(LDR[i]) == HIGH)
        {
            triggeredLDR = i;
            break;
        }
    }

    // Chỉ xử lý khi có LDR được kích hoạt
    if (triggeredLDR != -1)
    {
        // Kiểm tra thời gian chỉ khi là LDR mới
        if (triggeredLDR != lastTriggeredLDR)
        {
            // Debounce ngắn hơn cho hành động lướt
            if (currentTime - lastInterruptTime < 50)
            {
                return;
            }

            // Xử lý hướng quét
            if (lastTriggeredLDR != -1)
            {
                if (triggeredLDR > lastTriggeredLDR)
                {
                    digitalWrite(LED_PIN, HIGH);
                    Serial.println("Bật LED - Lướt từ trái sang phải");
                    swipeDirection = 1;
                }
                else if (triggeredLDR < lastTriggeredLDR)
                {
                    digitalWrite(LED_PIN, LOW);
                    Serial.println("Tắt LED - Lướt từ phải sang trái");
                    swipeDirection = -1;
                }

                // Đánh dấu cần cập nhật LCD
                needLCDUpdate = true;
            }

            lastTriggeredLDR = triggeredLDR;
            lastInterruptTime = currentTime;
        }
    }
}

// Kiểm tra xem có lỗi với cảm biến LDR không
bool checkLDRStatus()
{
    if (mode == MODE_AUTO)
    {
        int ldr0Status = digitalRead(LDR[0]);
        int ldr1Status = digitalRead(LDR[1]);

        if (ldr0Status == ldr1Status && ldr0Status == HIGH)
        {
            // Cả hai LDR cùng HIGH là bất thường (có thể bị che hoặc lỗi)
            Serial.println("CẢNH BÁO: Có thể LDR bị lỗi hoặc đang bị che! LDR0: " + String(ldr0Status) + " LDR1: " + String(ldr1Status));

            // Hiển thị cảnh báo trên LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("CANH BAO!");
            lcd.setCursor(0, 1);
            lcd.print("LDR co the loi");

            delay(2000); // Hiển thị cảnh báo trong 2 giây

            // Đánh dấu cần cập nhật LCD
            needLCDUpdate = true;

            return false;
        }
    }
    return true;
}

// Reset lastTriggeredLDR sau một khoảng thời gian
void checkSwipeTimeout()
{
    if (mode == MODE_AUTO && lastTriggeredLDR != -1)
    {
        unsigned long currentTime = millis();
        if (currentTime - lastInterruptTime > swipeTimeout)
        {
            lastTriggeredLDR = -1; // Reset nếu không có sự kiện mới trong khoảng thời gian swipeTimeout
            Serial.println("Đã reset trạng thái quét tay do hết thời gian");
        }
    }
}

// Hàm chính
void loop()
{
    // Kiểm tra nút nhấn
    checkButton();

    // Thực hiện chức năng theo chế độ
    if (mode == MODE_MANUAL)
    {
        autoMode();

        // Cập nhật giá trị Lux và LCD định kỳ mỗi 5 giây (chỉ ở chế độ thủ công)
        unsigned long currentMillis = millis();
        if (currentMillis - lastLCDUpdate > lcdUpdateInterval)
        {
            needLCDUpdate = true;
            lastLCDUpdate = currentMillis;
        }
    }
    else
    {
        // Kiểm tra timeout cho chế độ quét tay
        checkSwipeTimeout();
    }

    // Kiểm tra trạng thái LDR định kỳ
    static unsigned long lastCheck = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - lastCheck > 5000)
    { // Kiểm tra 5 giây một lần
        checkLDRStatus();
        lastCheck = currentMillis;
    }

    // Cập nhật LCD nếu cần
    if (needLCDUpdate)
    {
        updateLCD();
    }

    // Delay nhỏ giúp ổn định hệ thống
    delay(10);
}
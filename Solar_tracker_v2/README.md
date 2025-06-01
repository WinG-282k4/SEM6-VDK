# Hướng dẫn chạy chương trình Solar Tracker (ESP32)

## Yêu cầu phần cứng

- ESP32 Dev Board
- 2 Servo (gắn chân 18, 19)
- 4 cảm biến LDR (gắn chân 32, 33, 34, 35)
- Cảm biến DHT11 (gắn chân 14)
- Màn hình LED Matrix (dùng module MAX7219, gắn chân 21, 22, 23)
- Mạch đo điện áp tấm pin mặt trời (gắn chân 36)

## Yêu cầu phần mềm

- Arduino IDE hoặc PlatformIO
- Thư viện:
  - ESP32Servo
  - DHT sensor library
  - MD_MAX72xx
  - MD_Parola
  - Blynk (BlynkSimpleEsp32)

## Cài đặt thư viện (trên Arduino IDE)

Vào **Tools > Manage Libraries...** và tìm, cài đặt các thư viện trên.

## Cấu hình WiFi và Blynk

- Sửa thông tin WiFi trong file `Solar_tranker.cpp`:
  ```cpp
  char ssid[] = "Tên_WiFi";
  char pass[] = "Mật_khẩu_WiFi";
  ```
- Thay `BLYNK_AUTH_TOKEN` bằng token của bạn trên ứng dụng Blynk.

## Nạp chương trình

1. Kết nối ESP32 với máy tính qua cáp USB.
2. Chọn đúng board và cổng COM trong Arduino IDE.
3. Mở file `Solar_tranker.cpp` và nhấn **Upload**.

## Sử dụng

- Sau khi nạp, ESP32 sẽ tự động kết nối WiFi và gửi dữ liệu lên Blynk.
- Theo dõi nhiệt độ, độ ẩm, điện áp pin mặt trời, trạng thái servo và cảnh báo trên ứng dụng Blynk.
- Màn hình LED matrix sẽ hiển thị nhiệt độ và độ ẩm.

## Hướng dẫn tạo project Blynk

1. Tải ứng dụng Blynk IoT trên điện thoại (Android/iOS) hoặc truy cập https://blynk.cloud/
2. Đăng ký tài khoản và đăng nhập.
3. Tạo một template mới:
   - Chọn loại thiết bị: ESP32
   - Đặt tên template (ví dụ: Solar Tracker)
   - Ghi lại **Template ID** và **Device Name** nếu cần
4. Tạo một device từ template vừa tạo.
5. Lấy **BLYNK_AUTH_TOKEN** của device để dán vào mã nguồn (`#define BLYNK_AUTH_TOKEN ...`).
6. Thêm các widget vào dashboard:
   - Gauge/Value Display cho nhiệt độ (V0), độ ẩm (V1), điện áp pin mặt trời (V6)
   - Value Display cho các giá trị LDR (V2, V3, V4, V5)
   - Button (Switch) để bật/tắt tracking (V8, kiểu Switch, 0/1)
   - Notification/Label cho cảnh báo (V7)
7. Nhấn nút **Run** trên app để bắt đầu nhận dữ liệu từ ESP32.

**Lưu ý:**

- Đảm bảo ESP32 đã kết nối WiFi và nhập đúng Auth Token.
- Có thể tuỳ chỉnh giao diện và các widget theo ý muốn.

## Ghi chú

- Đảm bảo cấp nguồn đủ cho servo và LED matrix.
- Có thể chỉnh sửa các ngưỡng cảnh báo trong hàm `sendToBlynk`.

---

**Mọi thắc mắc vui lòng liên hệ tác giả hoặc tham khảo mã nguồn.**

const int threshold = 800;
int Sensor = A0;
int Value;
int Ledpin = 11;
int bright_now;
int time_on = 500;

void setup() {
  Serial.begin(9600);
  pinMode(Ledpin, OUTPUT);
  analogWrite(Ledpin, 0);
  pinMode(Sensor, INPUT_PULLUP);
  // bright_now = convert_to_outbright(analogRead(Sensor));
}

int convert_to_outbright(int in) {
  int out = map(in, 0, 1023, 0, 127);
  if (out <= 50) {
    out = 0;
  }
  return out;
}

void loop() {   
    Value = analogRead(Sensor);
    // int brightness = convert_to_outbright(Value);
    // int brightness = map(Value, 0, 1023, 0, 255);
    int brightness = convert_to_outbright(Value);

    Serial.print("Value read from sensor: ");
    Serial.print(Value);
    Serial.print(" | Brightness: ");
    Serial.print(bright_now);
    Serial.print(" -> ");
    Serial.println(brightness);
    analogWrite(Ledpin, brightness);

    // // Chỉ thay đổi độ sáng nếu có sự khác biệt
    // // if (brightness != bright_now) {
    //     int step = (brightness > bright_now) ? 1 : -1; // Xác định tăng hay giảm
    //     int diff = abs(brightness - bright_now);
    //     int delay_time = (diff > 0) ? (time_on / diff) : 0; // Tránh chia cho 0

    //     for (int i = bright_now; i != brightness; i += step) {
    //         analogWrite(Ledpin, i);
    //         // delay(delay_time);
    //     }

    //     bright_now = brightness;
    // }

    delay(time_on);
}

// const int threshold = 800;
// int Sensor = A0;
// int Value;
// int Ledpin = 11;
// int bright_now;
// int time_on = 500;

// void setup()
// {
//   Serial.begin(9600);
//   pinMode(Ledpin, OUTPUT);
//   analogWrite(Ledpin, 0);
//   pinMode(Sensor, INPUT_PULLUP);
//   bright_now = convert_to_outbright(analogRead(Sensor));
// }

// int convert_to_outbright(int in) {
//   int brightness = map(Value, 0, 1023, 0, 255);
//   if (brightness <= 50) {
//     brightness = 0;
//   }
//   return brightness;
// }

// void loop()
// {   

//     Value = analogRead(Sensor);
//     int brightness = map(Value, 0, 1023, 0, 255);
//     if (brightness >= 200) {
//       brightness = 255;
//     };
//     if (brightness <= 50) {
//       brightness = 0;
//     };

//     Serial.print("Value read from sensor: ");
//     Serial.println(Value);
//     Serial.println(bright_now, ' -> ', brightness);
//     // analogWrite(Ledpin, brightness); 
//     for (int i=bright_now; i <= brightness; i++) {
//       analogWrite(Ledpin, i);
//       delay(time_on/(brightness-bright_now));
//     }   
//     bright_now = brightness;
//     delay(time_on);
// }
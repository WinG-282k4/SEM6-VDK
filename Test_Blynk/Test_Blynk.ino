#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <NewPing.h>

#define BLYNK_TEMPLATE_ID "TMPLxxxxxxx"
#define BLYNK_TEMPLATE_NAME "xe tu dong"
#define BLYNK_AUTH_TOKEN "Cl_fcjrKmby7QZwgmyl-_EePWyiWAKxv"

// WiFi credentials
char ssid[] = "Tên_WiFi";
char pass[] = "Mật_khẩu_WiFi";

#define TRIGGER_PIN_1  23
#define ECHO_PIN_1     22
#define TRIGGER_PIN_2  19
#define ECHO_PIN_2     18
#define TRIGGER_PIN_3  5
#define ECHO_PIN_3     4
#define MAX_DISTANCE 200

NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

BlynkTimer timer;

void sendSensorData() {
  int dist1 = sonar1.ping_cm();
  int dist2 = sonar2.ping_cm();
  int dist3 = sonar3.ping_cm();

  dist1 = (dist1 == 0) ? MAX_DISTANCE : dist1;
  dist2 = (dist2 == 0) ? MAX_DISTANCE : dist2;
  dist3 = (dist3 == 0) ? MAX_DISTANCE : dist3;

  Blynk.virtualWrite(V1, dist1);
  Blynk.virtualWrite(V2, dist2);
  Blynk.virtualWrite(V3, dist3);

  Serial.printf("S1: %d, S2: %d, S3: %d\n", dist1, dist2, dist3);
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}

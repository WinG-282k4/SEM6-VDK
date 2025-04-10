int in1 = 7;
int in2 = 8;
int e1 = 9;

int in3 = 3;
int in4 = 4;
int e2 = 5;
int speed = 255;

void setup() {
  // put your setup code here, to run once:
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(e1, OUTPUT);

  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(e2, OUTPUT);
}

void gostraigh(){
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  analogWrite(e1, speed);
  analogWrite(e2, speed);
}

void turnleft(){
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  analogWrite(e1, speed);
  analogWrite(e2, speed);
}

void turnright(){
  speed =0;
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  analogWrite(e1, speed);
  analogWrite(e2, speed);
}

void loop() {
  // put your main code here, to run repeatedly:
  gostraigh();
  delay(5000);
  turnleft();
  delay(5000);
  turnright();
  delay(5000);
}

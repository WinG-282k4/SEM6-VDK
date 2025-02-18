
int led = 10;
int pin = 7;
// connect LED to pin 13
// connect pushbutton to pin 7
int value = 0; // variable to store the read value

void setup() {
pinMode(led, OUTPUT); // set pin 13 as output
pinMode (pin, INPUT); // set pin 7 as input
}

void loop() {

value = digitalRead (pin); // set value equal to the pin 7 input digitalWrite(led, value); // set LED to the pushbutton value
digitalWrite(led, value);
}
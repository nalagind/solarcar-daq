HardwareSerial Serial1(PC5, PB10);

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  pinMode(PA10, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial1.println("ok");
  digitalWrite(PA10, HIGH);
  delay(500);
  digitalWrite(PA10, LOW);
  delay(500);
}

#define motorPin 8
void setup() {
 pinMode(motorPin, OUTPUT);
}

void loop() {
 motorSpin();
}


void motorSpin(){
 digitalWrite(motorPin,HIGH);
 delay(10000);
 digitalWrite(motorPin,LOW);
 delay(2000);
  
}

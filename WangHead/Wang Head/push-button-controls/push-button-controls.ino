
const int btnPin = 4  ;
const int motorPin =  7  ;
long randNumber; 
int buttonState = 0;
void setup () {
  pinMode(btnPin, INPUT);
  pinMode(motorPin, OUTPUT);
  // Init USB serial port for debugging
 Serial.begin(9600);
}

void loop () {  
buttonState = digitalRead(btnPin);
  if(buttonState == HIGH){
motorSpin(); 
  }
}
void motorSpin(){
  randNumber = random(1000, 10000);
Serial.println(randNumber);
 digitalWrite(motorPin,HIGH);
 delay(randNumber);
 digitalWrite(motorPin,LOW);
 delay(1000);
  
}

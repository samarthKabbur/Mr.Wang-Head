const int btnPin = 4; 
const int randNumber = 5000 ; 
int buttonState = 0;
#define motorPin 7 
void setup() {
  // put your setup code here, to run once:
pinMode(btnPin, INPUT);
pinMode(motorPin, OUTPUT);
Serial.begin(9600);

}



void motorSpin(){
 /* //randNumber = random(1000, 10000);
 Serial.println(randNumber);
 digitalWrite(motorPin,HIGH);
 delay(5000);
 digitalWrite(motorPin,LOW);
 delay(1000);
 Serial.println("ok");
 */
 Serial.println("Button Pressed");
 digitalWrite(motorPin,HIGH);
 delay(1000);
 digitalWrite(motorPin,LOW);
 delay(1000);
 Serial.println("Can Press Again");
 
}


void loop() {
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(btnPin);
if(buttonState == HIGH){
motorSpin(); 
}
}

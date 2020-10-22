


/*
// https://www.instructables.com/id/Arduino-Sleep-and-Wakeup-Test-With-DS3231-RTC/
// Arduino wake-up with DS3231
// corrected the library for use with Atmega168P
// corrected by: https://github.com/rocketscream/Low-Power/issues/45
// and also: https://github.com/rocketscream/Low-Power/issues/14
// connect DS3231 pin INT/SQW to D2 on Arduino (INT0, PCINT18, PD2)
// v8: includes ethernet shield and upload to Thingspeak
// code from
https://github.com/iobridge/ThingSpeak-Arduino-Examples/blob/master/Ethernet/Ardui
no_to_ThingSpeak.ino
// and for two values: https://community.particle.
io/t/uploading-sensor-data-to-thingspeak/5497
// V9: modified for transmission over HC12 via hardware serial
// 1. prepare values for transmission: int sendDutyCycle, time and date
// 2. serialPrint only what needs to be transmitted
// 3. includes Thingspeak code
// 4. send 3 integers: humidity %, minute, hour and sensor identification (A,
B, ..)
//
// V10: code without thingspeak, no ethernet module, and cleaned up
// status 1/12/17: tested ok
// V11: send 4 values: duty cycle %, temperature, minute, hour and sensor
identification (A, B, ..)
// status 5/01/2018: tested ok
// V12: added second alarm ALM2, added 5th value: analog battery value
// status 5/01/2018: tested ok
//
// Use with Arduino_serial_Arduino_RX_HC12_burst_test_hardwareSerial_v4
// Use with ESP8266_RX_HC12_hardware-Serial_Thingspeak_v1 (experimental, to be
tested; 03/2018: tested and = ok)
// NEW: since 25/03/2018, use with ESP8266_irrigation_controller_v1 (copied from
ESP8266_RX_HC12_hardware-Serial_Thingspeak_v1)
**************************************
 ESP8266 irrigation controller:
 sensor module (TX)
**************************************
*/
/*
 Connect:
 SDA to A4
 SCL to A5
 D2 to DS3231 SQW
 sensor board to external supply 5V
 external supply ground to controller ground
*/
#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
#include <RTClibExtended.h>
#include <LowPower.h>
 #include <SPI.h> //Thingspeak
#define DS3231_I2C_ADDRESS 0x68
RTC_DS3231 RTC; //we are using the DS3231 RTC
// Variable Setup
// NOTE: pin D8 is used for reading incoming square wave for duty cycle
//measurement
long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;
const int wakePin = 2; //use interrupt 0 (pin 2) and run function wakeUp when pin 2
//gets LOW
int ledPin = 13; //use arduino on-board led for indicating sleep or wakeup
//status
int wakeStatus = 9; //use D10 to drive 5V power to sensor module and HC-12 with
//MOSFET, 1=wake
 int temperaturePin = A0; // analog input for LM35 temperature sensor
float temperatureT1;
int batteryPin = A1;
float batteryVoltage;
int temperatureRead;
byte goToSleepNow = 1;
byte ledStatus = 1;
volatile word timerValue[4];
volatile byte testState = 0;
volatile boolean signalPresent = false;
boolean newValuesAvailable = false;
float pwmPeriod, pwmWidth, pwmFrequency, pwmDutyDisplay;
unsigned long pwmDuty;
const byte x = 10; // loop value: loop amount = x, number of dutycycle
//measurements in 1 run
int sendDutyCycle; // contains integer value of float cycleValue
int dutyCycleStatus = 8; // Input Capture Pin
//-------------------------------------------------
void wakeUp() // here the interrupt is handled after wakeup
{
}
//------------------------------------------------------------
void setup() {
 //Set pin D2 as INPUT for accepting the interrupt signal from DS3231
 pinMode(wakePin, INPUT);
 //switch-on the on-board led for 1 second for indicating that the sketch is ok
//and running
 pinMode(ledPin, OUTPUT);
 digitalWrite(ledPin, LOW);
 //Set MOSFET gate drive to output
 pinMode(wakeStatus, OUTPUT);
digitalWrite(wakeStatus, LOW);
 analogReference(INTERNAL);
 delay(1000);
 //Initialize communication with the clock
 Wire.begin();
 RTC.begin();
 // A convenient constructor for using "the compiler's time":
 // DateTime now (__DATE__, __TIME__);
 // uncomment following line when compiling and uploading,
 // then comment following line and immediately upload again
  //RTC.adjust(DateTime(__DATE__, __TIME__)); //set RTC date and time to
//COMPILE time, see instructions above
 //clear any pending alarms
 RTC.armAlarm(1, false);
 RTC.clearAlarm(1);
 RTC.alarmInterrupt(1, false);
 RTC.armAlarm(2, false);
 RTC.clearAlarm(2);
 RTC.alarmInterrupt(2, false);
 //Set SQW pin to OFF (in my case it was set by default to 1Hz)
 //The output of the DS3231 INT pin is connected to this pin
 //It must be connected to arduino D2 pin for wake-up
RTC.writeSqwPinMode(DS3231_OFF);
  // setting two alarms for 9:10 each tuesday and thursday every week. 
 // see for explanation: https://github.com/JChristensen/DS3232RTC#alarm-methods
//format:
//RTC.setAlarm(alarmType,  seconds, minutes, hours, day or date); 
 //RTC.setAlarm(ALM1_MATCH_DAY, 0,     14,      18,     dowMonday); //set your wake-up time here:
 RTC.setAlarm(ALM1_MATCH_HOURS, 29, 18, 0); //set your wake-up time here
 RTC.setAlarm(ALM2_MATCH_DAY, 0, 10, 9, dowThursday); //where "xx" is minutes
 // every 00 minutes past the hour;
 // if every minute is needed change MINUTES to SECONDS (only for ALM1)
 // matches seconds AND minutes when _MINUTES is used. Sequence of time:
 // first seconds, then minutes, hours, daydate
 // or: seconds (but enter 00, is ignored), minutes then hours, daydate for ALM2
 // zero's mean: always
 // example: Set alarm1 every day at 18:33
 // RTC.setAlarm(ALM1_MATCH_HOURS, 33, 18, 0); set your wake-up time here
 // RTC.alarmInterrupt(1, true);
 RTC.alarmInterrupt(1, true); //set alarm1
 RTC.alarmInterrupt(2, true); //set alarm2
 Serial.begin(115200);
tmElements_t tm;
//RTC.read(tm);
Serial.print(tm.Hour, DEC);
Serial.print(':');
Serial.print(tm.Minute,DEC);
Serial.print(':');
Serial.println(tm.Second,DEC);
}
//------------------------------------------------------------
void loop() {
 //On first loop we enter the sleep mode
 if (goToSleepNow == 1) { // value 1 = go to
//sleep
 attachInterrupt(0, wakeUp, LOW); //use interrupt 0 (pin
//PD2) and run function wakeUp when pin 2 gets LOW
 digitalWrite(ledPin, LOW); //switch-off the led
//for indicating that we enter the sleep mode
 ledStatus = 0; //set the led status
//accordingly
// full power down of the ATMEGA 32P chip
 LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); //arduino enters sleep
//mode here
 detachInterrupt(0); //execution resumes from here after wake-up
 //When exiting the sleep mode we clear the alarm
 RTC.armAlarm(1, false);
 RTC.clearAlarm(1);
 RTC.alarmInterrupt(1, false);
 RTC.armAlarm(2, false);
 RTC.clearAlarm(2);
 RTC.alarmInterrupt(2, false);
 goToSleepNow = 0; // value 0 = do not go
//to sleep
 }
 //cycles the led to indicate that we are no more in sleep mode
 if (ledStatus == 0) {
 ledStatus = 1;
 digitalWrite(ledPin, HIGH);
 }
 //digitalWrite(wakeStatus, HIGH); // set wakeStatus to HIGH, wake mode, activate 
//moisture sensor electronics
 delay (1000);
 //measure(); //execute duty cycle measurement during wakeUp
 delay (500);
 digitalWrite(wakeStatus, LOW); //initiate wake status
//LOW, sleepmode, de-activate moisture sensor electronics
 goToSleepNow = 1;
 RTC.alarmInterrupt(1, true);
 RTC.alarmInterrupt(2, true);
}
 void measure()
{
 DateTime now = RTC.now();
 temperatureRead = analogRead(temperaturePin);
 temperatureT1 = temperatureRead / 9.3091;
 batteryVoltage = analogRead(batteryPin) / 500.0;
 int repeat1 = 0;
 unsigned long signalMax1 = 0; // max value for duty cycle
 unsigned long signalMin1 = 1000000; // min value for duty cycle
 // unsigned long currentMillis = millis();
 unsigned long cycleTotal = 0;
 float cycleValue = 0;
 while (repeat1 < x)
 {
 getPwmValues();
 if (newValuesAvailable)
 {
 repeat1++;
 newValuesAvailable = false;
 if (signalPresent)
 {
 if (pwmDuty < signalMin1) signalMin1 = pwmDuty;
 if (pwmDuty > signalMax1) signalMax1 = pwmDuty;
 cycleTotal += pwmDuty;
 pwmDutyDisplay = pwmDuty / 1000.0;
 }
 }
 }
 if (signalPresent) // if signal is present
 {
 DateTime now = RTC.now();
 cycleTotal -= signalMin1;
 cycleTotal -= signalMax1;
 cycleValue = cycleTotal / 1000.0 / (x - 2);
 Serial.print('<'); // this section for HC-12 transmission
 Serial.print(cycleValue); // ref Robin2's Serial Output Basics
 Serial.print(','); // http://forum.arduino.cc/index.php?topic=396450.0
 Serial.print(temperatureT1); // send temperature read from analog input
//temperaturePin
 Serial.print(',');
 Serial.print(batteryVoltage); // send battery voltage
 Serial.print(',');
 Serial.print(now.minute(), DEC); // send minute
 Serial.print(',');
 Serial.print(now.hour(), DEC); // send hour
 Serial.print(",B>"); // send sensor identifier
 }
 else // if no signal is present
 {
 if (digitalRead(dutyCycleStatus) == true) //if true then 5V present on pin
//8, sensor present
 {
 Serial.print('<'); // this section for HC-12 transmission
 Serial.print(",100"); // ref Robin2's Serial Output Basics
 Serial.print(','); // http://forum.arduino.cc/index.php?topic=396450.0
 Serial.print(temperatureT1); // send temperature read from analog input
//temperaturePin
 Serial.print(',');
 Serial.print(batteryVoltage); // send battery voltage
 Serial.print(',');
 Serial.print(now.second(), DEC);
 Serial.print(',');
 Serial.print(now.minute(), DEC);
 Serial.print(",B>");
 }
 else //if false then 0V on pin 8 then there is hardware error
 {
 Serial.print('<'); // this section for HC-12 transmission
 Serial.print(",0"); // ref Robin2's Serial Output Basics
 Serial.print(','); // http://forum.arduino.cc/index.php?topic=396450.0
 Serial.print(temperatureT1); // send temperature read from analog input
//temperaturePin
 Serial.print(',');
 Serial.print(batteryVoltage); // send battery voltage
 Serial.print(',');
 Serial.print(now.second(), DEC);
 Serial.print(',');
 Serial.print(now.minute(), DEC);
 Serial.print(",B>");
 }
 }
} 
void pwmMeasureBegin()
{
 for (byte j = 0; j < 4; j++)
 {
 timerValue[j] = 0;
 }
 
 TCCR1A = 0; // normal operation mode
 TCCR1B = 0; // stop timer clock (no clock source)
 TCNT1 = 0; // clear counter
 TIFR1 = bit (ICF1) | bit (TOV1); // clear flags
 testState = 0; // clear testState
 signalPresent = false; //reset
 TIMSK1 = bit (ICIE1); // interrupt on input capture
 TCCR1B = bit (CS10) | bit (ICES1);// start clock with no prescaler, rising
//edge on pin D8
}
ISR (TIMER1_CAPT_vect)
{
 signalPresent = true;
 switch (testState) {
 case 0: // first rising edge
 timerValue[0] = ICR1;
 testState = 1;
 break;
 case 1: // second rising edge
 timerValue[1] = ICR1;
 TCCR1B &= ~bit (ICES1); // capture on falling edge (pin D8)
 testState = 2;
 break;
 case 2: // first falling edge
 testState = 3;
 break;
 case 3: // second falling edge
 timerValue[2] = ICR1;
 testState = 4;
 break;
 case 4: // third falling edge
 timerValue[3] = ICR1;
 testState = 5; // all tests done
 TCCR1B = 0; //stop timer
 break;
 }
}
void getPwmValues()
{
 static boolean measurementInProcess = false;
 if (!measurementInProcess)
 {
 pwmMeasureBegin();
 measurementInProcess = true;
 delay(50);//time for signalPresent to be set
 }
 else if (testState == 5 || !signalPresent)//measurement complete or noSignal
 {
 if (!signalPresent)
 {
 // Serial.println("100% duty cycle. No PWM signal, zero data");
 // float cycleValue = 100.0;
 // Serial.print(cycleValue, 3);
 // Serial.println(" %");
 // displayTime();
 // Serial.println();
 // Serial.println();
 }
 calculatePwmValues();
 measurementInProcess = false; //reset
 }
}
void calculatePwmValues()
{
 word periodValue = timerValue[3] - timerValue[2];
 // word is the same as unsigned int
 word widthValue = timerValue[2] - timerValue[1];
 word diffValue = widthValue - periodValue;
 pwmPeriod = (periodValue * 0.0625);
 pwmWidth = diffValue * 0.0625;
 pwmDuty = (pwmWidth / pwmPeriod) * 100000;
 pwmFrequency = 1000 / (pwmPeriod);
 newValuesAvailable = true;
}

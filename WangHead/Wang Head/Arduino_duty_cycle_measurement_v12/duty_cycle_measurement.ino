// https://www.instructables.com/id/Arduino-Sleep-and-Wakeup-Test-With-DS3231-RTC/
// Arduino wake-up with DS3231
// corrected the library for use with Atmega168P
// corrected by: https://github.com/rocketscream/Low-Power/issues/45
// and also: https://github.com/rocketscream/Low-Power/issues/14
// connect DS3231 pin INT/SQW to D2 on Arduino (INT0, PCINT18, PD2)
// v8: includes ethernet shield and upload to Thingspeak
//    code from https://github.com/iobridge/ThingSpeak-Arduino-Examples/blob/master/Ethernet/Arduino_to_ThingSpeak.ino
// and for two values: https://community.particle.io/t/uploading-sensor-data-to-thingspeak/5497
// V9: modified for transmission over HC12 via hardware serial
//   1. prepare values for transmission: int sendDutyCycle, time and date
//   2. serialPrint only what needs to be transmitted
//   3. includes Thingspeak code
//   4. send 3 integers: humidity %, minute, hour and sensor identification (A, B, ..)
//
// V10: code without thingspeak, no ethernet module, and cleaned up
// status 1/12/17: tested ok
// V11: send 4 values: duty cycle %, temperature, minute, hour and sensor identification (A, B, ..)
// status 5/01/2018: tested ok
// V12: added second alarm ALM2, added 5th value: analog battery value
// status 5/01/2018: tested ok
//
// Use with Arduino_serial_Arduino_RX_HC12_burst_test_hardwareSerial_v4
// Use with ESP8266_RX_HC12_hardware-Serial_Thingspeak_v1 (experimental, to be tested)
/*
   Connect:
   RX to HC12 TX
   TX to HC12 RX
   D2 to DS3231 SQW
   D12 to MOSFET powerdrive gate on dutycycle sensor
   D8 to dutycycle sensor output (pin 11)
   A0 to LM35 output
   LM35 Vcc and GND to controller
   dutycycle ground to HC12 ground
   MOSFET drain to dutycycle sensor board ground
   MOSFET source to controller ground
   sensor board to external supply 5V
   external supply ground to controller ground
*/
//LIBRARIES
#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>
 #include <SPI.h> //Thingspeak
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <TimeLib.h>

//DEFINITIONS
#define DS3231_I2C_ADDRESS 0x68
#define motorPin 7 



// GLOBAL VARIABLES

  //Sleep Timer Variables
int wakePin = 2;    //use interrupt 0 (pin 2) and run function wakeUp when pin 2 gets LOW
int ledPin = 13;    //use arduino on-board led for indicating sleep or wakeup status
byte goToSleepNow = 1;
byte ledStatus = 1;
volatile word timerValue[4];
volatile byte testState = 0;
volatile boolean signalPresent = false;
boolean newValuesAvailable = false;

  //Sound Variables
static const uint8_t PIN_MP3_TX = 4; // Connects to module's RX
static const uint8_t PIN_MP3_RX = 5; // Connects to module's TX

//OBJECTS
RTC_DS3231 RTC;      //we are using the DS3231 RTC
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);
// Create the Player object
DFRobotDFPlayerMini player;
tmElements_t tm;
//-------------------------------------------------

void wakeUp()        // here the interrupt is handled after wakeup

{
    //detachInterrupt(digitalPinToInterrupt(2));
}

//------------------------------------------------------------

void setup() {

//THINGS TO TRY: 
 //set clock speed to 2MHZ, but it affects timer functions, but saves battery
 //CLKPR = 0x03 ;



//SLEEP TIMER SETUP
  
  //Set pin D2 as INPUT for accepting the interrupt signal from DS3231
  pinMode(wakePin, INPUT);
  //switch-on the on-board led for 1 second for indicating that the sketch is ok and running
  pinMode(ledPin, OUTPUT);
  //digitalWrite(wakePin, HIGH);
  digitalWrite(ledPin, LOW);
  delay(1000);
  //Initialize communication with the clock
  Wire.begin();
  RTC.begin();
  // A convenient constructor for using "the compiler's time":
  // DateTime now (__DATE__, __TIME__);
  // uncomment following line when compiling and uploading,
  // then comment following line and immediately upload again

   RTC.adjust(DateTime(__DATE__, __TIME__));   //set RTC date and time to COMPILE time, see instructions above
//RTC.adjust(DateTime(2020, 2, 15, 23, 38, 0));
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
  //example: Set alarm1 every day at 18:33: 00 seconds, 33 minutes, 18 hours, 0 = every day;
  // if for example Sunday then: dowSunday if a date then date of the month
  //
  // see for explanation: https://github.com/JChristensen/DS3232RTC#alarm-methods
  //Alarm Syntax: RTC.setAlarm(alarmType, seconds, minutes, hours, dayOrDate);
  //Alarm Syntax: the day of the week is from 1 to 7, 1 is monday, 7 is sunday. 
 RTC.setAlarm(ALM1_MATCH_DAY, 0, 20, 12, 1);  //set your wake-up time here:
  //RTC.setAlarm(ALM2_MATCH_MINUTES, 0, 10, 0, 0);  //where "xx" is minutes
  // every 00 minutes past the hour;
  // if every minute is needed change MINUTES to SECONDS (only for ALM1)
  // matches seconds AND minutes when _MINUTES is used. Sequence of time:
  // first seconds, then minutes, hours, daydate
  // or: seconds (but enter 00, is ignored), minutes then hours, daydate for ALM2
  // zero's mean: always
  // example: Set alarm1 every day at 18:33
//   RTC.setAlarm(ALM1_MATCH_HOURS, 39, 17, 0);  set your wake-up time here
  // RTC.alarmInterrupt(1, true);

  RTC.alarmInterrupt(1, true); //set alarm1
  RTC.alarmInterrupt(2, true); //set alarm2


//SOUND SETUP

  Serial.begin(9600);
  softwareSerial.begin(9600);
  if (player.begin(softwareSerial)) {
    Serial.println("OK"); //for debugging
    // Set volume to maximum (0 to 30).
    player.volume(5);
    // Play the first MP3 file on the SD card
    //actual code that plays the file
    player.play(1); //the number in the parenthesis corresponds to the file upload number. 
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!"); //debugging
  }
}

//------------------------------------------------------------

void loop() {

//SLEEP TIMER MAIN CODE; everything else in the runAll(); function. 

  //On first loop we enter the sleep mode
  if (goToSleepNow == 1) {                                 // value 1 = go to sleep
    attachInterrupt(0, wakeUp, LOW);                       //use interrupt 0 (pin PD2) and run function wakeUp when pin 2 gets LOW
    digitalWrite(ledPin, LOW);                             //switch-off the led for indicating that we enter the sleep mode
    ledStatus = 0;                                         //set the led status accordingly
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);   //arduino enters sleep mode here
    detachInterrupt(0); //execution resumes from here after wake-up
    //When exiting the sleep mode we clear the alarm
    RTC.armAlarm(1, false);
    RTC.clearAlarm(1);
    RTC.alarmInterrupt(1, false);
    RTC.armAlarm(2, false);
    RTC.clearAlarm(2);
    RTC.alarmInterrupt(2, false);
    goToSleepNow = 0;                                      // value 0 = do not go to sleep
  }

  //cycles the led to indicate that we are no more in sleep mode
  if (ledStatus == 0) {
    ledStatus = 1;
    digitalWrite(ledPin, HIGH);
  }
  delay (1000);
  runAll(); //execute duty cycle measurement during wakeUp
  delay (500);
  goToSleepNow = 1;
  RTC.alarmInterrupt(1, true);
  RTC.alarmInterrupt(2, true);
}

void runAll(){
//Serial.println("Hello World!"); //debugging

//NO LOOP CODE FOR SOUND
playSound();
//MOTOR CODE
motorSpin();
}


void motorSpin(){ //spin the motor for one second, then turn off. If the function is continously repeated, then will cycle the motor. 
  digitalWrite(motorPin,HIGH);
  delay(1000);
  digitalWrite(motorPin, LOW);
  delay(1000); 
}

void playSound(){
   // Set volume to maximum (0 to 30).
    player.volume(5);
    // Play the first MP3 file on the SD card
    //actual code that plays the file
    player.play(1); //the number in the parenthesis corresponds to the file upload number. 
    delay(31000);
    player.pause();
    player.sleep();
}
  

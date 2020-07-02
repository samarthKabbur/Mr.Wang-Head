#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  Serial.write("Hello World");
enableAlarmInLowPower();
Serial.write("Hello");
}

void loop() {
  // put your main code here, to run repeatedly:

}
void enableAlarmInLowPower(void) {
    // Read the control register.
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    uint8_t value = Wire.read();
    Wire.endTransmission();

    // Update it.
    value |= bit(6) | bit(7);  // set bits 6 and 7

    // Write it back.
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.write(value);
    Wire.endTransmission();
    Serial.write(value);
}

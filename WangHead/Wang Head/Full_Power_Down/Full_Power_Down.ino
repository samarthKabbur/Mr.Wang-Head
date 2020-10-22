#include <LowPower.h>

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

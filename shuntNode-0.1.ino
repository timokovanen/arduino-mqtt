// shuntNode-0.1
// arduino nano + ina226 + hw-411(lm2596)
// ina266: original 0.1ohm resistor unsoldered
//         rc-filter with 1uF ceramic cap and two 10ohm (2%) resistors 
// uses Arnd\@SV-Zanshin INA library V1.0.13
// https://github.com/SV-Zanshin/INA
// based on BackgroundRead example
// timo.kovanen@phnet.fi

#include <INA.h>
 
const uint8_t  INA_ALERT_PIN = 8;

INA_Class         INA;
volatile uint8_t  deviceNumber    = UINT8_MAX;
volatile uint64_t sumBusMillVolts = 0;
volatile int64_t  sumBusMicroAmps = 0;
volatile uint8_t  readings        = 0;

ISR(PCINT0_vect) { // routine is called whenever the INA_ALERT_PIN changes value
  
  *digitalPinToPCMSK(INA_ALERT_PIN) &= ~bit(digitalPinToPCMSKbit(INA_ALERT_PIN));  // Disable PCMSK
  PCICR &= ~bit(digitalPinToPCICRbit(INA_ALERT_PIN));        // disable interrupt for the group
  sei();                                                     // Enable interrupts (for I2C calls)
  sumBusMillVolts += INA.getBusMilliVolts(deviceNumber);     // Add current value to sum
  sumBusMicroAmps += INA.getBusMicroAmps(deviceNumber);      // Add current value to sum
  readings++;
  INA.waitForConversion(deviceNumber);  // Wait for conversion & INA int. flag
  cli();                                // Disable interrupts
  *digitalPinToPCMSK(INA_ALERT_PIN) |=
      bit(digitalPinToPCMSKbit(INA_ALERT_PIN));       // Enable PCMSK pin
  PCIFR |= bit(digitalPinToPCICRbit(INA_ALERT_PIN));  // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(INA_ALERT_PIN));  // enable interrupt for the group

}

void setup() {

  pinMode(INA_ALERT_PIN, INPUT_PULLUP);  // Declare pin with internal pull-up resistor
  *digitalPinToPCMSK(INA_ALERT_PIN) |= bit(digitalPinToPCMSKbit(INA_ALERT_PIN));  // Enable PCMSK
  PCIFR |= bit(digitalPinToPCICRbit(INA_ALERT_PIN));  // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(INA_ALERT_PIN));  // enable interrupt for the group

  Serial.begin(19200); // serial console  
  while (!Serial) {;} // wait for serial port to connect

//  Serial.println(F("\n{\"version\":\"shuntNode-0.1\"}"));

  delay(2000);

  uint8_t devicesFound = 0;
  
  while (deviceNumber == UINT8_MAX)
  {
    devicesFound = INA.begin(1, 100000);  // +/- 1 Amps maximum for 0.1 Ohm resistor
    for (uint8_t i = 0; i < devicesFound; i++) {
      if (strcmp(INA.getDeviceName(i), "INA226") == 0) {
        deviceNumber = i;
        INA.reset(deviceNumber);  // reset device to default settings
        break;
      }
    }
    if (deviceNumber == UINT8_MAX) {
      Serial.println(F("{\"status\":\"error\", \"version\":\"shuntNode-0.1\"}"));
      delay(5000);
    }
  }
//  Serial.println(F("{\"status\":\"ok\"}"));
//  Serial.println(deviceNumber);
//  Serial.println();
  INA.setAveraging(64, deviceNumber);                   // Average each reading 64 times
  INA.setBusConversion(8244, deviceNumber);             // Maximum conversion time 8.244ms
  INA.setShuntConversion(8244, deviceNumber);           // Maximum conversion time 8.244ms
  INA.setMode(INA_MODE_CONTINUOUS_BOTH, deviceNumber);  // Bus/shunt measured continuously
  INA.AlertOnConversion(true, deviceNumber);            // Make alert pin go low on finish
}

void loop() {

//  static long lastMillis = millis();  // store the last time we printed something
  if (readings >= 1) {
//    Serial.print(F("{\"AR\":"));
//    Serial.print((float)(millis() - lastMillis) / 1000, 2);
    Serial.print(F("{\"V\":"));
    Serial.print((uint16_t)sumBusMillVolts / readings);
    Serial.print(F(", \"I\":"));
//    Serial.print((float)sumBusMicroAmps / readings / 1000.0, 4);
    Serial.print((int32_t)sumBusMicroAmps / readings / 1000);
    Serial.println(F(", \"status\":\"ok\", \"version\":\"shuntNode-0.1\"}"));
//    lastMillis = millis();
    cli();  // disable interrupts to reset values
    readings        = 0;
    sumBusMillVolts = 0;
    sumBusMicroAmps = 0;
    sei();  // enable interrupts again
  }

}

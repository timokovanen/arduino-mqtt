// ethNode-0.1
// arduino + w5500 ethernet module
// uses ethernet-2.0.0 library
// timo.kovanen@phnet.fi

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {0x0e, 0x00, 0x00, 0x00, 0x01, 0x03};

boolean ethUp = false;

void setup() {

  Ethernet.init(53); // configure the CS pin (ethernet shield 10)

  Serial.begin(9600); // serial console  
  while (!Serial) {;} // wait for serial port to connect

  Serial.println(F("\nethNode-0.1"));

  delay(2000);

  if (Ethernet.linkStatus() != LinkON) {
      Serial.println(F("eth: no link"));
  }

}

void loop() {

  if (Ethernet.linkStatus() != LinkON) {
    ethUp = false;
  } else {
    
    if (!ethUp) {
      if (Ethernet.begin(mac) != 0) {
        ethUp = true;
        Serial.println(F("eth: obtained dhcp lease"));
        Serial.print(F("eth: ip "));
        Serial.println(Ethernet.localIP());
      }
    }

    if (ethUp) {
    
      // dhcp lease and link active
      // place your network related calls here
          
      switch (Ethernet.maintain()) {
        case 1:
          //renew fail
          ethUp = false;
          Serial.println(F("eth: renew fail"));
          break;
    
        case 2:
          //renew success
          Serial.println(F("eth: renew success"));
          Serial.print(F("eth: ip "));
          Serial.println(Ethernet.localIP());
          break;
    
        case 3:
          //rebind fail
          ethUp = false;
          Serial.println(F("eth: rebind fail"));
          break;
    
        case 4:
          //rebind success
          Serial.println(F("eth: rebind success"));
          Serial.print(F("eth: ip "));
          Serial.println(Ethernet.localIP());
          break;
    
        default:
          //nothing happened
          break;
      }
    }
  }
  
}

// mqttNode-0.1
//    uses pubsubclient-2.8 library
// based on ethNode-0.1
//    arduino + w5500 ethernet module
//    uses ethernet-2.0.0 library
// timo.kovanen@phnet.fi

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define CLIENT_VERSION  "mqttNode-0.1"
#define CLIENT_MAC      "0e0000000103"
#define CLIENT_TOPIC    CLIENT_MAC

#define NOTIFICATION_TOPIC  "N/" CLIENT_TOPIC
#define READ_TOPIC          "R/" CLIENT_TOPIC
#define WRITE_TOPIC         "W/" CLIENT_TOPIC

#define BROKER_PORT     1883

byte mac[] = {0x0e, 0x00, 0x00, 0x00, 0x01, 0x03};
IPAddress brokerIP(10, 199, 1, 1);
boolean ethUp = false;
boolean mqttSetupComplete = false;

boolean _publishOnline = false;
boolean _publishVersion = false;

EthernetClient ethClient;
PubSubClient mqttClient; // (ethClient);

void setup() {

  Ethernet.init(53); // configure the CS pin (ethernet shield 10)

  Serial.begin(9600); // serial console  
  while (!Serial) {;} // wait for serial port to connect

  Serial.println(F("\nmqttNode-0.1"));

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
        mqttSetup();
      }
    }

    if (ethUp) {
    
      // dhcp lease and link active
      // place your network related calls here

      if (mqttSetupComplete) {
        if (!mqttClient.connected()) {  // disconnected
          if (mqttClient.connect(CLIENT_MAC, NOTIFICATION_TOPIC "/status", 2, true, "{\"status\":\"offline\"}")) { // willTopic,willQos,willRetain,willMessage
            Serial.println(F("mqtt: connected"));
            mqttClient.subscribe(READ_TOPIC "/#");
            mqttClient.subscribe(WRITE_TOPIC "/#");
            publishOnline();
//          publishOnConnect(); // // one time, after connection published messages
          }
        } else {
          publishData(); // call in loop, eth & mqtt ok
        } 
      }  

       
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
  mqttClient.loop();

}

void mqttSetup() {
  
    mqttClient.setClient(ethClient);
    mqttClient.setServer(brokerIP, BROKER_PORT);
    mqttClient.setCallback(callback);
    mqttSetupComplete = true;  
    Serial.print(F("mqtt: broker ip "));
    Serial.println(brokerIP);

}

void publishData() {

  if (_publishVersion) {
    mqttClient.publish(NOTIFICATION_TOPIC "/info", "{\"version\":\"" CLIENT_VERSION "\"}");
    _publishVersion = false;
  }

  if (_publishOnline) {
    publishOnline();
    _publishOnline = false;
  }
}

void publishOnline() {
  mqttClient.publish(NOTIFICATION_TOPIC "/status", "{\"status\":\"online\"}", true); // retained message
}

void callback(char* topic, byte* payload, unsigned int length) {
  
  payload[length] = '\0'; // terminate string with '0'
  if (!(strcmp(topic, READ_TOPIC "/info"))) {
    _publishVersion = true;
  }
  if (!(strcmp(topic, READ_TOPIC "/status"))) {
    _publishOnline = true;
  }

}

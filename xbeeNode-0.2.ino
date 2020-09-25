// xbeeNode-0.2
// arduino mega 2560
// based on mqttNode-0.1
//    uses pubsubclient-2.8 library
// based on ethNode-0.1
//    arduino + w5500 ethernet module
//    uses ethernet-2.0.0 library
// timo.kovanen@phnet.fi

// 0.2 forwards full zigbee frame, changed serial console to 19200 8n1
// 0.1 forwards dataframes only

// send received api frames to broker N/<CLIENTID>/xbee/api payload({"frame": "<BASE64>"})
// request at command: W/<CLIENTID>/xbee/at payload({ "command": "<COMMAND W/O AT-PREFIX>" })


#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <XBee.h>
#include <ArduinoJson.h>
#include <Base64.h>

#define CLIENT_VERSION  "xbeeNode-0.2"
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

XBee xbee = XBee();
AtCommandRequest atRequest = AtCommandRequest();

void setup() {

  Ethernet.init(53); // configure the CS pin (ethernet shield 10)

  Serial.begin(19200); // serial console  
  while (!Serial) {;} // wait for serial port to connect

  Serial1.begin(9600); // xbee api serial1  
  while (!Serial1) {;} // wait for serial1 port to connect

  xbee.setSerial(Serial1);
  
  Serial.println(F("\nxbeeNode-0.1"));

  delay(2000);

  if (Ethernet.linkStatus() != LinkON) {
      Serial.println(F("eth: no link"));
  }

}

void loop() {

  xbee.readPacket();

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

  if (xbee.getResponse().isAvailable()) {

    Serial.print(F("xbee: ["));
    Serial.print(F("7E, "));
    if (xbee.getResponse().getMsbLength() < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(xbee.getResponse().getMsbLength(), HEX);    
    Serial.print(F(", "));
    if (xbee.getResponse().getLsbLength() < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(xbee.getResponse().getLsbLength(), HEX);    
    Serial.print(F(", "));
    
    Serial.print(xbee.getResponse().getApiId(), HEX);
    for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
      Serial.print(F(", "));
      if (xbee.getResponse().getFrameData()[i] < 0x10) {
        Serial.print(F("0"));
      }
      Serial.print(xbee.getResponse().getFrameData()[i], HEX);    
    }
    Serial.print(F(", "));
    if (xbee.getResponse().getChecksum() < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(xbee.getResponse().getChecksum(), HEX);    

    Serial.println(F("]"));

    char inputString[xbee.getResponse().getFrameDataLength()+5]; // start + msblen + lsblen + apiid + framedata + checksum
    inputString[0] = 0x7e;
    inputString[1] = xbee.getResponse().getMsbLength();
    inputString[2] = xbee.getResponse().getLsbLength();
    inputString[3] = xbee.getResponse().getApiId();
    for (uint16_t i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
      inputString[i+4] = xbee.getResponse().getFrameData()[i];
    }
    inputString[xbee.getResponse().getFrameDataLength()+4] = xbee.getResponse().getChecksum();

    byte inputStringLength = sizeof(inputString);

    uint16_t encodedLength = Base64.encodedLength(inputStringLength);
    char encodedString[encodedLength];
    Base64.encode(encodedString, inputString, inputStringLength);
    Serial.print(F("xbee: \""));
    Serial.print(encodedString);
    Serial.println(F("\""));

    char payloadString[encodedLength+12] = "{\"frame\":\""; // {" + base64 + "}    
    for (uint16_t i = 0; i < encodedLength; i++) {
      payloadString[i+10] = encodedString[i];
    }
    payloadString[encodedLength+10] = '\"';
    payloadString[encodedLength+11] = '}';
    payloadString[encodedLength+12] = '\0';
    
    mqttClient.publish(NOTIFICATION_TOPIC "/xbee/api", payloadString);
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
  if (!(strcmp(topic, WRITE_TOPIC "/xbee/at"))) {
    
    DynamicJsonDocument doc(33);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      const char* command = doc["command"];
      atRequest.setCommand((uint8_t*) command);
      xbee.send(atRequest);
    }
    
  }

}

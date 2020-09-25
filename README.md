# arduino and mqtt related stuff

ethNode - Maintain Ethernet connection with DHCP (W5500)

mqttNode - Basic MQTT functions and connection over ethNode

xbeeNode - Send Zigbee API frames (base64 encoded) and rquest AT commands over mqttNode with Digi Xbee (S2C) module

shuntNode - Read current and bus voltage (INA226) and send measurements to serial port as JSON

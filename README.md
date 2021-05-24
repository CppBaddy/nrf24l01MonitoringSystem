# nrf24l01MonitoringSystem
NRF24L01 based wireless monitoring system. Consists of multiple WirelessBase and WirelessSensor modules.

2.4GHz Wireless Dev Kit. Sensor board powered by CR2032 battery. Each sensor board has RJ11 input for landline phone line and three discrete/analog inputs, discrete outputs for other use. Sensor module detects phone ring and input change events and broadcasts these to the base controllers. Each message transmitted includes battery voltage value.

Wireless base controller board powered by 12V DC. Controller drives LED strip WS2811. Color of light motion depends on activated sensor and its channel. Primarely used as an intercom and a landline ring light indicator for people with hearing issues.

You need to assighn unique board IDs to each sensor board (see wirelessSensor/Makefile) and include these to wirelessBase too.

![Wireless Sensor board schematics](https://github.com/CppBaddy/nrf24l01MonitoringSystem/blob/main/wirelessSensorSchematics.png?raw=true)

![Wireless Base board schematics](https://github.com/CppBaddy/nrf24l01MonitoringSystem/blob/main/wirelessBaseSchematics.png?raw=true)

![JLCPCB PCB](https://github.com/CppBaddy/nrf24l01MonitoringSystem/blob/main/pcb.png?raw=true)

Original schematics and some pictures can be found ![here](https://easyeda.com/Yulay/nrf24l01-attiny85-sensor-board)


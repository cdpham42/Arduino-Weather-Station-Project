# Arduino-Weather-Station-Project

Personal project using an Arduino, BMP180 Pressure Sensor, HIH6130 Humidity Sensor, microSD breakout board, Chronodot realtime clock, and RGB LCD Shield to observe and record current atmospheric conditions.

Initial code for reading and recording observations was developed during an undergraduate course at the University of Illinois Urbana-Champaign: ATMS 315 Meteorological Instrumentation, taught by Professor Eric Snodgrass. Further changes were made for style and readability.

The LCD Shield is able to take readings from attached sensors and output current conditions: temperature, dew point, humidity, and pressure. Final implementation does not save the readings to the SD card, as well as suppress serial output. LCD Shield has an auto-off timer, and only takes readings when is on.

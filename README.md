# esp-relay-control

Application for controlling a Wemos D1 Mini ESP8266 board with a connected relay shield. Creates a WiFi captive portal where the state of the relay can be toggled. By default the relay control panel can be found at http://relaycontroller.local/ but the DNS will direct any network traffic to the ESP's address. 

The SSID has a random number at the end making it possible to run several setups at the same time.

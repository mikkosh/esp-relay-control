/* Relay controller app for ESP8266 / Wemos D1 Mini + Relay shield */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>


char softAP_ssid[40];
const char *softAP_password = "";

/* hostname for mDNS.http://relaycontroller.local */
String myHostname = "relaycontroller";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft Accesspoint network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

const int relayPin = D1;

void setup() {
  
  Serial.begin(115200);
  Serial.println();
  
  pinMode(relayPin, OUTPUT);

  // automatically generate SSID from hostname + add a trailing random ID
  String temp = String(myHostname);
  temp.toUpperCase();
  temp += "-%d";
  temp.toCharArray(softAP_ssid, temp.length()+1);
  randomSeed(analogRead(A0));
  sprintf(softAP_ssid, softAP_ssid, random(50,100));
  
  Serial.print("Configuring access point:");
  Serial.println(softAP_ssid);
  
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  if (MDNS.begin(myHostname)) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound(handleNotFound);
  
  server.begin(); // Web server start
  Serial.println("HTTP server started");

  // start with the relay pin in high state
  digitalWrite(relayPin, HIGH);
}
void handleRoot() {
  char temp[2300];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  // if hostname is not the correct one, do a redirect
  // since we're running a captive portal, this is just a vanity thingy
  Serial.print("Request hostname is:");
  Serial.println(server.hostHeader());  
  if(server.hostHeader() != myHostname+".local" && server.hostHeader() != "connectivitycheck.gstatic.com") {
    server.sendHeader("Location", "http://"+myHostname+".local", true);
    server.send ( 302, "text/plain", "");  
    return;
  }
  
  snprintf(temp, 2300,
           "<!DOCTYPE html>\n<html>\
  <head>\
    <title>Relay controller</title>\
    <style>\
      body { background-color: #efefef; font-family: Arial, Helvetica, Sans-Serif; Color: #333; font-size: 40px; text-align:center;}\
      h1 {font-size: 2em;} p {font-size: 1em;}\
.toggle {\n\
  -webkit-appearance: none;\n\
  -moz-appearance: none;\n\
  appearance: none;\n\
  width: 124px;\n\
  height: 64px;\n\
  display: inline-block;\n\
  position: relative;\n\
  border-radius: 50px;\n\
  overflow: hidden;\n\
  outline: none;\n\
  border: none;\n\
  cursor: pointer;\n\
  background-color: #707070;\n\
  transition: background-color ease 0.3s;\n\
}\n\
.toggle:before {\n\
  content: \"on off\";\n\
  display: block;\n\
  position: absolute;\n\
  z-index: 2;\n\
  width: 60px;\n\
  height: 60px;\n\
  background: #fff;\n\
  left: 2px;\n\
  top: 2px;\n\
  border-radius: 50px;\n\
  font: 20px/58px Helvetica;\n\
  text-transform: uppercase;\n\
  font-weight: bold;\n\
  text-indent: -44px;\n\
  word-spacing: 74px;\n\
  color: #fff;\n\
  text-shadow: -1px -1px rgba(0,0,0,0.15);\n\
  white-space: nowrap;\n\
  box-shadow: 0 1px 2px rgba(0,0,0,0.2);\n\
  transition: all cubic-bezier(0.3, 1.5, 0.7, 1) 0.3s;\n\
}\n\
.toggle:checked {\n\
  background-color: #4CD964;\n\
}\n\
.toggle:checked:before {\n\
  left: 62px;\n\
}\n\
    </style>\
  </head>\
  <body>\
    <h1>Relay control page</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <div style=\"text-align:center;\">\
    <input class=\"toggle\" onclick=\"location.href='/toggle';\" type=\"checkbox\" %s/>\
    </div>\
  </body>\
</html>",

           hr, min % 60, sec % 60, (digitalRead(relayPin) == HIGH)?"checked":""
          );
       
  server.send(200, "text/html", temp);
}

void handleToggle() {
  if(digitalRead(relayPin) == HIGH) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void loop() {
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
  // MDNS
  MDNS.update();
}



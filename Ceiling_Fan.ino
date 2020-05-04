#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <DNSServer.h>       //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

const int RF_PIN = 2;

const int SHORT_ON { 350 };
const int SHORT_OFF { 400 };
const int LONG_ON { 650 };
const int LONG_OFF { 700 };
const int REST { 11600 };

const int ACTIVE_DIP [4] {1,1,1,1};

const char* ssid = "SSIDgoesHERE";
const char* password = "PASSWORDgoesHERE";

MDNSResponder mdns;
ESP8266WebServer server ( 80 );


void ZERO() {
  digitalWrite(RF_PIN, LOW);
  delayMicroseconds(LONG_OFF);
  digitalWrite(RF_PIN, HIGH);
  delayMicroseconds(SHORT_ON);
}

void ONE() {
  digitalWrite(RF_PIN, LOW);
  delayMicroseconds(SHORT_OFF);
  digitalWrite(RF_PIN, HIGH);
  delayMicroseconds(LONG_ON);
}

void DIP() {
  for (int pos = 0; pos < 4; ++pos ) {
    if (ACTIVE_DIP[pos] == 1) { ONE(); }
    else { ZERO(); }
  }
}

void PULSEDELAY() {
  digitalWrite(RF_PIN, LOW);
  delayMicroseconds(REST);
}


void setup ( void ) {
    pinMode(RF_PIN, OUTPUT);
    Serial.begin ( 115200 );
    Serial.println ( "Initializing" );
    
    WiFi.mode(WIFI_STA); //We don't want the ESP to act as an AP
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    if ( mdns.begin ( ssid, WiFi.localIP() ) ) {
        Serial.println ( "MDNS responder started" );
        Serial.print ("IP = ");
        Serial.println (WiFi.localIP());
    }
    bool result = SPIFFS.begin();
    if (result != true) { SPIFFS.format();}
    Serial.println("SPIFFS opened: " + result);
    server.on("/control", HTTP_ANY, commandSet);    // don't need to handle GET as Fan Control is a one way transmission
    server.onNotFound ( handleNotFound );   // actually, "not specified" - will get resources from file system
    server.begin();
    Serial.println ( "HTTP server started" );
}

void TX(int op, int max_burst) { 
  for (int burst = 1; burst <= max_burst; burst++) {
    ZERO();
    ONE();
    DIP();
    ZERO();
    for (int btn = 1; btn <= 6; btn++) { 
      if (btn != op) {ZERO();} 
      else {ONE();} 
    }
    PULSEDELAY();
  }
} 

void commandSet()
{
//  String operation = server.arg("operation");
//  Serial.print("Operation = ");
//  Serial.println(operation);

  if (operation == "light_dimmer") { TX(6,60); }
  else {TX(operation.toInt(),6);}

  server.send ( 200, "text/plain", "OK" );
}

void handleNotFound() {
    if(!handleFileRead(server.uri())) {
        server.send(404, "text/plain", "FileNotFound");
    }
}

bool handleFileRead(String path){
    Serial.println("handleFileRead: " + path);
    if(path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
        if(SPIFFS.exists(pathWithGz))
            path += ".gz";
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

String getContentType(String filename){
    if(server.hasArg("download")) return "application/octet-stream";
    else if(filename.endsWith(".htm")) return "text/html";
    else if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".png")) return "image/png";
    else if(filename.endsWith(".gif")) return "image/gif";
    else if(filename.endsWith(".jpg")) return "image/jpeg";
    else if(filename.endsWith(".ico")) return "image/x-icon";
    else if(filename.endsWith(".xml")) return "text/xml";
    else if(filename.endsWith(".pdf")) return "application/x-pdf";
    else if(filename.endsWith(".zip")) return "application/x-zip";
    else if(filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

void loop ( void ) {
    mdns.update();
    server.handleClient();
}

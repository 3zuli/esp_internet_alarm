/*
 *  The ESP8266 Internet Connection Alarm
 *  by @3zuli
 *  based off the /ESP8266WiFi/WiFiClient example
 *  
 *  This program continually tries to connect to a website and when there's an error
 *  it turns on a 12V warning light / strobe / siren / whatever else it's connected to.
 *
 *  Circuit:
 *  ESP8266 wired up according to <ADD ESP WIRING GUIDE>
 *    -> GPIO02 connected to Gate of IRF540N mosfet through a 560R resistor
 *  Mosfet Source connected to GND, GND of 12V supply also connected to GND
 *  12V input connected to + lead of the strobe, - lead of the strobe connected to Drain of the mosfet
 *  todo: schematic
 *
 */

#include <ESP8266WiFi.h>

// This is not yet fully implemented, please ignore
//#define DETECT_INTERNET
#define DETECT_NO_INTERNET

// Your WiFi network credentials
const char* ssid     = "Your SSID";
const char* password = "Your Password";

// The URL we will use to test our connection
// We'll be connecting to http://httpbin.org/get , the "get" part is specified later in the code
// httpbin returns a very small amount of data, therefore should work
// even on the slowest connection (EDGE/2G)
const char* host = "httpbin.org";

// Global variable to track our connection status
bool hasInternet = true; 

// The ESP pin on which the strobe light is connected
const int strobePin = 02;


void led(int pin){
#if defined(DETECT_INTERNET)
    digitalWrite(pin,hasInternet);
#elifdef DETECT_NO_INTERNET
    digitalWrite(pin,~hasInternet);
#else
    digitalWrite(pin,~hasInternet);
#endif
}
//// led(strobePin);


int connectWifi(int retryDelay=500){
  // Retry connection to the specified network until success.
  // @param int retryDelay: Time in milliseconds to wait between status checking (default 500ms)
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  //Assume we aren't connected and turn on the strobe
  hasInternet=false; 
  digitalWrite(strobePin,HIGH);
  
  //Start connecting, the connection process is done by the ESP in the background
  WiFi.begin(ssid, password); 
  
  // values of WiFi.status() (wl_status_t) defined in wl_definitions.h located in (Windows):
  // %AppData%\Roaming\Arduino15\packages\esp8266\hardware\esp8266\1.6.5-947-g39819f0\libraries\ESP8266WiFi\src\include
  // Mac & Linux will vary
  
  wl_status_t wifiStatus = WL_IDLE_STATUS; //Assume nothing
  while (wifiStatus != WL_CONNECTED) {
    // While the ESP is connecting, we can periodicaly check the connection status using WiFi.status()
    // We keep checking until ESP has successfuly connected
    wifiStatus = WiFi.status();
    switch(wifiStatus){
      // Print the error status we are getting
      case WL_NO_SSID_AVAIL:
          Serial.println("SSID not available");
          hasInternet=false;
          break;
      case WL_CONNECT_FAILED:
          Serial.println("Connection failed");
          hasInternet=false;
          break;
      case WL_CONNECTION_LOST:
          Serial.println("Connection lost");
          hasInternet=false;
          break;
      case WL_DISCONNECTED:
          Serial.println("WiFi disconnected");
          hasInternet=false;
          break;
    }
    delay(retryDelay);
  }
  
  // Here we are connected, we can turn off the strobe & print our IP address
  digitalWrite(strobePin,LOW);
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(strobePin,OUTPUT); // Set strobe pin as OUTPUT
  delay(10);

  // We start by connecting to a WiFi network
  connectWifi(500);
}


void loop() {
  // Loop/check connection every 5 seconds
  delay(5000);
  
  // Turn on/off the strobe according to current connection state
  digitalWrite(strobePin,!hasInternet);
  
  wl_status_t wifiStatus = WiFi.status();
  if(wifiStatus != WL_CONNECTED){
      // If we lost connection to our WiFi, reconnect
      Serial.println("Lost WiFi connection, reconnecting...");
      connectWifi(500); // (strobe is activated inside this function)
  }
  else Serial.println("WiFi OK"); // Otherwise we are connected

  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connection
  WiFiClient client;
  const int httpPort = 80;
  // Attempt to connect to httpbin.org:80 (http)
  if (!client.connect(host, httpPort)) {
    // If we can't connect, we obviously don't have internet
    Serial.println("Connection failed!!!");
    // Set hasInternet to false, turn on the strobe, 
    // exit the loop() function and wait for the next round
    hasInternet=false;
    digitalWrite(strobePin,HIGH);
    return;
  }
  // Otherwise we are now connected to httpbin.org, 
  // which means our internet is at least somewhat working :)
  // Set hasInternet to true, turn off the strobe, proceed to the GET
  hasInternet=true;
  digitalWrite(strobePin,LOW);
  
  // We now create a URI for the request
  String url = "/get";
  
  Serial.print("Requesting URL: ");
  Serial.println(host+url); 
  
  // We need to manually create the HTTP GET message
  // client.print() will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10); //Wait for it to send the message
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  // We don't check if we actually received something from server
  // TODO: measure ping and turn on the strobe, when it's too high
  
  Serial.println("closing connection");
  Serial.println();
}


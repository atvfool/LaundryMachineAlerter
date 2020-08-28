/*
  Laundry Machine Alerter for Nano 33 IoT

  Connects to Wifi network and senses light using an optoresistor, if the threshold is met then it fires a post request

*/

#include <SPI.h>
#include <WiFiNINA.h>

// Constants
const char ssid[] = "SSID HERE";                    // Wifi SSID
const char pass[] = "WIFI PASSWORD HERE";           // Wifi password
const int statusPin = 13;                           // On board LED
const int ledPin = 2;                               // External LED
const int sensorPin = A0;                           // Sensor Pin
const unsigned long ALERT_SENT_TIMEOUT = 300000;    // Alert sent time out in ms, is 5 minutes

// variables
int status = WL_IDLE_STATUS;
WiFiSSLClient client;                               // Initialize the Wifi client
char server[] = "maker.ifttt.com";                  // server url
int sensorLow = 1023;
int sensorHigh = 0;
int sensorValue = 0;
int ledPinState = LOW;
bool alertSent = false;                             // Keep track of if the alert was sent, this will get set to false once 5 minutes has passed
unsigned long alertSentTime = 0;

/*
 * Setup
 * runs at startup
 */
void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Set pins
  pinMode(statusPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);

  // Turn on board LED on while it's starting up
  digitalWrite(statusPin, HIGH);
  
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    blinkStatusLEDError();
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
    Serial.println(fv + " < " + WIFI_FIRMWARE_LATEST_VERSION);
  }
  connectToAP();    // Connect to Wifi Access Point
  printWifiStatus();

  // Calibrate sensors
  Serial.println("Calibrating sensor");
  
  // Blink LED for 5 seconds
  unsigned long sensorCalibrateStart = millis();
  while((millis() - sensorCalibrateStart) < 5000){
    if((millis()) % 500 == 0) {// blink every 500 ms to calibrate the sensor
      blinkLED();
    }
    sensorValue = analogRead(sensorPin);
    if(sensorValue > sensorHigh){
      sensorHigh = sensorValue;
    }
    if(sensorValue < sensorLow){
      sensorLow = sensorValue;
    }
  }
  if(sensorHigh != sensorLow)
    Serial.println("Sensor calibrated");
  else {
    Serial.println("Sensor not calibrated, something went wrong");
    blinkStatusLEDError();
  }
  
  // turn on board LED off once connected.
  digitalWrite(ledPin, LOW);
  digitalWrite(statusPin, LOW);
  sensorValue = 0; // needed to reset this so it didn't immediately send an alert
  delay(1000);

}

/*
 * Loop
 * Runs indefinitely 
 */
void loop() {
  
  if(millis() - alertSentTime > ALERT_SENT_TIMEOUT){ // sets to false, if the light is sensor still reads it, will send again. 
    alertSent = false;
  }
  if(alertSent){
    digitalWrite(ledPin, HIGH); 
  }else{
    digitalWrite(ledPin, LOW);
    sensorValue = 0;
  }
  readSensorValue();

  if(shouldSendAlert() && status == WL_CONNECTED)
  {
    Serial.println("****Sending Alert****");
    Serial.println(sensorValue);
    Serial.println("Starting connection to server....");
    if(client.connect(server, 443)){
      Serial.println("connected to server");
      client.println("POST /trigger/[event name here]/with/key/[key here] HTTP/1.1");
      client.println("Host: maker.ifttt.com");
      client.println("Connection: close");
      client.println();
      Serial.println("Post request sent");
      alertSent = true;
      alertSentTime = millis();
    }else{
      Serial.println("connection to server failed");
      delay(10000); // wait 10 seconds before doing anything else
    }
  }

	// write out any response from the web server
  while(client.available()){
    char c = client.read();
    Serial.write(c);
  }
  
}

/*
 * returns true if an alert should be sent
 */
bool shouldSendAlert(){
  bool result = false;
  if(sensorValue > 200 && alertSent == false){
    result = true;
  }
  return result;
}

/*
 * reads the sensor value and converts it to a value between 0 and 255 every 500ms
 */
void readSensorValue(){
  if((millis() % 500) == 0) // read every 500 ms
  {
    sensorValue = analogRead(sensorPin);
    sensorValue = map(sensorValue, sensorLow, sensorHigh, 0, 255);
    Serial.println("Sensor Value: " + (String)sensorValue);
  }
}

/*
 * print the wifi status to the serial monitor
 */
void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP(); // Device IP address
  Serial.print("IP Address: ");
  Serial.println(ip);
}

/*
 * Attempt to connect to WiFi
 */
void connectToAP() {
  // Try to connect to Wifi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);

    // wait 1 second for connection:
    delay(1000);
    Serial.println("Connected...");
  }
}

/*
 * blink the status LED every 250 ms forever
 */
void blinkStatusLEDError(){
  while(true){
    if(millis() % 250 == 0) {// blink every 250 ms to indicate error
      blinkStatusLED();
    }
  }
}

/*
 * change the state of the Status LED
 */
void blinkStatusLED(){
  ledPinState = (ledPinState == LOW ? HIGH : LOW);
  digitalWrite(statusPin, ledPinState);
}

/*
 * change the state of the other LED
 */
void blinkLED(){
  ledPinState = (ledPinState == LOW ? HIGH : LOW);
  digitalWrite(ledPin, ledPinState);
}

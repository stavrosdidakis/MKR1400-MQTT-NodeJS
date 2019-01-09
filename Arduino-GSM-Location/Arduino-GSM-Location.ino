//Plymouth -  50.3749161, -4.1302485
//            50.3591270, -4.1299386

#include <MKRGSM.h>
#include "arduino_secrets.h"
#include "PubSubClient.h"
#include<avr/dtostrf.h>
#include <Wire.h>
#include "rgb_lcd.h"

//The sensitive data are set in the in the arduino_secrets.h tab
const char PINNUMBER[]     = SECRET_PINNUMBER;
const char GPRS_APN[]      = SECRET_GPRS_APN;
const char GPRS_LOGIN[]    = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

//Initialize the library instance
GSMLocation location;
GPRS gprs;
GSM gsmAccess;
GSMClient gsmClient;

//Settings for the MQTT client
//IP address of the server (set in arduino_secrets.h)
const char server[]   = MQTT_BROKER;
const char inTopic[]  = "dat503/serverToClient";
const char outTopic[] = "dat503/client1";
const char clientID[] = "MKR-client1";
//Variable to store the MQTT message/payload
char pubCharsPayload[100];

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle received messages
}
//Initialize the client
PubSubClient mqttClient(server, 80, callback, gsmClient);

//Initialize variables for the LCD screen
rgb_lcd lcd;
const int colorR = 50;
const int colorG = 0;
const int colorB = 50;

void setup() {
  //Initialize serial communications and wait for port to open
  Serial.begin(9600);
  while (!Serial) {
    ; //Wait for serial port to connect - needed for native USB port only
  }
  //Print a message to the Serial Monitor
  Serial.println("Initializing...");

  //Set up the LCD's settings, for row, columns, and color
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  //Print a message to the screen
  lcd.print("Initializing...");

  //Connection state
  bool connected = false;

  //After starting the modem with GSM.begin(),
  //connect the module to the GPRS network with the
  //APN, login and password
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
      Serial.println("Connected to the network");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Connected");
    } else {
      Serial.println("Still connecting...");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Still connecting");
      delay(1000);
    }
  }
  //After the connection is accomplished,
  //start the location process
  location.begin();
}

void loop() {
  //If location is available:
  // - Print on the Serial Monitor the coordinates,
  //   altitude, and accuracy
  // - Print on the LCD screen latitute and longitude
  // - Send the MQTT message

  if (location.available()) {
    //Display info on the Serial Monitor
    Serial.print("Location: ");
    Serial.print(location.latitude(), 7);
    Serial.print(", ");
    Serial.println(location.longitude(), 7);

    Serial.print("Altitude: ");
    Serial.print(location.altitude());
    Serial.println("m");

    Serial.print("Accuracy: +/- ");
    Serial.print(location.accuracy());
    Serial.println("m");
    Serial.println();

    //Display info on the LCD screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Latitude:");
    lcd.setCursor(0, 1);
    lcd.print(location.latitude(), 7);
    delay(3000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Longitude:");
    lcd.setCursor(0, 1);
    lcd.print(location.longitude(), 7);
    delay(3000);

    //Send the MQTT message
    mqttMessage();
  }
}

void mqttMessage(){
  if (mqttClient.connect(clientID)) {
    //Construct the string for the latitude
    char bufferLat[20];
    String latVal = dtostrf(location.latitude(), 10, 7, bufferLat);

    //Construct the string for the longitude
    char bufferLon[20];
    String lonVal = dtostrf(location.longitude(), 10, 7, bufferLon);

    //Construct the string for the sensor value (not used here)
    float sensorValue = 0;
    //sensorValue = analogRead(0);
    char bufferSensor[20];
    String sensorVal = dtostrf(sensorValue, 5, 3, bufferSensor);

    //Construct the payload as a JSON object
    String payload = "{\"Latitude\":" + latVal + ",\"Longitude\":" + lonVal + ",\"Sensor\":" + sensorVal + "}";
    payload.toCharArray(pubCharsPayload, (payload.length() + 1));
    mqttClient.publish(outTopic, pubCharsPayload);

    Serial.println("MQTT Published");
  } else {
    Serial.println("MQTT client can't connect!");
    mqttReconnect();
  }
  mqttClient.loop();
  delay(3000);
}

void mqttReconnect() {
  //Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    //Attempt to connect
    if (mqttClient.connect(clientID)) {
      Serial.println("MQTT client connected");
      //Once connected, publish an announcement...
      mqttClient.publish(outTopic, clientID);
      //... and resubscribe
      mqttClient.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      //Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

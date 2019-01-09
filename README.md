# Broadcasting GPS coordinates with the Arduino MKR GSM 1400 and the MQTT protocol

## Introduction

The project uses the [Arduino MKR GSM 1400](https://goo.gl/rFCPWe) to extract the GPS coordinates of the current location, and utilize GPRS and MQTT to transmit the coordinates to an online broker. The transmitted information can be extracted by any other clients that have access to the service; this means that data can be logged in a database, or used in real-time for applications such as automation, remote control, visualization, and many more.

---

## System Details
For the needs of this project, the following hardware components were used:
* Arduino MKR GSM 1400
* Antenna for MKR GSM 1400
* Grove LCD RGB screen
* Lithium battery, 3.7V, 2A

In addition to this, a custom 3D-printed object was made to host all components together (which you can find in **models/MKRGSM-3DModel.zip**). The device is mobile, and can be used for a sufficient period of time on its own. Following are photos of the finalised device.

<p align="center">
<img alt="abramovic" src="assets/gps.png" width="480" />
</p>

---
## Arduino Code
This part explains the code used for the board

##### Import Libraries / Declarations
For this project, a few dependencies are needed, which can be found in the folder named **libraries**; here follows a description for them:
* The **MKRGSM** library is used for accessing the board's functions in connecting to a GSM network (Global System for Mobile communication) using a SIM card and accessing the GPS module for extracting information related to location. The **arduino_secrets** is needed for setting the SIM card's settings (GPRS/APN - General Packet Radio Services / Access Point Name), and related credentials - if any. In order to find out what settings to use, check online about your provider's APN keyword (for EE it is "everywhere") - usually there is no login and password.
* **PubSubClient** is the MQTT (Message Queuing Telemetry Transport) library that allows us to use the board as a client for broadcasting information to the broker (server); this is needed for transmitting real-time information to our system, and also for logging sensor values, location coordinates, etc., to our main database (mongoDB).
* The library **dtostrf** is needed for an accurate conversion of the location coordinates to a String (this is required by the MQTT protocol). For example, the latitude will return a number such as: 50.3749161 - this has to be converted accurately to a String: "50.3749161".
* Finally, the libraries **Wire** and **rgb_lcd** are used by the RGB LCD screen that is used in this project; if you do not use the same screen, or no screen at all, then this part is not necessary.

```C
#include <MKRGSM.h>
#include "arduino_secrets.h"
#include "PubSubClient.h"
#include <avr/dtostrf.h>
#include <Wire.h>
#include "rgb_lcd.h"
```
The following code initializes most of the variables and settings needed in the project. First, its necessary to store the sensitive data of the GPRS network in our variables (i.e. APN, pin number, login, and password), so that we can access them later for GPRS and GPS connections - set the full information of the credentials is in the arduino_secrets.h tab.

Also, we need to initialize the GSM and GPRS instances from the library MKRGSM. These will allow us to communicate with the hardware and get the location coordinates, as well as connect to the web for the MQTT communication (sending and receiving messages).

In the MQTT settings we have to provide the topic that the client is subscribed to, and the IP address of the MQTT broker, which has been setup for this example on a private site. If you would like to setup your own, you can use [CloudMQTT](https://www.cloudmqtt.com), [HiveMQ](https://www.hivemq.com), or develop your own instance in [Heroku](https://elements.heroku.com/addons/cloudmqtt).

Finally, we configure the LCD settings - color and brightness of the screen.

```C
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
const int colorR = 150;
const int colorG = 0;
const int colorB = 150;
```

##### Setup
In the setup function we initialize the serial communication between the board and the computer (for debugging purposes), and also start the LCD display. Following that, we connect the board with the GPRS service, and if successful, we make a call to the GPS module in order to get the location's coordinates.

```C
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
```

##### Loop Function
In the loop function, we wait for the module to make available the location, and when this happens, the information is printed on the Serial Monitor, as well as the LCD screen. In addition, the mqttMessage() function is executed, which takes the provided information of the location, and sends it to the MQTT broker.

```C
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
```

##### MQTT
Finally, the following code makes sure that the board is connected using MQTT with the online broker, which distributes the messages to any other client that is subscribed to the same topic. This can be helpful if we need to develop further connections, such as a database that stores the broadcasted information, or an additional system for automation, remote control, visualization, and so on.

The mqttMessage() function takes the information from the location (latitude and longitude), and constructs a payload with this information formatted as a JSON message. After the message has been formatted accordindly, it is published in the appropriate.

If the client cannot connect, the mqttReconnect() function is executed, which intends to make a new attempt in communicating with the MQTT broker. The function includes also a debugging feature, so that we can easily understand why the problem occurs - this appears on the Serial Monitor.

```C
void mqttMessage(){
  if (mqttClient.connect(clientID)) {
    //Construct the string for the latitude
    char bufferLat[20];
    String latVal = dtostrf(location.latitude(), 10, 7, bufferLat);

    //Construct the string for the longitude
    char bufferLon[20];
    String lonVal = dtostrf(location.longitude(), 10, 7, bufferLon);

    //Construct the string for the sensor value
    //This is used only for demostration purposes here
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
```

***

## Node.js - Application Server
This project includes in addition a code example for [Node.js](https://nodejs.org/en/) that demonstrates how to read the MQTT messages broadcasted from the MKR1400. The package.json file references all libraries needed for this project, which they are also included in the node_modules folder, or they can be installed using Terminal (or Command Prompt) with:

```Terminal
npm install
```

In the Node.js app, we need to import the Express and MQTT modules. The Express module is used in creating a web server for our application, which runs locally on port 3000.

```JavaScript  
//Import the modules
var express = require('express');
var mqtt = require('mqtt')

//Store the express function to the app variable
var app = express();
//Create a server on localhost:3000
var server = app.listen(process.env.PORT || 3000);
app.use(express.static('public'));
console.log("Node is running on port 3000...");

//Setup variables for the MQTT communication
var MQTT_TOPIC = "dat503/client1";
var MQTT_ADDR = "mqtt://broker.i-dat.org:80";
var MQTT_PORT = 80;
//Connect the client to the broker's address
var client  = mqtt.connect(MQTT_ADDR);

//When the MQTT client is connected, subscribe to the topic
client.on('connect', function() {
  client.subscribe(MQTT_TOPIC, { qos: 1 });
});

//When a message is received from the client, convert it to a JSON object, and print on the console
client.on('message', function (topic, message) {
  var jsonObj = JSON.parse(message);
  console.log(jsonObj);
});
```

In order to execute this example, make sure that you set in Terminal (or Command Prompt) the current directory (cd) for this **NodeJS** folder. Use npm to start the service:

```Terminal
npm start
```

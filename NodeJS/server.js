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

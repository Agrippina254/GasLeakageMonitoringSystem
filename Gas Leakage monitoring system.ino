#include <dht.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// --------------------------------------------------------------------------------------------
//        GAS LEAKAGE MONITORING SYSTEM WITH CONTROLS & DASHBOARD ACTUATION
// --------------------------------------------------------------------------------------------

// Watson IoT connection details
#define MQTT_HOST "hy0o6u.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:hy0o6u:ESP32:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "Istilllovefairies@8113"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"

// Add GPIO pins used to connect devices
#define DHT_PIN 4 // GPIO pin the data line of the DHT sensor is connected to
#define RGB_RED 6 // GPIO pin to the RED pin of the RGB LED
#define RGB_BLUE 6 //GPIO pin to the BLUE pin of the RGB LED
#define RGB_GREEN 8 //GPIO pin to the GREEN pin of the RGB LED
#define MQ6 34   //GPIO Pin that received data from the MQ6 gas sensor


dht DHT;

// Add WiFi connection information
char ssid[] = "modero";  // your network SSID (name)
char pass[] = "Istilllovefairies@8113";  // your network password


// --------------------------------------------------------------------------------------------
//        SECTION 1: SETTING UP THE ENVIRONMENT (IBM CLOUD & WIFI CONNECTION)
// --------------------------------------------------------------------------------------------

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// variables to hold data
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[50];

float h = 0.0; // humidity
float t = 0.0; // temperature
int potValue = 0; //MQ6 analog
String Warning_Type = ""; //warning type
int gas_leak_val = 0;


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);
}

void setup()
{
  // Start serial console
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("GAS LEAKAGE MONITORING SYSTEM & CONTROL UNIT");

  // Start WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");



  // Connect to MQTT - IBM Watson IoT Platform
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);
  
  } else {
    Serial.println("MQTT Failed to connect!");
    ESP.restart();
  }

  //Initializing the digitla pins as outputs
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);

  

}

void loop()
{
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
  
  DHT.read11(DHT_PIN);
  h = DHT.humidity;
  t = DHT.temperature; // uncomment this line for Celsius
  // t = dht.readTemperature(true); // uncomment this line for Fahrenheit
  Serial.println("Temperature: ");
  Serial.println(t);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } 
  else {   
    Serial.println("Reading from DHT Sensor ...");

    //Test if the Temperature and Humidity sensor data is within the MQ6 Gas sensor operating range
    if((t>-20) && (t<70))
    { 
      //Reading values from the potentiometer value
      potValue = analogRead(MQ6);
      Serial.println(potValue);
      delay(2500);
      
    
      // Determining the alert zones based on the sensitivity of the MQ6 (using ppm)
      gas_leak_val = int(potValue*(9800/4095));
      if((gas_leak_val >=0) && (gas_leak_val<=1500))
      {
        //Early warning sign  
        Warning_Type = "Early Warning Sign";
        digitalWrite(RGB_GREEN, HIGH);   //turning on the GREEN LED - informational
        Serial.println(Warning_Type);
        delay(1000);   

        // Send data to Watson IoT Platform and Print Message to console in JSON format
        status["Gas Leakage Level(ppm)"] = gas_leak_val;
        status["Warning Type"] = Warning_Type;
        serializeJson(jsonDoc, msg, 100);
        Serial.println(msg);
        if (!mqtt.publish(MQTT_TOPIC, msg)) 
        {
          Serial.println("MQTT Publish failed");
        }

        
      }
      else if((gas_leak_val >=1501) && (gas_leak_val<=3000))
      {
        //Critical level warning sign
        Warning_Type = "Critical level warning sign";
        digitalWrite(RGB_BLUE, HIGH);   //turning on the BLUE LED 
        Serial.println(Warning_Type);
        delay(1000); 

         // Send data to Watson IoT Platform and Print Message to console in JSON format
        status["Gas Leakage Level(ppm)"] = gas_leak_val;
        status["Warning Type"] = Warning_Type;
        serializeJson(jsonDoc, msg, 100);
        Serial.println(msg);
        if (!mqtt.publish(MQTT_TOPIC, msg)) 
        {
          Serial.println("MQTT Publish failed");
        }
      }
      
      else if((gas_leak_val >=3001) && (gas_leak_val<=4095))
      {
        //Catastrophic level warning sign
        Warning_Type = "Catastrophic level warning sign";
        digitalWrite(RGB_RED, HIGH);   //turning on the RED LED
        Serial.println(Warning_Type);
        delay(1000); 

         // Send data to Watson IoT Platform and Print Message to console in JSON format
        status["Gas Leakage Level(ppm)"] = gas_leak_val;
        status["Warning Type"] = Warning_Type;
        serializeJson(jsonDoc, msg, 100);
        Serial.println(msg);
        if (!mqtt.publish(MQTT_TOPIC, msg)) 
        {
          Serial.println("MQTT Publish failed");
        }
        
      }
    }
    else
    {
      Serial.println("System operating under harsh environmental conditions");
      Serial.println("System performance is very inaccurate");
    }
}
}

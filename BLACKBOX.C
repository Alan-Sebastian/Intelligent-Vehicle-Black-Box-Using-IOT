#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <Adafruit_Sensor.h>
//#include <DHT.h>
#include <DHT_U.h>

//#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#define BOTtoken " Bot Token (Get from Botfather)"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "CHATID"



const char* ssid = "ID";
const char* password = "PSWD";
const char* serverAddress = "ADDRESS";

#define DHTTYPE DHT11
#define DHTPIN 4

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
const int alcoholSensorPin = 34;  // Use pin 22 for the alcohol sensor
const int potpin = 35;
const int accpin = 33;

float mq135 = 0;
float temperature = 0;

float pot = 0;
float acc = 0;
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
const int buttonPin = 22; // Push button pin

const int ledPin = 2;
bool ledState = LOW;
String accs = "Normal";

File dataFile;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP); // Set pin 22 as input with internal pull-up resistor

  delay(100);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  dht.begin();


#ifdef ESP8266
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
#endif

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  // Connect to Wi-Fi

#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
#endif

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void loop() {
  delay(delayMS);



  // Check if the button is pressed
  if (acc <= 2000 || acc > 2150) {
    bot.sendMessage(CHAT_ID, "EMERGENCY ALERT \n https://www.google.com/maps?q=9.080622950730717,76.86722573089912", "");
    Serial.println("Tilt detected");
    accs = "Tilt detected";
  }
  else {
    accs = "Normal";
  }
  if (digitalRead(buttonPin) == HIGH) {
    bot.sendMessage(CHAT_ID, "EMERGENCY ALERT \n https://www.google.com/maps?q=9.080622950730717,76.86722573089912", "");
    Serial.println("response send");
    if (millis() > lastTimeBotRan + botRequestDelay)  {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
    }
  }

  // Handle Telegram messages
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temperature = event.temperature;

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  float humidity = event.relative_humidity;

  // Get current timestamp
  unsigned long currentMillis = millis();
  mq135 = analogRead(alcoholSensorPin);
  pot = analogRead(potpin);
  acc = analogRead(accpin);


  // Open the file in append mode
  dataFile = SD.open("/data.csv", FILE_APPEND);
  if (dataFile) {
    // Write data to the file in CSV format
    dataFile.print(currentMillis);  // Timestamp
    dataFile.print(",");
    dataFile.print(temperature);  // Temperature
    dataFile.print(",");
    dataFile.println(humidity);  // Humidity
    dataFile.print(",");
    dataFile.println(mq135);  //alcohole
    dataFile.print(",");
    dataFile.println(pot);  // speed
    dataFile.close();
  } else {
    Serial.println("Error opening data.csv");
  }

  // Print sensor readings to serial monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("alcohole: ");
  Serial.println(mq135);
  Serial.print("speed: ");
  Serial.println(pot);
  Serial.print("Accelerator: ");
  Serial.println(acc);

  uploaddata();
}

void uploaddata() {
  float speed = pot;        // Replace with your speed data
  float temp = temperature;  // Replace with your temperature data
  float pressure = 2.3;  // Replace with your pressure data
  float alcohol = mq135;      // Replace with your alcohol data
//  int tractionControl = 1;   // Replace with your traction control data (1 or 0)
  int tractionControl=random(2);



  String payload = "speed=" + String(speed) + "&temp=" +String(temp)+ "&pressure=" +String(pressure)+ "&alcohol=" +String(alcohol)+ "&tilt=" +String(accs)+ "&traction_control=" +String(tractionControl);
  Serial.println(payload);
  // Configure client for HTTPS
  client.setInsecure();  // For now, ignore certificate validation, you might want to handle this differently in production

  // Send HTTP POST request
  HTTPClient http;
  http.begin(client, serverAddress);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  // Wait for some time before sending the next request
  delay(5000);  // Adjust as needed
}
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    bot.sendMessage(CHAT_ID, "EMERGENCY ALERT \n https://www.google.com/maps?q=8.489294889016634,76.94708154995395+", "");

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/led_on to turn GPIO ON \n";
      welcome += "/led_off to turn GPIO OFF \n";
      welcome += "/state to request current GPIO state \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/led_on") {
      bot.sendMessage(chat_id, "LED state set to ON", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }

    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }

    if (text == "/state") {
      if (digitalRead(ledPin)) {
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else {
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
    }
  }
}

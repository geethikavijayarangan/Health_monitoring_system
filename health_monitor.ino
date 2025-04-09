#define BLYNK_TEMPLATE_NAME "Health monitor"
#define BLYNK_TEMPLATE_ID "TMPL3T85BE28V"
#define BLYNK_AUTH_TOKEN "Gj6Si666OTIp93F13LTTDFcgkGtDpShh"

#include <ESP8266WebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp8266.h>

#define DHTTYPE DHT11
#define DHTPIN D5         // D5 pin= GPIO pin 14
#define DS18B20 D4        // D4 pin= GPIO pin 2
#define HEARTBEAT_PIN D6  // Analog pin connected to heartbeat sensor
#define REPORTING_PERIOD_MS 1000
#define LCD_POWER D8


float temperature, humidity, BPM, bodytemperature;

/Put your SSID & Password/
const char* ssid = "Sandy T1 pro";  // Enter SSID here
const char* password = "santhosh2121";  // Enter Password here

/* Blynk Auth Token */
char auth[] = "Gj6Si666OTIp93F13LTTDFcgkGtDpShh";  // Replace with your Blynk Auth Token


DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT sensor
OneWire oneWire(DS18B20);
DallasTemperature sensors(&oneWire);

ESP8266WebServer server(80);

// Variables for BPM measurement
volatile int beatCount = 0;
unsigned long lastBeat = 0;
unsigned long beatInterval = 0;

// LCD initialization
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change address to your LCD's I2C address if necessary
int displayState = 0;

void IRAM_ATTR detectBeat() {
  unsigned long currentTime = millis();
  beatInterval = currentTime - lastBeat;
  lastBeat = currentTime;
  if (beatInterval > 300) {  // Minimum time between beats (prevent noise)
    beatCount++;
    BPM = 60000.0 / beatInterval;  // Convert interval to BPM
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(HEARTBEAT_PIN, INPUT);
  pinMode(LCD_POWER, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(HEARTBEAT_PIN), detectBeat, RISING);
  digitalWrite(LCD_POWER, HIGH);
  Serial.println(F("DHTxx test!"));
  dht.begin();
  sensors.begin();
  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Blynk setup
  Blynk.begin(auth, ssid, password);

  server.on("/", handle_OnConnect);
  server.on("/data", handle_Data);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);
}

void loop() {
  Blynk.run();  // Run Blynk
  server.handleClient();
  sensors.requestTemperatures();

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  bodytemperature = sensors.getTempCByIndex(0);

  temperature = t;
  humidity = h;

  Serial.print("Room Temperature: ");
  Serial.print(t);
  Serial.println("°C");

  Serial.print("Room Humidity: ");
  Serial.print(h);
  Serial.println("%");

  Serial.print("BPM: ");
  Serial.println(BPM);

  Serial.print("Body Temperature: ");
  Serial.print(bodytemperature);
  Serial.println("°C");

  // Send data to Blynk
  Blynk.virtualWrite(V0, temperature);       // Room Temperature
  Blynk.virtualWrite(V1, humidity);         // Room Humidity
  Blynk.virtualWrite(V2, bodytemperature);  // Body Temperature
  Blynk.virtualWrite(V3, BPM);              // BPM

  updateLCD();
  delay(REPORTING_PERIOD_MS);
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}

void handle_Data() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"bodytemperature\":" + String(bodytemperature) + ",";
  json += "\"BPM\":" + String(BPM);
  json += "}";
  server.send(200, "application/json", json);
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void updateLCD() {
  lcd.clear();
  switch (displayState) {
    case 0:
      lcd.print("Room Temp: ");
      lcd.print(temperature);
      lcd.print(" C");
      break;
    case 1:
      lcd.print("Humidity: ");
      lcd.print(humidity);
      lcd.print(" %");
      break;
    case 2:
      lcd.print("Body Temp: ");
      lcd.print(bodytemperature);
      lcd.print(" C");
      break;
    case 3:
      lcd.print("Heart Rate: ");
      lcd.print(BPM);
      lcd.print(" BPM");
      break;
  }   
  displayState = (displayState + 1) % 4;  // Cycle through states
}

String SendHTML() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Patient Health Monitoring</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f8f9fa; color: #333; }";
  html += ".container { margin: 20px auto; max-width: 600px; text-align: center; }";
  html += ".card { padding: 20px; margin: 10px 0; background: #fff; border-radius: 10px; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); }";
  html += ".alert { color: red; font-weight: bold; display: none; margin: 10px 0; }"; // Alert style
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Health Monitoring System</h1>";
  html += "<div id='highTempAlert' class='alert'>⚠ ALERT: High Temperature Detected! ⚠</div>";
  html += "<div id='lowTempAlert' class='alert'>⚠ ALERT: Low Temperature Detected! ⚠</div>";
  html += "<div id='highBPMAlert' class='alert'>⚠ ALERT: High Heart Rate Detected! ⚠</div>";
  html += "<div id='lowBPMAlert' class='alert'>⚠ ALERT: Low Heart Rate Detected! ⚠</div>";
  html += "<div class='card' id='temperatureCard'><h2>Room Temperature: Loading... °C</h2></div>";
  html += "<div class='card' id='humidityCard'><h2>Room Humidity: Loading... %</h2></div>";
  html += "<div class='card' id='bodyTempCard'><h2>Body Temperature: Loading... °C</h2></div>";
  html += "<div class='card' id='bpmCard'><h2>Heart Rate: Loading... BPM</h2></div>";
  html += "<canvas id='heartbeatChart' width='400' height='200'></canvas>";
  html += "</div>";
  html += "<script>";
  
  html += "let bpmData = []; let labels = []; let maxPoints = 20;";
  html += "const ctx = document.getElementById('heartbeatChart').getContext('2d');";
  html += "const chart = new Chart(ctx, {";
  html += "  type: 'line',";
  html += "  data: {";
  html += "    labels: labels,";
  html += "    datasets: [{";
  html += "      label: 'Heart Rate (BPM)',";
  html += "      data: bpmData,";
  html += "      borderColor: 'red',";
  html += "      fill: false";
  html += "    }]";
  html += "  },";
  html += "  options: {";
  html += "    scales: {";
  html += "      x: { display: true },";
  html += "      y: { beginAtZero: true }";
  html += "    }";
  html += "  }";
  html += "});";

  html += "function updateGraph(newBPM) {";
  html += "  if (bpmData.length >= maxPoints) {";
  html += "    bpmData.shift();";
  html += "    labels.shift();";
  html += "  }";
  html += "  bpmData.push(newBPM);";
  html += "  labels.push(new Date().toLocaleTimeString());";
  html += "  chart.update();";
  html += "}";

  html += "function updateCards(data) {";
  html += "  document.getElementById('temperatureCard').innerHTML = <h2>Room Temperature: ${data.temperature} °C</h2>;";
  html += "  document.getElementById('humidityCard').innerHTML = <h2>Room Humidity: ${data.humidity} %</h2>;";
  html += "  document.getElementById('bodyTempCard').innerHTML = <h2>Body Temperature: ${data.bodytemperature} °C</h2>;";
  html += "  document.getElementById('bpmCard').innerHTML = <h2>Heart Rate: ${data.BPM} BPM</h2>;";
  html += "}";

  html += "function checkAlerts(data) {";
  html += "  document.getElementById('highTempAlert').style.display = data.bodytemperature > 37.5 ? 'block' : 'none';";
  html += "  document.getElementById('lowTempAlert').style.display = data.bodytemperature < 36.0 ? 'block' : 'none';";
  html += "  document.getElementById('highBPMAlert').style.display = data.BPM > 100 ? 'block' : 'none';";
  html += "  document.getElementById('lowBPMAlert').style.display = data.BPM < 60 ? 'block' : 'none';";
  html += "}";

  html += "function fetchData() {";
  html += "  fetch('/data')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      updateGraph(data.BPM);";
  html += "      updateCards(data);";
  html += "      checkAlerts(data);";
  html += "    })";
  html += "    .catch(error => console.error('Error fetching data:', error));";
  html += "}";

  html += "setInterval(fetchData, 1000);"; // Fetch data every second
  html += "</script>";
  html += "</body>";
  html += "</html>";
  return html;
}

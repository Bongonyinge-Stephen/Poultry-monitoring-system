#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 32
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LIGHT_RELAY 25
#define LED_BLUE 12
#define LED_GREEN 14
#define LED_RED 27

const char* ssid = "Jamesoneea";
const char* password = "james@123";

// Web login credentials
String username = "admin";
String password_login = "chosen123";

WebServer server(80);

bool manualMode = false;
bool lightStatus = false;

float lightOnThreshold = 33.0;
float lightOffThreshold = 34.0;

float currentTemp = 0;
float currentHum = 0;

// Login Page HTML
String loginPage = R"rawliteral(
<!DOCTYPE html><html><head><title>Login</title>
<style>
body { font-family: Arial; background: #f0f0f0; display: flex; align-items: center; justify-content: center; height: 100vh; }
form { background: white; padding: 2rem; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.2); }
input { display: block; width: 100%; padding: 0.5rem; margin: 1rem 0; border-radius: 4px; border: 1px solid #ccc; }
button { padding: 0.7rem; width: 100%; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
</style></head><body>
<form action="/dashboard" method="GET">
  <h2>Smart Poultry Login</h2>
  <input name="user" placeholder="Username" required>
  <input name="pass" type="password" placeholder="Password" required>
  <button type="submit">Login</button>
</form>
</body></html>
)rawliteral";

// Dashboard HTML
String getDashboardHTML(float temp, float hum) {
  String page = R"rawliteral(
<!DOCTYPE html><html><head><title>Smart Poultry Dashboard</title>
<style>
body { font-family: Arial; background: #f5f5f5; padding: 1rem; }
h2 { color: #2c3e50; }
.card { background: white; padding: 1rem; margin-bottom: 1rem; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
.status { padding: 0.5rem; font-weight: bold; }
.on { color: green; } .off { color: red; }
a.btn { display: inline-block; padding: 0.5rem 1rem; margin: 0.2rem; border-radius: 4px; text-decoration: none; color: white; }
.primary { background: #007bff; } .secondary { background: #6c757d; } .danger { background: #dc3545; }
</style></head><body>
<h2>Smart Poultry Dashboard</h2>

<div class="card">
  <h3>Sensor Readings</h3>
  <p>Temperature: )rawliteral" + String(temp, 1) + R"rawliteral( C</p>
  <p>Humidity: )rawliteral" + String(hum, 1) + R"rawliteral( %</p>
</div>

<div class="card">
  <h3>Light Control</h3>
  <p>Mode: )rawliteral" + (manualMode ? "Manual" : "Automatic") + R"rawliteral(</p>
  <p>Status: <span class="status )rawliteral" + (lightStatus ? "on" : "off") + R"rawliteral("> )rawliteral" + (lightStatus ? "ON" : "OFF") + R"rawliteral(</span></p>
  <a class="btn primary" href="/toggleLight">Toggle Light Mode</a>
  <a class="btn secondary" href="/manualLightOn">Turn ON Light</a>
  <a class="btn danger" href="/manualLightOff">Turn OFF Light</a>
</div>

<div class="card">
  <h3>Brooding Thresholds</h3>
  <p>Light ON threshold: )rawliteral" + String(lightOnThreshold) + R"rawliteral( C</p>
  <p>Light OFF threshold: )rawliteral" + String(lightOffThreshold) + R"rawliteral( C</p>
  <p><em>Adjust these thresholds in your code as needed.</em></p>
</div>

</body></html>
)rawliteral";
  return page;
}

// Route Handlers
void handleRoot() {
  server.send(200, "text/html", loginPage);
}

void handleDashboard() {
  String user = server.arg("user");
  String pass = server.arg("pass");

  if (user != username || pass != password_login) {
    server.send(401, "text/plain", "Unauthorized Access");
    return;
  }

  currentTemp = dht.readTemperature();
  currentHum = dht.readHumidity();

  if (isnan(currentTemp)) currentTemp = 0;
  if (isnan(currentHum)) currentHum = 0;

  Serial.println("User logged in successfully.");
  Serial.println("Current Temperature: " + String(currentTemp, 1) + " °C");
  Serial.println("Current Humidity: " + String(currentHum, 1) + " %");
  Serial.println("Light Status: " + String(lightStatus ? "ON" : "OFF"));

  server.send(200, "text/html", getDashboardHTML(currentTemp, currentHum));
}

void handleToggleLight() {
  manualMode = !manualMode;
  server.sendHeader("Location", "/dashboard?user=" + username + "&pass=" + password_login);
  server.send(303);
}

void handleManualLightOn() {
  if (manualMode) {
    digitalWrite(LIGHT_RELAY, LOW);
    lightStatus = true;
  }
  server.sendHeader("Location", "/dashboard?user=" + username + "&pass=" + password_login);
  server.send(303);
}

void handleManualLightOff() {
  if (manualMode) {
    digitalWrite(LIGHT_RELAY, HIGH);
    lightStatus = false;
  }
  server.sendHeader("Location", "/dashboard?user=" + username + "&pass=" + password_login);
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(LIGHT_RELAY, OUTPUT);
  digitalWrite(LIGHT_RELAY, HIGH); // Default OFF

  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/dashboard", handleDashboard);
  server.on("/toggleLight", handleToggleLight);
  server.on("/manualLightOn", handleManualLightOn);
  server.on("/manualLightOff", handleManualLightOff);
  server.begin();
}

void loop() {
  server.handleClient();

  currentTemp = dht.readTemperature();
  if (isnan(currentTemp)) currentTemp = 0;

  // LED indicators
  if (currentTemp <= lightOnThreshold) {
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
  } else if (currentTemp >= lightOffThreshold) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_GREEN, LOW);
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, LOW);
  }

  // Auto mode control
  if (!manualMode) {
    if (currentTemp <= lightOnThreshold && !lightStatus) {
      digitalWrite(LIGHT_RELAY, LOW);
      lightStatus = true;
    } else if (currentTemp >= lightOffThreshold && lightStatus) {
      digitalWrite(LIGHT_RELAY, HIGH);
      lightStatus = false;
    }
  }
}

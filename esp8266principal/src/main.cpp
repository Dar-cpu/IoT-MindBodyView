#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <dec.h>

const char* ssid = "Pruebaint1";        // Cambiar según necesite
const char* password = "holaprueba";    // Cambiar según necesite
const char* serverURL = "http://192.168.43.42:3000/data";   // Cambiar según necesite

#define TRIG_PIN 5        // D1 - Trigger HC-SR04
#define ECHO_PIN 4        // D2 - Echo HC-SR04
#define DHT_PIN 12        // D6 - DHT11
#define FLAME_PIN 13      // D7 - Sensor de llama
#define IR_PIN 14         // D5 - Sensor infrarrojo
#define BUTTON_PIN 0      // D3 - Botón
#define BATTERY_PIN A0    // ADC - Nivel de batería

#define MOTOR_PIN1 16     // D0
#define MOTOR_PIN2 2      // D4
#define MOTOR_PIN3 15     // D8
#define MOTOR_PIN4 3      // RX   - No tenía otro pin, tenía errores 

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Variables globales
struct SensorData {
  float trashLevel;
  float temperature;
  float humidity;
  bool flameDetected;
  float batteryLevel;
  int userTokens;
  int dailyDeposits;
  bool windowOpen;
};

SensorData currentData;
WiFiClient client;
HTTPClient http;

// Variables de control
bool lastButtonState = HIGH;
bool windowIsOpen = false;
unsigned long lastSensorRead = 0;
unsigned long lastWebSend = 0;
unsigned long lastSerialSend = 0;
unsigned long windowOpenTime = 0;
bool lastIRState = HIGH;
bool wifiConnected = false;

// Variables del motor 
int currentStep = 0;  
bool motorRunning = false;
int targetSteps = 0;
int stepsTaken = 0;
unsigned long lastStepTime = 0;
const long stepInterval = 3; 
bool clockwise = true;

// Secuencia de pasos 
int stepSequence[4][4] = {
  {HIGH, LOW, LOW, LOW},   
  {LOW, HIGH, LOW, LOW},   
  {LOW, LOW, HIGH, LOW},   
  {LOW, LOW, LOW, HIGH}    
};

const unsigned long SENSOR_INTERVAL = 3000;     // 3s
const unsigned long WEB_INTERVAL = 10000;       // 10s
const unsigned long SERIAL_INTERVAL = 2000;     // 2s
const unsigned long WINDOW_TIMEOUT = 10000;       //10s

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println();
  Serial.println("=== INICIANDO SISTEMA ===");
  
  // Pines de sensores y boton
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(TRIG_PIN, LOW);
  yield(); 

  // Pines del motor 
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_PIN3, OUTPUT);
  pinMode(MOTOR_PIN4, OUTPUT);
  yield(); 

  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  digitalWrite(MOTOR_PIN3, LOW);
  digitalWrite(MOTOR_PIN4, LOW);
  currentStep = 0;
  yield(); 

  //valores por defecto
  currentData.userTokens = 0;
  currentData.dailyDeposits = 0;
  currentData.windowOpen = false;
  currentData.trashLevel = 50.0;
  currentData.temperature = 25.0;
  currentData.humidity = 60.0;
  currentData.flameDetected = false;
  currentData.batteryLevel = 100.0;
  
  Serial.println("Inicializando DHT11...");
  dht.begin();
  delay(1000);
  yield(); 

  setupWiFi();
  
  Serial.println("Sistema listo!");
  Serial.println(" Porfa un 20 :) ");  // Era para mi calificación xd 
  Serial.println("==================="); 
  yield(); 
}

void setupWiFi() {
  Serial.println("Configurando WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(1000);
    Serial.print(".");
    attempts++;
    yield(); 
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    yield();
    Serial.println();
    Serial.println("WiFi no conectado - funcionando sin red");
    wifiConnected = false;
  } 
  yield(); 
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {    // Leer sensores
    readSensors();
    lastSensorRead = currentTime;
  }
  checkButton();     
  checkTrashDeposit();
  stepMotor();
 
  if (wifiConnected && currentTime - lastWebSend >= WEB_INTERVAL) {  // Comunicaciones
    sendDataToWeb();
    lastWebSend = currentTime;
  }
  if (currentTime - lastSerialSend >= SERIAL_INTERVAL) {  //Serial
    sendDataToSerial();
    lastSerialSend = currentTime;
  }

  checkCriticalAlerts();     // Verificar alertas
  checkWiFiStatus();     
  
  yield();
  delay(50);
}

void stepMotor() {
  if (!motorRunning) return;
  if (clockwise) {
    currentStep = (currentStep + 1) % 4;
  } else { 
    currentStep = (currentStep - 1 + 4) % 4;
  }
  digitalWrite(MOTOR_PIN1, stepSequence[currentStep][0]);
  digitalWrite(MOTOR_PIN2, stepSequence[currentStep][1]);
  digitalWrite(MOTOR_PIN3, stepSequence[currentStep][2]);
  digitalWrite(MOTOR_PIN4, stepSequence[currentStep][3]);
  delayMicroseconds(1); 
  stepsTaken++;
  if (stepsTaken >= targetSteps) {
    motorRunning = false;
    stepsTaken = 0;
    Serial.println("Motor detenido");
    if (windowIsOpen) {
      closeWindow();
    }
  digitalWrite(MOTOR_PIN1, LOW);   // Apagar el motor 
  digitalWrite(MOTOR_PIN2, LOW);
  digitalWrite(MOTOR_PIN3, LOW);
  digitalWrite(MOTOR_PIN4, LOW);
  }
 
}

void readSensors() {
  float newTrashLevel = readUltrasonicSensor();
  if (newTrashLevel >= 0) {
    currentData.trashLevel = newTrashLevel;
  }
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp) && temp > -10 && temp < 60) {
    currentData.temperature = temp;
  }
  if (!isnan(hum) && hum > 0 && hum <= 100) {
    currentData.humidity = hum;
  }
  currentData.flameDetected = (digitalRead(FLAME_PIN) == LOW);
  currentData.batteryLevel = readBatteryLevel();
}

float readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1; // Timeout
  }
  
  float distance = (duration * 0.034) / 2;
  if (distance < 2 || distance > 200) {
    return -1;
  }
  float level = map(distance, 5, 33, 100, 0);
  return constrain(level, 0, 100);
}

float readBatteryLevel() {
  int reading = analogRead(BATTERY_PIN);
  float voltage = (reading / 1024.0) * 3.3;
  float batteryVoltage = voltage * 12.1;
  float percentage = ((batteryVoltage - 10.0) / 2.6) * 100.0;
  return constrain(percentage, 0, 100);
}

void checkButton() {
  bool currentState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentState == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (!windowIsOpen) {
        openWindow();
      }
    }
  }
  lastButtonState = currentState;
}

void openWindow() {
  Serial.println("Abriendo ventana...");
  clockwise = true;
  targetSteps = 512;  
  stepsTaken = 0;
  motorRunning = true;
  windowIsOpen = true;
  currentData.windowOpen = true;
  Serial.println("Ventana abierta");
}

void closeWindow() {
  Serial.println("Cerrando ventana...");
  clockwise = false;
  targetSteps = 512;  
  stepsTaken = 0;
  motorRunning = true;
  windowIsOpen = false;
  currentData.windowOpen = false;
  Serial.println("Ventana cerrada");  
}

void checkTrashDeposit() {
  bool currentState = digitalRead(IR_PIN);
  if (windowIsOpen && lastIRState == HIGH && currentState == LOW) {
    delay(50);
    if (digitalRead (IR_PIN == LOW)) {
      currentData.dailyDeposits++;
      currentData.userTokens += 10;
      Serial.println("¡Depósito detectado! +5 tokens");
      Serial.println("Total tokens: " + String(currentData.userTokens));
    }
  }
  lastIRState = currentState;
}

void checkWiFiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      Serial.println("WiFi reconectado!");
      wifiConnected = true;
    }
  } else {
    if (wifiConnected) {
      Serial.println("WiFi desconectado");
      wifiConnected = false;
    }
  }
}

void sendDataToWeb() {
  if (!wifiConnected) return;
  http.setTimeout(3000);
  http.begin(client, serverURL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"type\":\"data\",";
  json += "\"trash\":" + String((int)currentData.trashLevel) + ",";
  json += "\"temp\":" + String((int)currentData.temperature) + ",";
  json += "\"hum\":" + String((int)currentData.humidity) + ",";
  json += "\"flame\":" + String(currentData.flameDetected ? "true" : "false") + ",";
  json += "\"bat\":" + String((int)currentData.batteryLevel) + ",";
  json += "\"tokens\":" + String(currentData.userTokens) + ",";
  json += "\"deps\":" + String(currentData.dailyDeposits) + ",";
  json += "\"win\":" + String(currentData.windowOpen ? "true" : "false") + ",";
  json += "\"button\":" + String(BUTTON_PIN ? "true" : "false") + ",";  
  json += "\"time\":" + String(millis() / 1000);
  json += "}";
  
  int code = http.POST(json);
  
  if (code > 0) {
    Serial.println("Web OK: " + String(code));
  } else {
    Serial.println("Web Error: " + String(code));
  }
  
  http.end();
}

void sendDataToSerial() {
  String json = "{";
  json += "\"type\":\"status\",";
  json += "\"trash\":" + String(currentData.trashLevel, 1) + ",";
  json += "\"temp\":" + String(currentData.temperature, 1) + ",";
  json += "\"hum\":" + String(currentData.humidity, 1) + ",";
  json += "\"flame\":" + String(currentData.flameDetected ? "true" : "false") + ",";
  json += "\"bat\":" + String(currentData.batteryLevel, 1) + ",";
  json += "\"tokens\":" + String(currentData.userTokens) + ",";
  json += "\"deps\":" + String(currentData.dailyDeposits) + ",";
  json += "\"win\":" + String(currentData.windowOpen ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"wifi\":" + String(wifiConnected ? "true" : "false");
  json += "}";
  
  Serial.println(json);
}

void checkCriticalAlerts() {
  if (currentData.flameDetected) {
    Serial.println("¡ALERTA: FUEGO DETECTADO (" + String((int)currentData.flameDetected) + "%)!");
  }

  if (currentData.batteryLevel < 20) {
    Serial.println("¡ALERTA: BATERÍA BAJA (" + String((int)currentData.batteryLevel) + "%)!");
  }
  
  if (currentData.trashLevel > 85) {
    Serial.println("¡ALERTA: CONTENEDOR LLENO (" + String((int)currentData.trashLevel) + "%)!");
  }
}
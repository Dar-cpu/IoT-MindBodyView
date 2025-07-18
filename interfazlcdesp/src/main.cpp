#include <WiFi.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>  

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

#define PIN_TX 1
#define PIN_RX 3

#define LED_RED   4  
#define LED_GREEN 16
#define LED_BLUE  17

#define BLACK   0x0000
#define WHITE   0xFFFF
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define RED     0xF800
#define BLUE    0x001F
#define CYAN    0x07FF

struct SensorData {
  float trashLevel = 0;
  float temperature = 0;
  float humidity = 0;
  bool flameDetected = false;
  float batteryLevel = 100;
  int userTokens = 0;
  int dailyDeposits = 0;
  bool connected = false;
};

struct Button {
  int x, y, w, h;
  String label;
  uint16_t color;
  bool pressed;
  bool justPressed;
  bool justReleased;
};

SensorData data;
String inputBuffer = "";
unsigned long lastUpdate = 0;
unsigned long lastBlink = 0;
unsigned long lastTouch = 0;
bool blinkState = false;
bool needsRedraw = true;
bool touchPressed = false;
bool lastTouchState = false;

// Botones 
Button buttons[3] = {
  {10, 200, 90, 30, "STATS", BLUE, false, false, false},
  {110, 200, 90, 30, "CONFIG", YELLOW, false, false, false},
  {210, 200, 90, 30, "REFRESH", GREEN, false, false, false}
};

Button backButton = {250, 200, 60, 30, "VOLVER", GREEN, false, false, false};

int currentScreen = 0; // 0=Main, 1=Stats, 2=Config

// Funciones 
void readSerial();
void parseData(String jsonData);
void sendCommand(String command);
void handleTouch();
void updateDisplay();
void updateLEDs();
void setLED(int r, int g, int b);
bool isPointInButton(int x, int y, Button &btn);

void showStartup();
void showMainScreen();
void showStatsScreen();
void showConfigScreen();

void drawBattery();
void drawTrashLevel();
void drawAlerts();
void drawButtons();
void drawButton(Button &btn);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX); 

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  setLED(0, 0, 1); 

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  ts.begin();

  for (int i = 0; i < 3; i++) {
    buttons[i].pressed = false;
    buttons[i].justPressed = false;
    buttons[i].justReleased = false;
  }
  backButton.pressed = false;
  backButton.justPressed = false;
  backButton.justReleased = false;

  showStartup();
  delay(2000);
 
  // Datos de prueba
  data.trashLevel = 25.5;
  data.temperature = 23.2;
  data.humidity = 65.0;
  data.batteryLevel = 85.0;
  data.userTokens = 150;
  data.dailyDeposits = 5;
  data.connected = true;
  
  Serial.println("Sistema iniciado");
}

void loop() {
  readSerial();
  handleTouch();
  updateLEDs();

  if (millis() - lastUpdate > 500) {
    updateDisplay();
    lastUpdate = millis();
  }

  if (millis() - lastBlink > 1000) {
    blinkState = !blinkState;
    lastBlink = millis();
    if (data.flameDetected) needsRedraw = true;
  }
  
  delay(50);
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (inputBuffer.length() > 0) {
        parseData(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
  
  // Leer desde UART
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    if (message.length() > 0) {
      parseData(message);
    }
  }
}

void parseData(String jsonData) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("Error JSON: " + String(error.c_str()));
    return;
  }

  data.trashLevel = doc["trashLevel"] | data.trashLevel;
  data.temperature = doc["temperature"] | data.temperature;
  data.humidity = doc["humidity"] | data.humidity;
  data.flameDetected = doc["flameDetected"] | data.flameDetected;
  data.batteryLevel = doc["batteryLevel"] | data.batteryLevel;
  data.userTokens = doc["userTokens"] | data.userTokens;
  data.dailyDeposits = doc["dailyDeposits"] | data.dailyDeposits;
  data.connected = true;
  
  needsRedraw = true;
  Serial.println("Datos actualizados");
}

void sendCommand(String command) {
  JsonDocument doc;
  doc["command"] = command;
  doc["timestamp"] = millis();
  
  String output;
  serializeJson(doc, output);
  Serial2.println(output);
  Serial.println("Enviado: " + output);
}

void handleTouch() {
  TouchPoint p = ts.getTouch();

  touchPressed = (p.zRaw > 0);

  bool touchJustPressed = touchPressed && !lastTouchState;
  bool touchJustReleased = !touchPressed && lastTouchState;
  
  if (touchJustPressed) {
    Serial.printf("Touch detectado en: x=%d, y=%d\n", p.x, p.y);
    
    if (currentScreen == 0) {
      for (int i = 0; i < 3; i++) {
        if (isPointInButton(p.x, p.y, buttons[i])) {
          buttons[i].pressed = true;
          buttons[i].justPressed = true;
          needsRedraw = true;
          Serial.printf("Botón %d presionado\n", i);
          break;
        }
      }
    } else {
      // Pantallas secundarias - revisar botón de retorno
      if (isPointInButton(p.x, p.y, backButton)) {
        backButton.pressed = true;
        backButton.justPressed = true;
        needsRedraw = true;
        Serial.println("Botón volver presionado");
      }
    }
  }
  
  if (touchJustReleased) {
    Serial.println("Touch liberado");
    
    if (currentScreen == 0) {
      // Liberación de botones principales
      for (int i = 0; i < 3; i++) {
        if (buttons[i].pressed) {
          buttons[i].pressed = false;
          buttons[i].justReleased = true;
          
          switch (i) {
            case 0: 
              currentScreen = 1; 
              Serial.println("Cambiando a pantalla Stats");
              break;
            case 1: 
              currentScreen = 2; 
              Serial.println("Cambiando a pantalla Config");
              break;
            case 2: 
              sendCommand("refresh"); 
              Serial.println("Enviando comando refresh");
              break;
          }
          needsRedraw = true;
          break;
        }
      }
    } else {
      // Liberación del botón de retorno
      if (backButton.pressed) {
        backButton.pressed = false;
        backButton.justReleased = true;
        currentScreen = 0;
        needsRedraw = true;
        Serial.println("Regresando a pantalla principal");
      }
    }
  }

  lastTouchState = touchPressed;
  for (int i = 0; i < 3; i++) {
    buttons[i].justPressed = false;
    buttons[i].justReleased = false;
  }
  backButton.justPressed = false;
  backButton.justReleased = false;
}

bool isPointInButton(int x, int y, Button &btn) {
  return (x >= btn.x && x <= btn.x + btn.w && 
          y >= btn.y && y <= btn.y + btn.h);
}

void updateDisplay() {
  if (!needsRedraw) return;
  
  switch (currentScreen) {
    case 0: showMainScreen(); break;
    case 1: showStatsScreen(); break;
    case 2: showConfigScreen(); break;
  }
  
  needsRedraw = false;
}

void showStartup() {
  tft.fillScreen(BLACK);
  tft.setTextColor(GREEN);
  tft.setTextSize(3);
  tft.drawString("EcoSmart", 80, 60);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.drawString("Container", 100, 100);
  tft.setTextSize(1);
  tft.drawString("Iniciando...", 120, 140);
}

void showMainScreen() {
  tft.fillScreen(BLACK);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.drawString("Contenedor ....", 20, 5);
  drawBattery();
  drawTrashLevel();
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.drawString("Temperatura: " + String(data.temperature, 1) + " C", 10, 100);
  tft.drawString("Humedad: " + String(data.humidity, 1) + " %", 10, 115);
  tft.setTextColor(YELLOW);
  tft.drawString("Tokens: " + String(data.userTokens), 10, 135);
  tft.drawString("Depositos hoy: " + String(data.dailyDeposits), 10, 150);
  drawAlerts();

  tft.setTextColor(data.connected ? GREEN : RED);
  tft.drawString(data.connected ? "Conectado" : "Desconectado", 220, 180);
  drawButtons();
}

void showStatsScreen() {
  tft.fillScreen(BLACK);
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.drawString("ESTADISTICAS", 80, 10);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  int y = 50;
  tft.drawString("Nivel actual: " + String(data.trashLevel, 1) + " %", 10, y);
  y += 20;
  tft.drawString("Temperatura: " + String(data.temperature, 1) + " C", 10, y);
  y += 20;
  tft.drawString("Humedad: " + String(data.humidity, 1) + "%", 10, y);
  y += 20;
  tft.drawString("Tokens ganados: " + String(data.userTokens), 10, y);
  y += 20;
  tft.drawString("Depositos realizados: " + String(data.dailyDeposits), 10, y);

  drawButton(backButton);
}

void showConfigScreen() {
  tft.fillScreen(BLACK);
  
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.drawString("CONFIGURACION", 60, 10);
  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  
  int y = 50;
  tft.drawString("Sistema: Operativo", 10, y);
  y += 20;
  tft.drawString("Conexion: " + String(data.connected ? "Activa" : "Inactiva"), 10, y);
  y += 20;
  tft.drawString("Bateria: " + String(data.batteryLevel, 1) + " %", 10, y);
  y += 20;
  tft.drawString("Memoria libre: OK", 10, y);

  drawButton(backButton);
}

void drawBattery() {
  int x = 275, y = 5;
  tft.drawRect(x, y, 40, 15, WHITE);
  tft.drawRect(x + 40, y + 4, 3, 7, WHITE);
  
  int fill = (data.batteryLevel / 100.0) * 38;
  uint16_t color = data.batteryLevel > 30 ? GREEN : 
                   data.batteryLevel > 15 ? YELLOW : RED;
  tft.fillRect(x + 1, y + 1, fill, 13, color);
}

void drawTrashLevel() {
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.drawString("Nivel de Basura:", 10, 40);
  
  // Barra de progreso
  int barX = 10, barY = 55, barW = 200, barH = 20;
  tft.drawRect(barX, barY, barW, barH, WHITE);
  
  int fill = (data.trashLevel / 100.0) * (barW - 2);
  uint16_t color = data.trashLevel > 80 ? RED :
                   data.trashLevel > 60 ? YELLOW : GREEN;
  tft.fillRect(barX + 1, barY + 1, fill, barH - 2, color);
  
  // Porcentaje
  tft.setTextColor(WHITE);
  tft.drawString(String(data.trashLevel, 1) + "%", barX + barW + 10, barY + 6);
}

void drawAlerts() {
  int y = 170;
  
  if (data.flameDetected && blinkState) {
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.drawString("⚠ FUEGO DETECTADO!", 10, y);
  } else if (data.trashLevel > 85) {
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.drawString("⚠ Contenedor lleno", 10, y);
  } else {
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.drawString("✓ Sistema OK", 10, y);
  }
}

void drawButtons() {
  for (int i = 0; i < 3; i++) {
    drawButton(buttons[i]);
  }
}

void drawButton(Button &btn) {
  uint16_t color = btn.pressed ? WHITE : btn.color;
  
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, color);
  tft.setTextColor(color);
  tft.setTextSize(1);
  
  int textX = btn.x + (btn.w - btn.label.length() * 6) / 2;
  int textY = btn.y + (btn.h - 8) / 2;
  tft.drawString(btn.label, textX, textY);
}

void setLED(int r, int g, int b) {
  digitalWrite(LED_RED, r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE, b ? LOW : HIGH);
}

void updateLEDs() {
  if (data.flameDetected) {
    setLED(blinkState ? 1 : 0, 0, 0); // Rojo parpadeante
  } else if (data.trashLevel > 85) {
    setLED(1, 0, 0); // Rojo fijo
  } else if (data.connected) {
    setLED(0, 1, 0); // Verde
  } else {
    setLED(0, 0, 1); // Azul
  }
}
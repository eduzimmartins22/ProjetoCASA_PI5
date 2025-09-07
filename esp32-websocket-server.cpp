https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// Configurações WiFi
const char* ssid = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";

// Configurações dos LEDs
#define NUM_LEDS 10
#define LED_PIN 2
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Configurações do Jato d'Água
#define WATER_PUMP_PIN 4
#define WATER_SENSOR_PIN 34

// Servidores
WebSocketsServer webSocket = WebSocketsServer(8080);
WebServer httpServer(80);

// Array de LEDs
CRGB leds[NUM_LEDS];

// Reutilizar as estruturas do código anterior
struct LEDController {
    bool isOn = false;
    CRGB colors[NUM_LEDS];
    uint8_t brightness = 150;
    
    void initialize() {
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
        FastLED.setBrightness(brightness);
        
        for(int i = 0; i < NUM_LEDS; i++) {
            colors[i] = CRGB::Black;
            leds[i] = CRGB::Black;
        }
        FastLED.show();
    }
    
    void toggleLEDs(bool state) {
        isOn = state;
        if (!isOn) {
            for(int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Black;
            }
        } else {
            for(int i = 0; i < NUM_LEDS; i++) {
                leds[i] = colors[i];
            }
        }
        FastLED.show();
    }
    
    void setLEDColor(int position, uint32_t hexColor) {
        if (position >= 0 && position < NUM_LEDS) {
            uint8_t r = (hexColor >> 16) & 0xFF;
            uint8_t g = (hexColor >> 8) & 0xFF;
            uint8_t b = hexColor & 0xFF;
            
            colors[position] = CRGB(r, g, b);
            
            if (isOn) {
                leds[position] = colors[position];
                FastLED.show();
            }
        }
    }
    
    void setAllLEDsColor(uint32_t hexColor) {
        for(int i = 0; i < NUM_LEDS; i++) {
            setLEDColor(i, hexColor);
        }
    }
};

struct WaterController {
    bool isOn = false;
    int level = 50;
    int pwmValue = 127;
    
    void initialize() {
        pinMode(WATER_PUMP_PIN, OUTPUT);
        pinMode(WATER_SENSOR_PIN, INPUT);
        analogWrite(WATER_PUMP_PIN, 0);
    }
    
    void togglePump(bool state) {
        isOn = state;
        if (isOn) {
            updatePumpSpeed();
        } else {
            analogWrite(WATER_PUMP_PIN, 0);
        }
    }
    
    void setLevel(int newLevel) {
        level = constrain(newLevel, 0, 100);
        pwmValue = map(level, 0, 100, 0, 255);
        
        if (isOn) {
            updatePumpSpeed();
        }
    }
    
    void updatePumpSpeed() {
        analogWrite(WATER_PUMP_PIN, pwmValue);
    }
    
    int readWaterSensor() {
        return analogRead(WATER_SENSOR_PIN);
    }
    
    void emergencyStop() {
        isOn = false;
        analogWrite(WATER_PUMP_PIN, 0);
    }
};

// Instâncias dos controladores
LEDController ledController;
WaterController waterController;

// Converter string hexadecimal para uint32_t
uint32_t hexStringToUint32(String hexString) {
    if (hexString.startsWith("#")) {
        hexString = hexString.substring(1);
    }
    return strtoul(hexString.c_str(), NULL, 16);
}

// Processar comandos JSON
String processCommand(String jsonString) {
    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        return "Erro ao processar JSON: " + String(error.c_str());
    }
    
    String command = doc["command"];
    String response = "OK";
    
    if (command == "LED_TOGGLE") {
        bool state = doc["payload"]["state"];
        ledController.toggleLEDs(state);
        response = "LEDs " + String(state ? "ligados" : "desligados");
        
    } else if (command == "LED_COLOR") {
        String colorHex = doc["payload"]["color"];
        uint32_t color = hexStringToUint32(colorHex);
        ledController.setAllLEDsColor(color);
        response = "Cor dos LEDs alterada para: " + colorHex;
        
    } else if (command == "LED_SET_POSITION") {
        int position = doc["payload"]["position"];
        String colorHex = doc["payload"]["color"];
        uint32_t color = hexStringToUint32(colorHex);
        ledController.setLEDColor(position, color);
        response = "LED " + String(position) + " configurado com cor: " + colorHex;
        
    } else if (command == "WATER_TOGGLE") {
        bool state = doc["payload"]["state"];
        waterController.togglePump(state);
        response = "Bomba d'água " + String(state ? "ligada" : "desligada");
        
    } else if (command == "WATER_INCREASE" || command == "WATER_DECREASE") {
        int level = doc["payload"]["level"];
        waterController.setLevel(level);
        response = "Nível da bomba ajustado para: " + String(level) + "%";
        
    } else {
        response = "Comando não reconhecido: " + command;
    }
    
    // Enviar status atualizado para todos os clientes WebSocket
    broadcastStatus();
    
    return response;
}

// Gerar JSON de status
String generateStatusJson() {
    StaticJsonDocument<400> doc;
    
    doc["timestamp"] = millis();
    doc["led_status"] = ledController.isOn;
    doc["water_status"] = waterController.isOn;
    doc["water_level"] = waterController.level;
    doc["water_sensor_reading"] = waterController.readWaterSensor();
    
    JsonArray ledArray = doc.createNestedArray("led_colors");
    for(int i = 0; i < NUM_LEDS; i++) {
        JsonObject ledObj = ledArray.createNestedObject();
        ledObj["position"] = i;
        ledObj["r"] = ledController.colors[i].red;
        ledObj["g"] = ledController.colors[i].green;
        ledObj["b"] = ledController.colors[i].blue;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return "STATUS:" + jsonString;
}

// Broadcast status para todos os clientes WebSocket
void broadcastStatus() {
    String status = generateStatusJson();
    webSocket.broadcastTXT(status);
}

// === CONFIGURAÇÃO WEBSOCKET ===
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Desconectado!\n", num);
            break;
            
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Conectado de %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            
            // Enviar status atual para o novo cliente
            webSocket.sendTXT(num, generateStatusJson());
            break;
        }
        
        case WStype_TEXT: {
            String message = String((char*)payload);
            Serial.printf("[%u] Recebido: %s\n", num, message.c_str());
            
            String response = processCommand(message);
            webSocket.sendTXT(num, response);
            break;
        }
        
        default:
            break;
    }
}

// === CONFIGURAÇÃO HTTP ===
void handleCORS() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
    handleCORS();
    httpServer.send(200, "text/plain", "OK");
}

void handleCommand() {
    handleCORS();
    
    if (httpServer.method() == HTTP_POST) {
        String body = httpServer.arg("plain");
        String response = processCommand(body);
        httpServer.send(200, "application/json", response);
    } else {
        httpServer.send(405, "text/plain", "Method Not Allowed");
    }
}

void handleStatus() {
    handleCORS();
    String status = generateStatusJson();
    httpServer.send(200, "application/json", status);
}

void handleRoot() {
    handleCORS();
    String html = R"(
    <html>
    <head><title>Arduino Controller</title></head>
    <body>
        <h1>Arduino Controller API</h1>
        <p>WebSocket: ws://)" + WiFi.localIP().toString() + R"(:8080</p>
        <p>HTTP API:</p>
        <ul>
            <li>POST /api/command - Enviar comandos</li>
            <li>GET /api/status - Obter status</li>
        </ul>
        <p>Status: Sistema funcionando!</p>
    </body>
    </html>
    )";
    httpServer.send(200, "text/html", html);
}

// === SETUP ===
void setup() {
    Serial.begin(115200);
    
    // Inicializar controladores
    ledController.initialize();
    waterController.initialize();
    
    // Conectar WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
    
    // Configurar WebSocket
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    Serial.println("Servidor WebSocket iniciado na porta 8080");
    
    // Configurar rotas HTTP
    httpServer.on("/", handleRoot);
    httpServer.on("/api/command", HTTP_OPTIONS, handleOptions);
    httpServer.on("/api/command", HTTP_POST, handleCommand);
    httpServer.on("/api/status", HTTP_GET, handleStatus);
    httpServer.begin();
    Serial.println("Servidor HTTP iniciado na porta 80");
    
    // Teste inicial
    Serial.println("Sistema iniciado!");
    Serial.println("Comandos aceitos via WebSocket e HTTP:");
    Serial.println("- LED_TOGGLE, LED_COLOR, LED_SET_POSITION");
    Serial.println("- WATER_TOGGLE, WATER_INCREASE, WATER_DECREASE");
    
    // Enviar status inicial
    delay(1000);
    broadcastStatus();
}

// === LOOP ===
void loop() {
    // Processar WebSocket e HTTP
    webSocket.loop();
    httpServer.handleClient();
    
    // Verificar comandos via Serial (para debug)
    if (Serial.available()) {
        String receivedData = Serial.readStringUntil('\n');
        receivedData.trim();
        
        if (receivedData.length() > 0) {
            String response = processCommand(receivedData);
            Serial.println(response);
        }
    }
    
    // Monitoramento de segurança
    static unsigned long lastSensorCheck = 0;
    if (millis() - lastSensorCheck > 1000) {
        int waterSensorValue = waterController.readWaterSensor();
        if (waterSensorValue > 3000 && waterController.isOn) {
            waterController.emergencyStop();
            Serial.println("EMERGÊNCIA: Bomba parada por nível alto de água!");
            broadcastStatus();
        }
        lastSensorCheck = millis();
    }
    
    // Broadcast status periodicamente
    static unsigned long lastStatusBroadcast = 0;
    if (millis() - lastStatusBroadcast > 10000) {
        broadcastStatus();
        lastStatusBroadcast = millis();
    }
    
    delay(10);
}
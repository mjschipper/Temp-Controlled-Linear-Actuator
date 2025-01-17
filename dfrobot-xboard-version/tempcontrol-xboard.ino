/**
 * XBoard V2 Temperature Control System with Web Interface
 * 
 * Hardware:
 * - DFRobot XBoard V2 (ATmega328P + WIZ5100)
 * - SHT31d Temperature Sensor
 * - L9110S H-Bridge Motor Driver
 * - 2x 4-pin PWM Fans
 * - 12V Linear Actuator
 * 
 * Pin Mapping for XBoard V2:
 * Digital Pins (PWM marked with *):
 * - D3* → Fan Exhaust PWM
 * - D5* → Fan Intake PWM
 * - D6* → Actuator Extend
 * - D9* → Actuator Retract
 * 
 * Analog Pins (Used as Digital):
 * - A4/SDA → SHT31d SDA
 * - A5/SCL → SHT31d SCL
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <SHT31.h>
#include <ArduinoJson.h>  // Make sure to install ArduinoJson library

// Pin definitions for XBoard
const uint8_t PIN_ACTUATOR_EXTEND = 6;
const uint8_t PIN_ACTUATOR_RETRACT = 9;
const uint8_t PIN_FAN_EXHAUST = 3;
const uint8_t PIN_FAN_INTAKE = 5;
const uint8_t SHT31_ADDRESS = 0x44;

// Temperature thresholds (Celsius)
const float TEMP_OPEN_THRESHOLD = 29.0;  // Adjusted for bottom sensor
const float TEMP_CLOSE_THRESHOLD = 26.5;
const float FAN_START_TEMP = 25.0;
const float FAN_MAX_TEMP = 30.0;
const float ESTIMATED_TEMP_DIFF = 2.5;  // Estimated temp difference top to bottom

// Fan speeds (PWM values 0-255)
const uint8_t FAN_MIN_SPEED = 0;
const uint8_t FAN_MAX_SPEED = 255;
const uint8_t FAN_MIN_START = 64;      // ~25% minimum speed when active
const float EXHAUST_INTAKE_RATIO = 1.3; // Exhaust runs 30% faster than intake

// Timing constants
const unsigned long ACTUATOR_TIMEOUT = 4500;      // 4.5 seconds for full travel
const unsigned long TEMP_CHECK_INTERVAL = 1000;   // Check temp every second
const unsigned long TEMP_TREND_INTERVAL = 30000;  // Trend analysis every 30 seconds

// Temperature tracking
const int TEMP_HISTORY_SIZE = 10;
float tempHistory[TEMP_HISTORY_SIZE];
int tempHistoryIndex = 0;

// System states
enum ActuatorState {
    IDLE,
    EXTENDING,
    RETRACTING
};

// Network configuration
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);  // Change to your desired IP
EthernetServer server(80);

// System status structure
struct SystemStatus {
    float currentTemp;
    float estimatedTopTemp;
    uint8_t exhaustFanSpeed;
    uint8_t intakeFanSpeed;
    ActuatorState actuatorState;
    unsigned long uptime;
    float tempHistory[TEMP_HISTORY_SIZE];
} status;

// Global variables
SHT31 sht;
ActuatorState currentState = IDLE;
float lastTemperature = (TEMP_OPEN_THRESHOLD + TEMP_CLOSE_THRESHOLD) / 2;
unsigned long actuatorStartTime = 0;

void setupPWM() {
    // Configure Timer2 for 25kHz PWM on pins 3 and 11
    TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS21);  // Prescaler of 8
    
    // Configure Timer0 for 25kHz PWM on pins 5 and 6
    TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01);  // Prescaler of 8
}

void setup() {
    Serial.begin(115200);
    
    // Initialize Ethernet
    Ethernet.begin(mac, ip);
    server.begin();
    Serial.print(F("Server started at "));
    Serial.println(Ethernet.localIP());
    
    // Initialize I2C and sensor
    Wire.begin();
    Wire.setClock(100000);
    
    if (!sht.begin(SHT31_ADDRESS)) {
        Serial.println(F("SHT31 init failed"));
        while (1) { delay(1); }
    }
    
    // Initialize pins
    pinMode(PIN_ACTUATOR_EXTEND, OUTPUT);
    pinMode(PIN_ACTUATOR_RETRACT, OUTPUT);
    pinMode(PIN_FAN_EXHAUST, OUTPUT);
    pinMode(PIN_FAN_INTAKE, OUTPUT);
    
    // Configure PWM
    setupPWM();
    
    // Initialize outputs
    stopActuator();
    analogWrite(PIN_FAN_EXHAUST, 0);
    analogWrite(PIN_FAN_INTAKE, 0);
    
    // Initialize status
    status.currentTemp = readTemperature();
    status.estimatedTopTemp = estimateTopTemperature(status.currentTemp);
    status.exhaustFanSpeed = 0;
    status.intakeFanSpeed = 0;
    status.actuatorState = IDLE;
    status.uptime = 0;
    
    // Initialize temperature history
    for(int i = 0; i < TEMP_HISTORY_SIZE; i++) {
        tempHistory[i] = status.currentTemp;
        status.tempHistory[i] = status.currentTemp;
    }
}

float readTemperature() {
    if (!sht.read()) {
        Serial.println(F("Error reading sensor"));
        return lastTemperature;
    }
    return sht.getTemperature();
}

float estimateTopTemperature(float bottomTemp) {
    return bottomTemp + ESTIMATED_TEMP_DIFF;
}

uint8_t calculateFanSpeed(float bottomTemp) {
    float topTemp = estimateTopTemperature(bottomTemp);
    
    if (topTemp <= FAN_START_TEMP) {
        return FAN_MIN_SPEED;
    }
    if (topTemp >= FAN_MAX_TEMP) {
        return FAN_MAX_SPEED;
    }
    
    // Calculate speed with non-linear curve for better response
    float tempRange = FAN_MAX_TEMP - FAN_START_TEMP;
    float tempAboveStart = topTemp - FAN_START_TEMP;
    float speedFactor = (tempAboveStart / tempRange);
    speedFactor = speedFactor * speedFactor;  // Square for non-linear response
    
    return FAN_MIN_START + (uint8_t)((FAN_MAX_SPEED - FAN_MIN_START) * speedFactor);
}

void updateFanSpeeds(float bottomTemp) {
    uint8_t baseSpeed = calculateFanSpeed(bottomTemp);
    
    // Calculate fan speeds with exhaust running faster
    uint8_t exhaustSpeed = min(255, (uint8_t)(baseSpeed * EXHAUST_INTAKE_RATIO));
    uint8_t intakeSpeed = baseSpeed;
    
    // Update PWM outputs
    analogWrite(PIN_FAN_EXHAUST, exhaustSpeed);
    analogWrite(PIN_FAN_INTAKE, intakeSpeed);
    
    // Update status
    status.exhaustFanSpeed = map(exhaustSpeed, 0, 255, 0, 100);  // Convert to percentage
    status.intakeFanSpeed = map(intakeSpeed, 0, 255, 0, 100);
    
    // Debug output
    Serial.print(F("Fan Speeds - Exhaust: "));
    Serial.print(status.exhaustFanSpeed);
    Serial.print(F("%, Intake: "));
    Serial.print(status.intakeFanSpeed);
    Serial.println(F("%"));
}

void analyzeTempTrend() {
    float avgTemp = 0;
    for(int i = 0; i < TEMP_HISTORY_SIZE; i++) {
        avgTemp += tempHistory[i];
    }
    avgTemp /= TEMP_HISTORY_SIZE;
    
    float tempDelta = tempHistory[(tempHistoryIndex - 1 + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE] - 
                     tempHistory[tempHistoryIndex];
    
    Serial.print(F("Temp Trend - Avg: "));
    Serial.print(avgTemp, 1);
    Serial.print(F("°C, Delta: "));
    Serial.print(tempDelta, 2);
    Serial.println(F("°C/30s"));
}

void checkTemperatureAndAct() {
    float currentTemp = readTemperature();
    float estimatedTopTemp = estimateTopTemperature(currentTemp);
    
    // Update temperature history
    tempHistory[tempHistoryIndex] = currentTemp;
    status.tempHistory[tempHistoryIndex] = currentTemp;
    tempHistoryIndex = (tempHistoryIndex + 1) % TEMP_HISTORY_SIZE;
    
    // Update status
    status.currentTemp = currentTemp;
    status.estimatedTopTemp = estimatedTopTemp;
    
    // Update fan speeds
    updateFanSpeeds(currentTemp);
    
    // Print status
    Serial.print(F("Bottom Temp: "));
    Serial.print(currentTemp, 1);
    Serial.print(F("°C, Est. Top: "));
    Serial.print(estimatedTopTemp, 1);
    Serial.print(F("°C, State: "));
    Serial.println(getStateString());
    
    // Actuator control based on bottom temperature
    if (currentState == IDLE) {
        if (lastTemperature <= TEMP_OPEN_THRESHOLD && 
            currentTemp > TEMP_OPEN_THRESHOLD) {
            extendActuator();
        }
        else if (lastTemperature > TEMP_CLOSE_THRESHOLD && 
                 currentTemp <= TEMP_CLOSE_THRESHOLD) {
            retractActuator();
        }
    }
    
    lastTemperature = currentTemp;
}

void extendActuator() {
    digitalWrite(PIN_ACTUATOR_EXTEND, LOW);
    digitalWrite(PIN_ACTUATOR_RETRACT, HIGH);
    currentState = EXTENDING;
    status.actuatorState = EXTENDING;
    actuatorStartTime = millis();
    Serial.println(F("Extending actuator"));
}

void retractActuator() {
    digitalWrite(PIN_ACTUATOR_EXTEND, HIGH);
    digitalWrite(PIN_ACTUATOR_RETRACT, LOW);
    currentState = RETRACTING;
    status.actuatorState = RETRACTING;
    actuatorStartTime = millis();
    Serial.println(F("Retracting actuator"));
}

void stopActuator() {
    digitalWrite(PIN_ACTUATOR_EXTEND, HIGH);
    digitalWrite(PIN_ACTUATOR_RETRACT, HIGH);
    currentState = IDLE;
    status.actuatorState = IDLE;
    Serial.println(F("Stopping actuator"));
}

const char* getStateString() {
    switch (currentState) {
        case IDLE: return "IDLE";
        case EXTENDING: return "EXTENDING";
        case RETRACTING: return "RETRACTING";
        default: return "UNKNOWN";
    }
}

void sendJSONStatus(EthernetClient& client) {
    StaticJsonDocument<512> doc;
    
    doc["temperature"] = status.currentTemp;
    doc["estimatedTopTemp"] = status.estimatedTopTemp;
    doc["exhaustFanSpeed"] = status.exhaustFanSpeed;
    doc["intakeFanSpeed"] = status.intakeFanSpeed;
    doc["actuatorState"] = getStateString();
    doc["uptime"] = millis();
    
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    
    serializeJson(doc, client);
}

void sendWebPage(EthernetClient& client) {
    // Send HTTP headers
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html"));
    client.println(F("Connection: close"));
    client.println();
    
    // Start HTML document
    client.println(F("<!DOCTYPE HTML>"));
    client.println(F("<html><head><title>Temperature Control System</title>"));
    
    // CSS Styles
    client.println(F("<style>"));
    client.println(F("body{font-family:Arial,sans-serif;margin:20px;background:#f0f2f5}"));
    client.println(F(".container{max-width:800px;margin:0 auto;padding:20px;background:white;border-radius:10px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}"));
    client.println(F(".status-card{background:#fff;padding:15px;margin:10px 0;border-radius:8px;border:1px solid #ddd}"));
    client.println(F(".temp-display{font-size:24px;font-weight:bold;color:#1a73e8}"));
    client.println(F(".fan-status{display:flex;justify-content:space-between;margin:20px 0}"));
    client.println(F(".fan-gauge{width:45%;background:#f5f5f5;border-radius:4px;padding:10px}"));
    client.println(F(".progress-bar{height:20px;background:#e1e1e1;border-radius:10px;overflow:hidden}"));
    client.println(F(".progress-fill{height:100%;background:#4CAF50;transition:width 0.3s ease}"));
    client.println(F("</style></head><body>"));
    
    // Basic page structure
    client.println(F("<div class='container'>"));
    client.println(F("<h1>Temperature Control System</h1>"));
    client.println(F("<div id='status'>Loading...</div>"));
    client.println(F("</div>"));
    
    // JavaScript for updates
    client.println(F("<script>"));
    
    // Helper function for uptime formatting
    client.println(F("function formatUptime(ms){"));
    client.println(F("const s=Math.floor(ms/1000);"));
    client.println(F("const m=Math.floor(s/60);"));
    client.println(F("const h=Math.floor(m/60);"));
    client.println(F("const d=Math.floor(h/24);"));
    client.println(F("if(d>0)return`${d}d ${h%24}h`;"));
    client.println(F("if(h>0)return`${h}h ${m%60}m`;"));
    client.println(F("if(m>0)return`${m}m ${s%60}s`;"));
    client.println(F("return`${s}s`}"));
    
    // Main update function
    client.println(F("function updateStatus(){"));
    client.println(F("fetch('/api/status')"));
    client.println(F(".then(r=>r.json())"));
    client.println(F(".then(d=>{"));
    client.println(F("document.getElementById('status').innerHTML=`"));
    
    // Temperature display
    client.println(F("<div class='status-card'><h2>Current Temperature</h2>"));
    client.println(F("<div class='temp-display'>${d.temperature.toFixed(1)}°C"));
    client.println(F("<div style='font-size:14px;color:#666'>Est. top: ${d.estimatedTopTemp.toFixed(1)}°C</div></div></div>"));
    
    // Fan status display
    client.println(F("<div class='fan-status'><div class='fan-gauge'><h3>Exhaust Fan</h3>"));
    client.println(F("<div class='progress-bar'><div class='progress-fill' style='width:${d.exhaustFanSpeed}%'></div></div>"));
    client.println(F("<div>${d.exhaustFanSpeed}%</div></div>"));
    client.println(F("<div class='fan-gauge'><h3>Intake Fan</h3>"));
    client.println(F("<div class='progress-bar'><div class='progress-fill' style='width:${d.intakeFanSpeed}%'></div></div>"));
    client.println(F("<div>${d.intakeFanSpeed}%</div></div></div>"));
    
    // Actuator status
    client.println(F("<div class='status-card'><h3>Actuator Status</h3>"));
    client.println(F("<div style='font-size:18px;margin-top:10px;color:${d.actuatorState==='IDLE'?'#4CAF50':'#FFA500'}'>${d.actuatorState}</div></div>"));
    
    // System status
    client.println(F("<div class='status-card'><h3>System Status</h3><div>Uptime: ${formatUptime(d.uptime)}</div></div>`"));
    
    // Error handling
    client.println(F("}).catch(e=>{console.error('Error:',e);"));
    client.println(F("document.getElementById('status').innerHTML=`<div class=\"status-card\" style=\"color:red\">Error connecting to device. Retrying...</div>`})}"));
    
    // Update interval
    client.println(F("setInterval(updateStatus,1000);"));
    client.println(F("updateStatus();"));
    client.println(F("</script></body></html>"));
}

void handleWebClient() {
    EthernetClient client = server.available();
    if (client) {
        boolean currentLineIsBlank = true;
        String httpMethod = "";
        String httpPath = "";
        boolean isFirstLine = true;
        
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                
                // Parse first line for method and path
                if (isFirstLine) {
                    if (c == ' ') {
                        isFirstLine = false;
                    } else if (httpMethod.length() == 0) {
                        httpMethod += c;
                    } else {
                        httpPath += c;
                    }
                }
                
                if (c == '\n' && currentLineIsBlank) {
                    // Send response based on request
                    if (httpMethod == "GET") {
                        if (httpPath.startsWith("/api/status")) {
                            sendJSONStatus(client);
                        } else {
                            sendWebPage(client);
                        }
                    }
                    break;
                }
                
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            }
        }
        delay(1);
        client.stop();
    }
}

void loop() {
    static unsigned long lastTempCheck = 0;
    static unsigned long lastTrendCheck = 0;
    unsigned long currentMillis = millis();
    
    // Update system status
    status.uptime = currentMillis;
    
    // Regular temperature checking
    if (currentMillis - lastTempCheck >= TEMP_CHECK_INTERVAL) {
        lastTempCheck = currentMillis;
        checkTemperatureAndAct();
    }
    
    // Temperature trend analysis
    if (currentMillis - lastTrendCheck >= TEMP_TREND_INTERVAL) {
        lastTrendCheck = currentMillis;
        analyzeTempTrend();
    }
    
    // Actuator timeout checking
    if (currentState != IDLE && 
        (currentMillis - actuatorStartTime >= ACTUATOR_TIMEOUT)) {
        stopActuator();
    }
    
    // Handle web clients
    handleWebClient();
    
    // Process any pending Ethernet packets
    Ethernet.maintain();
}
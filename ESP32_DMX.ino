#include <Arduino.h>
#include <esp_dmx.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "Settings.h"

int transmitPin = 5;
int receivePin = 8;
int enablePin = 9;
dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();

const int pwmPin = 10; 
const int freq = 22000; // Frequenza in Hz
const int resolution = 8; // Risoluzione (8 bit = 0-255)
const int channel = 2;  // Canale PWM



const char* password = "12345678";
int dmxSlot = 1;

AsyncWebServer server(80);
Preferences preferences;

const char PAGE_HOME[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controllo DMX</title>
    <style>
        body {
            font-family: 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, #007aff, #00c6ff);
            height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .container {
            background: white;
            padding: 30px;
            border-radius: 16px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
            text-align: center;
            width: 90%;
            max-width: 420px;
        }

        h1 {
            color: #333;
            font-size: 24px;
            margin-bottom: 20px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-size: 16px;
            font-weight: 500;
        }

        input[type="number"], input[type="submit"] {
            width: 100%;
            padding: 14px;
            border: 1px solid #ddd;
            border-radius: 8px;
            box-sizing: border-box;
            font-size: 16px;
            transition: 0.3s ease;
        }

        input[type="number"] {
            margin-bottom: 16px;
        }

        input[type="submit"] {
            background-color: #007aff;
            color: white;
            font-weight: bold;
            border: none;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: #005ecb;
        }

        .slider-container {
            margin-top: 20px;
        }

        .slider-wrapper {
            position: relative;
            text-align: center;
        }

        .slider {
            width: 100%;
            appearance: none;
            height: 8px;
            border-radius: 5px;
            background: #ddd;
            outline: none;
            transition: opacity 0.2s;
        }

        .slider:hover {
            opacity: 1;
        }

        .slider::-webkit-slider-thumb {
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #007aff;
            cursor: pointer;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
        }

        .slider-value {
            margin-top: 8px;
            font-size: 16px;
            font-weight: bold;
            color: #007aff;
        }
    </style>
    <script>
        function sendVoltageControl(value) {
            fetch(`/phd_voltage?voltage=${value}`)
                .then(response => {
                    if (response.ok) {
                        console.log("Voltage control sent successfully with value:", value);
                    } else {
                        console.error("Failed to send voltage control");
                    }
                });
            document.getElementById('sliderValue').textContent = value;
        }
    </script>
</head>
<body>
    <div class="container">
        <h1>Imposta Slot DMX</h1>
        <form action="/setSlot" method="get">
            <label for="slot">Slot DMX:</label>
            <input type="number" id="slot" name="slot" min="1" max="512" placeholder="Inserisci numero slot">
            <input type="submit" value="Imposta">
        </form>
        
        <div class="slider-container">
            <label for="voltageSlider">Imposta valore PWM</label>
            <div class="slider-wrapper">
                <input type="range" min="0" max="255" step="1" value="0" class="slider" id="voltageSlider" oninput="sendVoltageControl(this.value)">
                <div class="slider-value" id="sliderValue">0</div>
            </div>
        </div>
    </div>
</body>
</html>

)=====";

void setup() {
  Serial.print("Init...");
  preferences.begin("config", true);
  Serial.print("Recupero slot DMX: ");
  dmxSlot = Settings::getInstance()-> readInt("dmxSlot", 1);
  Serial.println(dmxSlot);


  preferences.end();
  
  Serial.begin(115200);
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = { {1, "Default Personality"} };
  int personality_count = 1;
  dmx_driver_install(dmxPort, &config, personalities, personality_count);
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);

  uint64_t chipId = ESP.getEfuseMac();
  // Converti il chipId in una stringa esadecimale
  String chipIdStr = String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);

  // Crea l'SSID con il chipId
  String ssid = "DMX_"+ chipIdStr;


  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_HOME);
  });

  server.on("/setSlot", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("slot")){
      
      dmxSlot = request->getParam("slot")->value().toInt();
    
      dmxSlot = Settings::getInstance()-> writeInt("dmxSlot", dmxSlot);
  
      Serial.print("Slot DMX impostato a: ");
      Serial.println(dmxSlot);

      preferences.end();

      String resp = "Slot DMX impostato con successo a " + String(dmxSlot);
      
       request->send(200, "text/plain", resp);
    } 
    else
    {
      request->send(400, "text/plain", "Parametro 'slot' mancante.");
    }
  });

    server.on("/phd_voltage", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (request->hasParam("voltage")) {
          int voltageValue = request->getParam("voltage")->value().toInt();
          ledcWrite(pwmPin, voltageValue);
      }
      request->send(200, "OK");
  });

  server.begin();

  ledcAttachChannel(pwmPin, freq, resolution, channel);

}

void loop() 
{
  dmx_packet_t packet;
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    if (!packet.err) {
      unsigned long now = millis();
      dmx_read(dmxPort, data, packet.size);
      if (now - lastUpdate > 100) {
        Serial.printf("Start code is 0x%02X and slot %d is 0x%02X\n", data[0], dmxSlot, data[dmxSlot]);
        lastUpdate = now;
        ledcWrite(pwmPin, data[dmxSlot]);
      }
    } else {
      Serial.println("A DMX error occurred.");
    }
  } else if (dmxIsConnected) {
    Serial.println("DMX was disconnected.");
    dmx_driver_delete(dmxPort);
    while (true) yield();
  }
}

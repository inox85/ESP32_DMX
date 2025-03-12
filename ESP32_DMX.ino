#include <Arduino.h>
#include <esp_dmx.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

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


const char* ssid = "ESP32-DMX";
const char* password = "password";
int dmxSlot = 1;

AsyncWebServer server(80);
Preferences preferences;

const char PAGE_HOME[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Imposta Slot DMX</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, #e0eafc, #68a2e8);
            height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .header {
            width: 100%;
            background: linear-gradient(to right, #4a00e0, #8e2de2);
            color: white;
            text-align: center;
            padding: 20px 0;
            margin-bottom: 20px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
        }

        .container {
            background-color: rgba(255, 255, 255, 0.9);
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
            text-align: center;
            width: 90%; /* Larghezza adattabile ai dispositivi mobili */
            max-width: 400px; /* Larghezza massima */
        }

        h1 {
            color: #333;
            margin-bottom: 20px;
        }

        label {
            display: block;
            margin-bottom: 10px;
            color: #555;
            text-align: left; /* Allineamento a sinistra per i label */
        }

        input[type="number"] {
            padding: 12px;
            border: 1px solid #ccc;
            border-radius: 5px;
            width: calc(100% - 24px); /* Larghezza adattabile */
            margin-bottom: 20px;
            box-sizing: border-box; /* Include padding e border nella larghezza */
        }

        input[type="submit"] {
            background-color: #4CAF50;
            color: white;
            padding: 14px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            width: 100%; /* Larghezza adattabile */
        }

        input[type="submit"]:hover {
            background-color: #45a049;
        }

        .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #d3d3d3;
            outline: none;
            opacity: 0.7;
            -webkit-transition: .2s;
            transition: opacity .2s;
        }
        
        .slider:hover {
            opacity: 1;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #800020;
            cursor: pointer;
            vertical-align: middle;
        }
        
        .slider::-webkit-slider-runnable-track {
            background: #800020;
            height: 8px;
            border-radius: 5px;
        }
        
        .slider:focus {
            outline: none;
        }
        
        .slider:focus::-webkit-slider-thumb {
            box-shadow: 0 0 5px #800020;
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
        }

    </script>
</head>
<body>
    <div class="header">
        <h1>Controllo DMX</h1>
    </div>
    <div class="container">
        <h1>Imposta Slot DMX</h1>
        <form action="/setSlot" method="get">
            <label for="slot">Slot DMX:</label>
            <input type="number" id="slot" name="slot" value="">
            <input type="submit" value="Imposta">
        </form>

        <div class="slider-container">
            <label class="slider-label" for="voltageSlider">Imposta valore PWM</label>
            <div class="slider-wrapper">
                <input type="range" min="0" max="255" step="1" value="0" class="slider" id="voltageSlider" oninput="sendVoltageControl(this.value)">
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
  dmxSlot = preferences.getUInt("slot", 1);
  Serial.println(dmxSlot);


  preferences.end();
  
  Serial.begin(115200);
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = { {1, "Default Personality"} };
  int personality_count = 1;
  dmx_driver_install(dmxPort, &config, personalities, personality_count);
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);


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
      Serial.print("Slot DMX impostato a: ");
      Serial.println(dmxSlot);

      preferences.begin("config", false);

      preferences.putUInt("slot", dmxSlot);
      
      dmxSlot = preferences.getUInt("slot", 1);
      Serial.print("Recupero slot DMX: ");
      Serial.println(dmxSlot);

      preferences.end();

      String resp = "Slot DMX impostato con successo a " + String(dmxSlot);
      
      request->send(200, "text/html", resp);
    } 
    else
    {
      request->send(400, "text/html", "Parametro 'slot' mancante.");
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

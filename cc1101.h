// https://github.com/dbuezas/esphome-cc1101

#ifndef CC1101TRANSCIVER_H
#define CC1101TRANSCIVER_H

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <ArduinoJson.h>

int CC1101_module_count = 0;
#define get_cc1101(id) (*((CC1101 *)id))

class CC1101 : public PollingComponent, public Sensor {
  int _SCK;
  int _MISO;
  int _MOSI;
  int _CSN;
  int _GDO0; // TX (and also RX if ESP8266)
#ifdef USE_ESP32
  int _GDO2; // RX
#endif
  float _bandwidth;
  
  float _moduleNumber;
  int _last_rssi = 0;

  void setup() {
#ifdef USE_ESP32
    pinMode(_GDO0, OUTPUT);
    pinMode(_GDO2, INPUT);
#else
    pinMode(_GDO0, INPUT);
#endif
    ELECHOUSE_cc1101.addSpiPin(_SCK, _MISO, _MOSI, _CSN, _moduleNumber);
    ELECHOUSE_cc1101.setModul(_moduleNumber);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(_bandwidth);
    ELECHOUSE_cc1101.setMHZ(_freq);
    ELECHOUSE_cc1101.SetRx();
  }

 public:
  float _freq;
  CC1101(int SCK, int MISO, int MOSI, int CSN, int GDO0, 
#ifdef USE_ESP32
         int GDO2, 
#endif
         float bandwidth, float freq)
      : PollingComponent(100) {
    _SCK = SCK;
    _MISO = MISO;
    _MOSI = MOSI;
    _CSN = CSN;
    _GDO0 = GDO0;
#ifdef USE_ESP32
    _GDO2 = GDO2;
#endif
    _bandwidth = bandwidth;
    _freq = freq;
    _moduleNumber = CC1101_module_count++;
  }

  void beginTransmission() {
    ELECHOUSE_cc1101.setModul(_moduleNumber);
    ELECHOUSE_cc1101.SetTx();
#ifndef USE_ESP32
    pinMode(_GDO0, OUTPUT);
    noInterrupts();
#endif
  }
  void endTransmission() {
#ifndef USE_ESP32
    interrupts();
    pinMode(_GDO0, INPUT);
#endif
    ELECHOUSE_cc1101.setModul(_moduleNumber);
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.SetRx();  // yes, twice
  }
  void setBW(float bandwidth) {
    ELECHOUSE_cc1101.setModul(_moduleNumber);
    ELECHOUSE_cc1101.setRxBW(bandwidth);
  }
  bool rssi_on;
  void update() override {
    int rssi = 0;
    if (rssi_on) {
      ELECHOUSE_cc1101.setModul(_moduleNumber);
      rssi = ELECHOUSE_cc1101.getRssi();
    }
    if (rssi != _last_rssi) {
      decode_to_json(data, rssi);

//      ELECHOUSE_cc1101.ReceiveData();
      // Get received data
//      uint8_t* data = ELECHOUSE_cc1101.getData();
//      uint8_t data_length = ELECHOUSE_cc1101.getLength();

      // Display received data
//      Serial.print("Received data: ");
//      for (int i = 0; i < data_length; i++) {
//        Serial.print(data[i], HEX);
//        Serial.print(" ");
//      }
//      Serial.println();

      ESP_LOGD("cc1101", "Received RSSI: %d", rssi);
      publish_state(rssi);
      _last_rssi = rssi;
    }
  }

  void decode_to_json(uint8_t* data, uint8_t length) {
    // Create a JSON document
//    StaticJsonDocument<256> doc;
    DynamicJsonDocument doc(256);

    // Example: assume data contains key-value pairs
    for (uint8_t i = 0; i < length; i += 2) {
      char key[2] = { static_cast<char>(data[i]), '\0' };
      doc[key] = data[i + 1];
    }

    // Log the DOC output
    ESP_LOGD("cc1101", "DOC: %s", doc);

    // Serialize JSON document to string
    char json_output[256];
    serializeJson(doc, json_output, sizeof(json_output));

    // Log the JSON output
    ESP_LOGD("cc1101", "JSON: %s", json_output);
  }

  uint8_t data[64];  // Buffer to hold received data

};

#endif
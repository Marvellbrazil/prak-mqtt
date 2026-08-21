#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <rom/rtc.h>

#ifdef __cplusplus
extern "C"
{
  uint8_t temprature_sens_read();
  int hallRead();
}
#endif

const char *ssid = "Hyper Data-2.4G";
const char *password = "hyperdata2026";

const char *mqtt_server = "fc8997de959045f2b0e09a13b4521491.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char *mqtt_user = "esp32_client";
const char *mqtt_password = "secret123";

const char *topic_publish = "iot/esp32/telemetry";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const char *getResetReason(RESET_REASON reason)
{
  switch (reason)
  {
  case 1:
    return "POWERON_RESET";
  case 3:
    return "SW_RESET";
  case 4:
    return "OWDT_RESET";
  case 5:
    return "DEEPSLEEP_RESET";
  case 6:
    return "SDIO_RESET";
  case 7:
    return "TG0WDT_SYS_RESET";
  case 8:
    return "TG1WDT_SYS_RESET";
  case 9:
    return "RTCWDT_SYS_RESET";
  case 10:
    return "INTRUSION_RESET";
  case 11:
    return "TGWDT_CPU_RESET";
  case 12:
    return "SW_CPU_RESET";
  case 13:
    return "RTCWDT_CPU_RESET";
  case 14:
    return "EXT_CPU_RESET";
  case 15:
    return "RTCWDT_BROWN_OUT_RESET";
  case 16:
    return "RTCWDT_RTC_RESET";
  default:
    return "UNKNOWN";
  }
}

void connectWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    String clientId = "ESP32-" + String(random(0xffff), HEX);
    if (!client.connect(clientId.c_str(), mqtt_user, mqtt_password))
    {
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  connectWiFi();

  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(1024);
}

int calculateSignalQuality(int rssi)
{
  if (rssi <= -100)
    return 0;
  if (rssi >= -50)
    return 100;
  return 2 * (rssi + 100);
}

void loop()
{
  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 3000)
  {
    lastMsg = millis();

    int wifiRSSI = WiFi.RSSI();
    int signalQuality = calculateSignalQuality(wifiRSSI);
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t heapSize = ESP.getHeapSize();
    uint32_t cpuFreqMHz = ESP.getCpuFreqMHz();
    uint32_t flashSizeMB = ESP.getFlashChipSize() / (1024 * 1024);
    uint32_t flashSpeedMHz = ESP.getFlashChipSpeed() / 1000000;
    float chipTempC = (temprature_sens_read() - 32) / 1.8;
    int hallVal = hallRead();
    int touchVal = touchRead(4);

    StaticJsonDocument<1024> doc;

    doc["device_id"] = "ESP32_DOIT_DEVKIT_V1";
    doc["uptime_sec"] = millis() / 1000;

    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["chip_temp_c"] = round(chipTempC * 10.0) / 10.0;
    sensors["hall_effect"] = hallVal;
    sensors["touch_gpio4"] = touchVal;

    JsonObject network = doc.createNestedObject("network");
    network["ssid"] = WiFi.SSID();
    network["bssid"] = WiFi.BSSIDstr();
    network["channel"] = WiFi.channel();
    network["ip"] = WiFi.localIP().toString();
    network["mac"] = WiFi.macAddress();
    network["gateway"] = WiFi.gatewayIP().toString();
    network["subnet"] = WiFi.subnetMask().toString();
    network["dns"] = WiFi.dnsIP().toString();
    network["rssi_dbm"] = wifiRSSI;
    network["signal_pct"] = signalQuality;

    JsonObject system = doc.createNestedObject("system");
    system["chip_model"] = ESP.getChipModel();
    system["chip_revision"] = ESP.getChipRevision();
    system["chip_cores"] = ESP.getChipCores();
    system["cpu_mhz"] = cpuFreqMHz;
    system["free_heap_kb"] = freeHeap / 1024;
    system["min_free_heap_kb"] = minFreeHeap / 1024;
    system["heap_usage_pct"] = round(((float)(heapSize - freeHeap) / heapSize) * 100.0);
    system["flash_mb"] = flashSizeMB;
    system["flash_speed_mhz"] = flashSpeedMHz;
    system["reset_reason_cpu0"] = getResetReason(rtc_get_reset_reason(0));
    system["reset_reason_cpu1"] = getResetReason(rtc_get_reset_reason(1));

    char jsonBuffer[1024];
    serializeJson(doc, jsonBuffer);
    client.publish(topic_publish, jsonBuffer);
  }
}

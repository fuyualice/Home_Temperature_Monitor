#include <Arduino.h>
#include <Wire.h>
#include "time.h"

#include "secrets.h"
#include "RX8025.h"
// #include "image.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>

#include <WiFi.h>
#include <HTTPClient.h>

#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

#define I2C_SDA 21
#define I2C_SCL 22

RX8025 rtc;

Adafruit_BME280 bme;
struct BME280Data {
  float temperature;
  float humidity;
  float pressure;
};

// CS=5, DC=17, RST=16, BUSY=4
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

BME280Data read_BME280(Adafruit_BME280 &bme){
  BME280Data data;

  data.temperature = bme.readTemperature();
  data.humidity = bme.readHumidity();
  data.pressure = bme.readPressure() / 100.0F;

  Serial.print("Temp: ");
  Serial.print(data.temperature);
  Serial.println("℃");

  Serial.print("Hum: ");
  Serial.print(data.humidity);
  Serial.println("%");

  Serial.print("Pres: ");
  Serial.print(data.pressure);
  Serial.println("hPa");

  return data;
}

void sync_NTP(){
  configTime(9*3600, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)){
    Serial.println("[NTP] Failed to obtain time");
    return;
  }

  // Serial.println(&timeinfo, "[NTP] %A, %B %d %Y %H:%M:%S");

  DataTime dt;
  dt.second = timeinfo.tm_sec;
  dt.minute = timeinfo.tm_min;
  dt.hour = timeinfo.tm_hour;
  dt.day = timeinfo.tm_mday;
  dt.month = timeinfo.tm_mon + 1;
  dt.year = timeinfo.tm_year + 1900;
  dt.weekday = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday;
  rtc.write(&dt);

  Serial.printf("[NTP] %d/%d/%d %d:%d:%d (%d)\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.weekday);
}

bool upload_data_influxdb(BME280Data &bme280, DataTime &dt){
  char data[256];
  snprintf(data, sizeof(data), "BME280 temperature=%f,humidity=%f,pressure=%f", bme280.temperature, bme280.humidity, bme280.pressure);

  Serial.println(data);

  HTTPClient http;
  http.begin(influxdb_url);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Authorization", influxdb_token);
  uint16_t httpCode = http.POST(data);
  // String payload = http.getString();
  http.end();

  if (httpCode > 0) {
    Serial.printf("[influxdb] %d\n", httpCode);
    // Serial.println("[influxdb] Payload: " + payload);
    return true;
  }
  else{
    Serial.printf("Error sending data: %s\n", HTTPClient::errorToString(httpCode));
    return false;
  }
}

void routine(){
  BME280Data bme280 = read_BME280(bme);
  DataTime dt;
  rtc.read(&dt);
  rtc.set_alarm(&dt);

  if (WiFi.status() == WL_CONNECTED){
    if (dt.hour == 0 && dt.minute == 0){
      sync_NTP();
    }
    upload_data_influxdb(bme280, dt);
  }
  else{
    Serial.println("[Network] Wi-Fi is not connected.");
  }

  char timeStr[32];
  snprintf(timeStr, sizeof(timeStr), "%04d/%02d/%02d %02d:%02d:%02d (%d)",
           dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.weekday);

  // display.setFullWindow();
  display.setPartialWindow(0,0,display.width(),display.height());
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(10, 50);
    display.printf(timeStr);

    display.setCursor(10, 120);
    display.printf("Temp: %.1f C", bme280.temperature);

    display.setCursor(10, 190);
    display.printf("Hum: %.1f %%", bme280.humidity);

    display.setCursor(10, 260);
    display.printf("Pres: %.1f hPa", bme280.pressure);
  }
  while (display.nextPage());
  // display.hibernate();

}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(32, INPUT_PULLUP);

  if (!bme.begin(0x76)){
    Serial.println("[BME280] Could not find the BME280.");
  }

  display.init(115200);
  display.setRotation(0);
  display.setFullWindow();
  display.setFont(&FreeMono12pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);
  display.display(true);  // trueで全画面更新

  unsigned long start = millis();
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
  {
    Serial.print(".");
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("[Wi-Fi] Connected.");
  }

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED){
    Serial.println("[system] Cold Start.");
    sync_NTP();
  }
  else{
    Serial.println("[system] Wakeup from Deep Sleep.");
  }

  rtc.init();

  routine();
  delay(2000);

  Serial.println("--------------------");
  
  WiFi.disconnect(true);
  pinMode(25, INPUT);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)25, LOW);
  esp_deep_sleep_start();
}

void loop() {
}
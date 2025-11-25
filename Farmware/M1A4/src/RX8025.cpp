#include "RX8025.h"

uint8_t RX8025::bcd2dec(uint8_t val){
  return ((val >> 4) * 10) + (val & 0x0F);
}
uint8_t RX8025::dec2bcd(uint8_t val){
  return ((val / 10) << 4) | (val % 10);
}

void RX8025::init(){
  uint8_t regE_data = 0x60;
  uint8_t regF_data = 0x00;

  Wire.beginTransmission(RX8025_ADDR);
  Wire.write(0xE0);
  Wire.write(regE_data);
  Wire.write(regF_data);
  Wire.endTransmission();

  Serial.println("[RTC] Init.");
  delay(1);
}

void RX8025::read(DataTime* dt){
  Wire.requestFrom(RX8025_ADDR, 8);
  Wire.read();
  dt->second = bcd2dec(Wire.read() & 0x7F);
  dt->minute = bcd2dec(Wire.read() & 0x7F);
  dt->hour = bcd2dec(Wire.read() & 0x3F);
  dt->weekday = bcd2dec(Wire.read() & 0x07);
  dt->day = bcd2dec(Wire.read() & 0x3F);
  dt->month = bcd2dec(Wire.read() & 0x1F);
  dt->year = bcd2dec(Wire.read()) + 2000;

  Serial.printf("[RTC Read] %d/%d/%d %d:%d:%d (%d)\n", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second, dt->weekday);
}

void RX8025::write(DataTime* dt){
  Wire.beginTransmission(RX8025_ADDR);
  Wire.write(0x00);
  Wire.write(dec2bcd(dt->second));
  Wire.write(dec2bcd(dt->minute));
  Wire.write(dec2bcd(dt->hour));
  Wire.write(dec2bcd(dt->weekday));
  Wire.write(dec2bcd(dt->day));
  Wire.write(dec2bcd(dt->month));
  Wire.write(dec2bcd(dt->year - 2000));
  Wire.endTransmission();

  Serial.printf("[RTC Write] %d/%d/%d %d:%d:%d (%d)\n", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second, dt->weekday);
  delay(1);
}

void RX8025::set_alarm(DataTime* dt){
    uint8_t alarm_minute = (dt->minute / 10 * 10 + 10) % 60;
    uint8_t alarm_hour = (dt->hour + ((dt->minute / 10 * 10) + 10) / 60) % 24;
    Wire.beginTransmission(RX8025_ADDR);
    Wire.write(0xB0);
    Wire.write(dec2bcd(alarm_minute));
    Wire.write(dec2bcd(alarm_hour));
    Wire.endTransmission();
    Serial.printf("[RTC Set Alarm] %d:%d\n", alarm_hour, alarm_minute);
    delay(1);
}
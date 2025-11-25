#pragma once
#include <Arduino.h>
#include <Wire.h>

#define RX8025_ADDR 0x32

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t weekday;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} DataTime;

class RX8025{
    public:
        void init();
        void read(DataTime* dt);
        void write(DataTime* dt);
        void set_alarm(DataTime* dt);
    private:
        uint8_t bcd2dec(uint8_t val);
        uint8_t dec2bcd(uint8_t val);
};
#include <SPI.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "RF24.h"
#include "Wire.h"

// ---------- Wiring ----------
// Roomba TX (mini-DIN pin 4) -> Uno D4 (SoftwareSerial RX)
// Roomba RX (mini-DIN pin 3) <- Uno D5 (SoftwareSerial TX)
// GND <-> GND

// ---------- Roomba OI ----------
#define OI_RESET 7 
#define OI_START 128 
#define OI_SAFE 131 
#define OI_QUERY_LIST 149 
#define OI_DRIVE 137 
#define OI_FORWARD 0x8000 
#define OI_STOP 0x00 
#define OI_LEFT 1 
#define OI_RIGHT -1 
#define OI_SONG 140 
#define OI_PLAY 141 
#define OI_SENSORS 142 
#define PKT_DIST 19 
#define PKT_ANGLE 20 
#define OI_CLEAN 135
#define OI_BAUD        129
#define BAUD_19200_CODE 11  // per OI spec

// ---------- Serial to Roomba ----------
SoftwareSerial ss(4, 5); // RX, TX

// ---------- RF24 ----------
RF24 myRadio(7, 8);
byte addresses[][6] = {"0"};

struct package {
  int16_t dist_mm;
  int16_t angle_deg;
  bool connected;
};
package data = {0, 0, true};

// ---------- Helpers ----------
void drain(Stream &s) {
  while (s.available()) (void)s.read();
}

bool readN(Stream &s, uint8_t *buf, uint8_t n, uint16_t timeout_ms) {
  uint8_t got = 0; uint32_t t0 = millis();
  while (got < n && (millis() - t0) < timeout_ms) {
    if (s.available()) buf[got++] = s.read();
  }
  return got == n;
}

// Change Roomba to 19200 (start at 115200 TX only), then reopen ss @ 19200
void setRoombaTo19200() {
  ss.begin(115200);     // Roomba default after real power cycle
  delay(120);
  ss.write(OI_START);   // Passive
  delay(40);
  ss.write(OI_BAUD);    // Change baud
  ss.write(BAUD_19200_CODE);
  delay(100);

  ss.end();             // switch local UART to 19200 to match
  delay(30);
  ss.begin(19200);
  delay(120);

  ss.write(OI_START);   // Wake again at new baud
  delay(40);
  ss.write(OI_SAFE);    // Safe mode (enables sensors/sounds; we won't drive)
  delay(40);
}

// drives the roomba motors
void roomba_drive(int radius = 0x8000, int velocity = 200) {
    ss.write(OI_DRIVE);
    ss.write(highByte(velocity));
    ss.write(lowByte(velocity));
    ss.write(highByte(radius));
    ss.write(lowByte(radius));
}


// this makes the Roomba do a lil dance and play a tune 
// so that we know it is properly connected to the arduino
void connection_display() {

  // do a lil dance
  roomba_drive(1); delay(1000);
  roomba_drive(-1); delay(2000);
  roomba_drive(1); delay(1000);
  roomba_drive(); delay(1000);
  roomba_drive(0x8000, 0);

  // store song on Roomba
  uint8_t song[] = { OI_SONG, 0, 1, 69, 32 };
  ss.write(song, sizeof(song));

  // play song
  uint8_t play[] = { OI_PLAY, 0 };
  ss.write(play, sizeof(play));
}

void setup() {
  // Radio
  myRadio.begin();
  myRadio.setChannel(115);
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate(RF24_250KBPS);
  myRadio.openWritingPipe(addresses[0]);

  // Roomba @ 19200 for reliable SoftwareSerial RX
  setRoombaTo19200();
  connection_display();
  delay(1000);
  ss.write(OI_CLEAN);
}

void loop() {
  static const uint8_t QUERY_DIST_ANGLE[] = { OI_QUERY_LIST, 2, PKT_DIST, PKT_ANGLE };

  // 1) Clear any stale bytes
  drain(ss);

  // 2) Request both in one shot
  ss.write(QUERY_DIST_ANGLE, sizeof(QUERY_DIST_ANGLE));

  // 3) Read exactly 4 bytes (big-endian): dist(2), angle(2)
  uint8_t raw[4];
  if (!readN(ss, raw, 4, 200)) {
    data.connected = false;
    myRadio.write(&data, sizeof(data));
    delay(50);
    return;
  }
  data.connected = true;

  // 4) Parse SIGNED 16-bit
  int16_t d_mm    = int16_t((uint16_t(raw[0]) << 8) | raw[1]);
  int16_t d_angle = int16_t((uint16_t(raw[2]) << 8) | raw[3]);

  data.dist_mm  = d_mm;
  data.angle_deg = d_angle;

  // 5) Transmit to peer
  myRadio.write(&data, sizeof(data));

  // 6) Pace read rate (adjust as desired)
  delay(50); // ~20 Hz
}

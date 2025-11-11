#include <SPI.h>
#include "RF24.h"

RF24 myRadio(7, 8);
byte addresses[][6] = {"0"};

struct package {
  int dist_mm, angle_deg;
  bool connected = false;
};
typedef struct package Package;
Package data;


void setup() {
  Serial.begin(115200);

  myRadio.begin();
  myRadio.setChannel(115);
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate(RF24_250KBPS);
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();

  Serial.print("Setup of reciever completed\n");
}

void loop() {

  data.connected = false;
  if (myRadio.available()) {
    while (myRadio.available()) {
      myRadio.read(&data, sizeof(data));
    }
  }

  // avoid uneeded printing
  if (!data.connected) return;
  //if (data.dist_mm == 0 && data.angle_deg == 0) return;

  Serial.print(data.dist_mm);
  Serial.print(", ");
  Serial.println(data.angle_deg);
}

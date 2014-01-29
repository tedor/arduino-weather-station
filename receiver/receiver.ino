#include <SPI.h>

const uint64_t pipe = 0xF0A1B2C3D1LL; // listening pipe number
const bool debug = false;             // enable/disable debug mode

// NRF24 init
#include <RF24.h>
#include <nRF24L01.h>
#include "printf.h"
#define NRF24_CE 8
#define NRF24_CS 9
RF24 radio(NRF24_CE, NRF24_CS);

void setup() {
  Serial.begin(9600);
 
  // Init NRF24 radio device
  if(debug) {
    printf_begin();  
  }
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(67);
  radio.setAutoAck(1);
  radio.enableDynamicPayloads();
  radio.setRetries(15, 15);
  radio.setCRCLength(RF24_CRC_16);
  radio.powerUp();

  radio.openReadingPipe(1, pipe);
  radio.startListening();
  
  if(debug) {
    radio.printDetails();
  }
}

void loop() {
  char receivedData[32] = "";
  int len = 0;

  // Wait data on transmitter
  if (radio.available()) {
    bool done = false;
    while ( !done ) {
      len = radio.getDynamicPayloadSize();
      done = radio.read(&receivedData, len);
      delay(5);
    }

    // Send to com port info
    Serial.println(receivedData); 
  } 
}
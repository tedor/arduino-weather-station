#include <RF22Datagram.h>
#include <RF22.h>
#include <SPI.h>

#define SERVER_ADDRESS 10               // server address number
const bool debug = false;               // enable/disable debug mode
char* checkCode = "your_checksum";      // check correct recived string

// SI4432 init
RF22Datagram rf22(SERVER_ADDRESS);

uint8_t receivedChecksum[9];
uint8_t receivedData[32];
char* requestData;

void setup() {
  Serial.begin(9600);
  if (!rf22.init() && debug) {
    Serial.println("RF22 init failed");
  }
  
  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  rf22.setTxPower(RF22_TXPOW_11DBM);
}

void loop() {
  // Wait for a message addressed to us
  rf22.waitAvailable();
  
  uint8_t lenChecksum = sizeof(receivedChecksum);
  uint8_t lenData = sizeof(receivedData);
  uint8_t from;
  uint8_t to;    
  if (rf22.recvfrom(receivedChecksum, &lenChecksum, &from, &to)) {
    if (strcmp((char*)receivedChecksum, checkCode) == 0) {
      rf22.waitAvailableTimeout(500);
      if (rf22.recvfrom(receivedData, &lenData, &from, &to)) {
        // Send a reply back to the originator
        requestData = "OK";
        rf22.sendto((uint8_t*)requestData, 2, from);
        rf22.waitPacketSent(200);

        delay(1);

        // Send to com port info
        Serial.println((char*)receivedData);
      }
    }
  }
}
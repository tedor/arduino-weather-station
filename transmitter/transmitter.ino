#include <SPI.h>
#include <stdio.h>  
#include <stdlib.h>
#include <sdelay.h>
#include <math.h>

const int deviceNumber = 1;             // info about device number
const unsigned long sleepTime = 300000; // sleep time - 5 min
const bool debug = false;               // enable/disable debug mode
const uint64_t pipe = 0xF0A1B2C3D1LL;   // pipe number for transmitter

// NRF24 init
#include <RF24.h>
#include <nRF24L01.h>
#include "printf.h"
#define NRF24_CE 8
#define NRF24_CS 9
RF24 radio(NRF24_CE, NRF24_CS);

// BMP085 init
#include <Wire.h>
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

// DHT22 init
#include "DHT.h"
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Conv to mmHg
const float convMmHgCoef = 0.0075;

// Votage define
const long InternalReferenceVoltage = 1062;

void setup() {
  if(debug) {
    Serial.begin(9600);  
  }

  // Init DHT22 device
  dht.begin();

  // Init BMP085 device
  bmp.begin();

  // Init voltage read
  getBandgap();

  // Init nRF24 radio device
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
  radio.openWritingPipe(pipe);
  radio.powerUp();  
  if(debug) {
    radio.printDetails();
  }

  delay(2000);
}

void loop() {
  // Init vars
  char temperatureString[8] = "";
  char* temperatureNegativeString = "";
  char pressureString[32] = "";
  char humidityString[8] = "";
  char batteryVoltageString[8] = "";  
  char sendData[32] = "";

  // Read data
  int pressure = round(bmp.readPressure() * convMmHgCoef);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float batteryVoltage = getBandgap();
  
  if(temperature < 0) {
    temperature = -temperature;
    temperatureNegativeString = "-";
  }
  // Conver values to string
  dtostrf(temperature, 3, 1, temperatureString);  
  itoa(pressure, pressureString, 10);
  dtostrf(humidity, 3, 1, humidityString);
  dtostrf(batteryVoltage, 3, 2, batteryVoltageString);

  // Create send string
  sprintf(sendData, "%i|t:%s%s,p:%s,h:%s,bv:%s", deviceNumber, temperatureNegativeString, temperatureString, pressureString, humidityString, batteryVoltageString);

  // Wake up
  radio.powerUp();
  
  // Send data string
  bool sendOk = radio.write(&sendData, strlen(sendData));
  if(debug) {
    if (sendOk) {
      Serial.println("Send - ok");
    } else {
      Serial.println("Send - failed");
    }

    Serial.println(sendData);
    delay(100);
  }

  // Sleep
  radio.powerDown();
  sdelay(sleepTime);
}

float getBandgap () {
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG) 
   ADMUX = _BV (REFS0) | _BV (MUX3) | _BV (MUX2) | _BV (MUX1);
   ADCSRA |= _BV( ADSC );  // start conversion
   while (ADCSRA & _BV (ADSC)){}  // wait for conversion to complete
   float results = (((InternalReferenceVoltage * 1024) / ADC) + 5) / 10;
   results = results / 100;
   return results;
} 

#include <SPI.h>
#include <stdio.h>  
#include <stdlib.h>
#include <sdelay.h>
#include <math.h>

#define WAKING_UP_TIME 65
#define SERVER_ADDRESS 10              // server address number
#define CLIENT_ADDRESS 20              // client address number
#define SLEEP_TIME 288995              // sleep time - 5 min
const bool debug = false;              // enable/disable debug mode
const int sensorPowerPin = A1;         // sensors off/on power pin
const int radioPowerPin = 8;           // radio on/off power pinu

uint8_t checkCode[] = "your_checksum";

// SI4432 initu
#include <RF22Datagram.h>
#include <RF22.h>
#include <SPI.h>
RF22Datagram rf22(CLIENT_ADDRESS);
uint8_t receivedData[32];

// BMP085 init
#include <Wire.h>
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

// DHT22 init
#include <dht.h>
#define DHTPIN 7
#define DHTTYPE DHT22
dht DHT;

// Conv to mmHg
const float convMmHgCoef = 0.0075;

void(* resetFunc) (void) = 0;

void setup() {
  if(debug) {
    Serial.begin(9600);  
    Serial.println("setup");
    delay(10);
  }

  // Init voltage read
  getBandgap();
}

void loop() {
  // Power on sensors and sleep with power down mode
  pinMode(sensorPowerPin, OUTPUT);
  digitalWrite(sensorPowerPin, HIGH);

  // Sleep 2s for prepare DTH sensor 
  sleepWithWDT(WDTO_2S);

  // Init BMP085 device
  bmp.begin();

  // Read DHT sensor
  int repeat = 5;
  while (repeat) {
    int chk = DHT.read22(DHTPIN);
    if (chk == DHTLIB_OK) {
      repeat = 0;
    } else {
      repeat--;
      delay(100);
    }
  }

  // Init vars
  char temperatureString[8] = "";
  char* temperatureNegativeString = "";
  char pressureString[32] = "";
  char humidityString[8] = "";
  char batteryVoltageString[8] = "";  
  char sendData[32] = "";

  // Read data
  int pressure = round(bmp.readPressure() * convMmHgCoef);
  float temperature = DHT.temperature;  
  float humidity = DHT.humidity;
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
  sprintf(sendData, "%i|t:%s%s,p:%s,h:%s,bv:%s", CLIENT_ADDRESS, temperatureNegativeString, temperatureString, pressureString, humidityString, batteryVoltageString);

  if (debug) {
    Serial.println(sendData);
    delay(100);
  }

  // Wake up radio module
  pinMode(radioPowerPin, OUTPUT);
  digitalWrite(radioPowerPin, 1);

  // Init SI4432 radio device
  if (!rf22.init() && debug) {
    Serial.println("RF22 init failed");
  }

  rf22.setModemConfig(RF22::GFSK_Rb2Fd5);
  rf22.setTxPower(RF22_TXPOW_11DBM);

  int i = 5;

  while(i--) {
    rf22.sendto(checkCode, sizeof(checkCode), SERVER_ADDRESS);
    rf22.waitPacketSent(500);
    delay(1);

    rf22.sendto((uint8_t*) sendData, sizeof(sendData), SERVER_ADDRESS);
    rf22.waitPacketSent(500);
    delay(1);

    if (rf22.waitAvailableTimeout(500)) {
      // Should be a message for us now  
      if (debug) {
        Serial.println("Transmit - done");
      }
      uint8_t len = sizeof(receivedData);
      if (rf22.recv(receivedData, &len)) {
        if (strcmp((char*)receivedData, "OK") == 0) {
          i = 0;
        }
      } 
    } else {
      if (debug) {
        Serial.println("Transmit - retry");
      }
      delay(200);
    }
  }
  
  // Radio model power off 
  digitalWrite(radioPowerPin, LOW);

  // Sensors power off
  digitalWrite(sensorPowerPin, LOW);

  if (debug) {
    Serial.println("sleep");
    delay(100);
  }

  // Sleep
  sleepNow();
}

void sleepNow()
{
  // Power off si4432 module 
  digitalWrite(10, HIGH);
  pinMode(13, INPUT); 

  // Power off dht22 module
  pinMode(DHTPIN, INPUT); 
  digitalWrite(DHTPIN, HIGH);

  // Power off bmp085 module
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  // Radio model power off 
  digitalWrite(radioPowerPin, LOW);

  // Sensors power off
  digitalWrite(sensorPowerPin, LOW);

  // disable ADC
  ADCSRA = 0;  

  unsigned long sleepPeriod = SLEEP_TIME;

  while (sleepPeriod >= 8192 + WAKING_UP_TIME) { 
    sleepWithWDT(WDTO_8S);   
    sleepPeriod -= 8192 + WAKING_UP_TIME; 
  }
}

float getBandgap () 
{
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  float data = (high<<8) | low;
 
  float result = 1125300 / data / 1000;
  return result; // Vcc in volts
} 

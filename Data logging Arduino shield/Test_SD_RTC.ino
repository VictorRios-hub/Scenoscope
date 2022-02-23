/*
Appli de localisation et de monitoring du niveau sonore
*/

// include the necessary libraries
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU9250.h"
#include "BMP180.h"
#include <SoftwareSerial.h> //Used for transmitting to the device
#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module

MPU9250 accelgyro;
I2Cdev   I2C_M;

SoftwareSerial softSerial(2, 3); //RX, TX

RFID nano; //Create instance

RTC_PCF8523 rtc;
 
// pin definition for the Uno
#define sd_cs  10
#define LED_R  6
#define LED_G  7

#define SW_SAVE_EPC   0
#define SW_SAVE_IMU   0
#define SW_AUTO_START 1
#define SW_SET_RTC    0

#define NB_POINTS 1000

int sw_start=0;

void setup() {
  
  // initialize the serial port
  // it will be used to print some diagnostic info
  Serial.begin(38400);

  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println();
  Serial.println("Initializing...");

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (! rtc.initialized() || rtc.lostPower() || SW_SET_RTC) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  
  // try to access the SD card. If that fails (e.g.
  // no card present), the setup process will stop.
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(sd_cs)) 
  {
    Serial.println(F("failed!"));
    digitalWrite(LED_R,HIGH);
    return;
  }
  Serial.println(F("OK!"));

  if(SW_AUTO_START)
  {
    sw_start=1;
  }
  
  delay(3000);
  Serial.println("     ");

}

void loop() 
{
  //acquire data periodically if sw_start = 1
  if(sw_start)
  {
    GetOneSample();
  }
  delay(2000);
}

void GetOneSample()
{
    digitalWrite(LED_G,HIGH);
    
    PrintOut();
    digitalWrite(LED_G,LOW);
}

void PrintOut()
{
  String strData = "";
  DateTime now = rtc.now();

  //Display time
  strData = String(now.day());
  strData += "/";
  strData += String(now.month());
  strData += "/";
  strData += String(now.year());
  strData += ";";
  strData += String(now.hour());
  strData += ":";
  strData += String(now.minute());
  strData += ":";
  strData += String(now.second());
  strData += ";";
  //Save into SD card
  saveintoSD(strData);
  Serial.println(strData);
}


void saveintoSD(String strVal)
{ 
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) 
  {
    Serial.println(F("Data recorded"));
    dataFile.println(strVal);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else 
  {
    Serial.println("error opening datalog.csv");
  }
}

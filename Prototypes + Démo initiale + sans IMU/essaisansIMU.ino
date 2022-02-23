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

SoftwareSerial softSerial(A14, A15); //RX, TX //Hardware : TX0 à A14 et RXI à A15

RFID nano; //Create instance

RTC_PCF8523 rtc;
 
// pin definition for the Uno
#define sd_cs  10
#define LED_R  6
#define LED_G  7

#define SW_SAVE_EPC   1
#define SW_SAVE_IMU   0
#define SW_AUTO_START 1
#define SW_SET_RTC    0


int sw_start=0;

uint8_t buffer_m[6];

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t mx, my, mz;

float Axyz[3];
float Gxyz[3];
float Mxyz[3];

void setup() {

  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();
  
  // initialize the Serial port
  // it will be used to print some diagnostic info
  Serial.begin(38400);

  while (!Serial) {
    // wait for Serial port to connect. Needed for native USB port only
  }

  Serial.println();
  Serial.println("Initializing...");
  
  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println("Module failed to respond. Please check wiring.");
    while (1); //Freeze!
  }

  nano.setRegion(REGION_EUROPE); //Set to Europe

  nano.setReadPower(2000); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  nano.enableDebugging(Serial);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (! rtc.initialized() || rtc.lostPower() || SW_SET_RTC) {
    //Serial.println("RTC is NOT initialized, let's set the time!");
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
  Serial.println("Start");
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
  String strEPClist="";

  digitalWrite(LED_G,HIGH);
  if(SW_SAVE_EPC==1)
  {
    Serial.println(F("Start EPC"));
    strEPClist=GetEPCNearby();
  }
  PrintOut(strEPClist);
  digitalWrite(LED_G,LOW);
}

void PrintOut(String strEPClist)
{
  String strData = "";
  DateTime now = rtc.now();
  Serial.println(strEPClist);

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
  //Display Tags IDs
  strData += strEPClist;
  //Save into SD card
  Serial.println(strData);
  saveintoSD(strData);

}

String GetEPCNearby()
{
  String strData = "";
  byte myEPC[12]; //Most EPCs are 12 bytes
  byte myEPClength;
  byte responseType = 0; 

  myEPClength = sizeof(myEPC); //Length of EPC is modified each time .readTagEPC is called

  Serial.println(F("Searching for tag"));
  responseType = nano.readTagEPC(myEPC, myEPClength, 500); //Scan for a new tag up to 2000ms
  int x=0;
  int i=0;
  String str="";
  if (responseType == RESPONSE_SUCCESS)
  {
    int rssi = nano.getTagRSSI(); //Get the RSSI for this tag read
    while(x < 12)
    {
      str+=String(myEPC[x],HEX);
      str+=":";
      x++;
    }
    strData+=str;
    strData+=";";
    strData+=String(rssi);
    strData+=";";
    
  }
    
  else
  {
    Serial.println("timeout!");
  }
  Serial.println(strData);
  return strData;
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
    Serial.println(strVal);
    dataFile.println(strVal);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else 
  {
    Serial.println("error opening datalog.csv");
  }
}

boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while(!softSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while(softSerial.available()) softSerial.read();
  
  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a continuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200
    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}

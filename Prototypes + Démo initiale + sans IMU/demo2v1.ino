/*
Appli de localisation et de monitoring du niveau sonore
Attention !! : La fonction delay ne fonctionne plus après le démarrage du RTC !!
*/

// include the necessary libraries
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include "MPU9250.h"

RTC_PCF8523 rtc;
// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
MPU9250 IMU(Wire,0x69);//SDO high state otherwise conflict with PCF8523 on the same I2C bus
 
// pin definition for the Uno
#define sd_cs  10
#define button 8
#define LED_R  6
#define LED_G  7

#define SW_SAVE_EPC   1
#define SW_SAVE_SOUND 0
#define SW_NOISE_CAL  0
#define SW_SAVE_IMU   1
#define SW_AUTO_START 1
#define SW_SET_RTC    0

#define NB_POINTS 1000
#define LOW_NOISE_LEVEL 640

int noisefloor=LOW_NOISE_LEVEL;
int Tabpoints[NB_POINTS]={0};
int cp=0;
int cpsample=0;
int sw_start=0;
int status;

unsigned char Tab[19] = {0x02,0x00,0x0C,0x00,0x00,0x00,0x08,0x00,0x11,0xAA,0x55,0x00,0x04,0x01,0x00,0x00,0x00,0xb7,0xb9};//inventory with RSSI

// char array to print to the screen
char sensorPrintout[6];

void setup() {
  //////////////////////////////////////////////////////
  // initialize the serial port
  //////////////////////////////////////////////////////
  // it will be used to print some diagnostic info
  Serial.begin(38400);
  Serial1.begin(38400);
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }
  while (!Serial1) {
    // wait for serial port to connect. Needed for native USB port only
  }

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
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  pinMode(button, INPUT_PULLUP);
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

    // start communication with IMU 
  status = IMU.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    digitalWrite(LED_R,HIGH);
    while(1);
  }
  
  //////////////////////////////////////////////////////
  // init sound sensor
  //////////////////////////////////////////////////////
  if(SW_NOISE_CAL)
  {
    noisefloor = analogRead(A0);//to remove fake value
    delay(1000);
    noisefloor = analogRead(A0);
    Serial.print("noise floor: ");
    Serial.println(noisefloor);
  }

  if(SW_AUTO_START)
  {
    sw_start=1;
  }
  
  delay(3000);
  cp=0;
}

void loop() 
{
  // don't do anything if the image wasn't loaded correctly.
  // Read the value of the sensor on A0
  //wait for triger to start logging 
  if(digitalRead(button)==0)
  {
    if(sw_start==0)
    { 
      sw_start = 1;
      Serial.println("Start datalogging...");
      saveintoSD(String("Start datalogging..."));  
      
    }
    else 
    {
      sw_start = 0;
      Serial.println("Stop datalogging...");
      saveintoSD(String("Stop datalogging..."));
    }
    delay(1000);
  }

  //acquire data periodically if sw_start = 1
  if(sw_start)
  {
    GetOneSample();
  }
  delay(1);
}

void GetOneSample()
{
  int sensorVal = 0;
  String strValpower="";
  String strEPClist="";
  String strIMUval="";

  //read sensor periodicaly
  sensorVal = analogRead(A0);

  if(cp>=NB_POINTS)
  {
    digitalWrite(LED_G,HIGH);
    if(SW_SAVE_SOUND==1)
    {
      strValpower=calcpower();
    }
    if(SW_SAVE_EPC==1)
    {
      strEPClist=GetEPCNearby();
    }
    if(SW_SAVE_IMU==1)
    {
      strIMUval=GetIMUData();
    }
    cpsample=cpsample+1;
    PrintOut(strValpower,strIMUval, strEPClist);
    cp=0;
    digitalWrite(LED_G,LOW);
  }
  else
  {
    // read the input on analog pin 0:
    sensorVal = analogRead(A0);
    Tabpoints[cp]=sensorVal;
    cp=cp+1;
  }  
}

void PrintOut(String strValpower, String strIMUval, String strEPClist)
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
  //Display sound level
  strData += strValpower;
  //Display IMU data
  strData += strIMUval;
  //Display Tags IDs
  strData += strEPClist;
  //Save into SD card
  saveintoSD(strData);
  Serial.println(strData);
}

String GetEPCNearby()
{
  String strData = "";
  // Send command to the reader
  for(int i=0;i<19;i++)
  {
    Serial1.write(Tab[i]);
  }

  // Wait for response from the reader
  int timeout=0;
  while(Serial1.available()==0 && timeout < 2000)
  {
    delay(1);
    timeout++;
  }
  // If something received
  if (Serial1.available())
  {
    // read from port 1, send to port 0:   
    int TabByte[512]={0};
    int Lg=0;
    while (Serial1.available()) 
    {
      TabByte[Lg++] = Serial1.read();
      delay(10);
    }      
    //print reader message
    int NbTag = TabByte[9];
    strData += String(NbTag);
    strData += ";";
    //Check EPC
    int Index=10;
    for(int i=0;i<NbTag;i++)
    {
      int EPClen=TabByte[Index++];
      //the first byte of the EPC is required for the demo
      strData += String(TabByte[Index]);
      strData += ";";
      Index=Index+15;//EPC + ANTID + NBREAD
      int RSSI=TabByte[Index++];
      strData += String(RSSI);
      strData += ";";
    }
  }
  else
  {
    Serial.println("timeout!!");
  }
  return strData;
}

String GetIMUData()
{
  String strData = "";
  //Read the sensor
  IMU.readSensor();
  strData+=String(IMU.getAccelX_mss());
  strData+=";";
  strData+=String(IMU.getAccelY_mss());
  strData+=";";
  strData+=String(IMU.getAccelZ_mss());
  strData+=";";
  strData+=String(IMU.getGyroX_rads());
  strData+=";";
  strData+=String(IMU.getGyroY_rads());
  strData+=";";
  strData+=String(IMU.getGyroZ_rads());
  strData+=";";
  strData+=String(IMU.getMagX_uT());
  strData+=";";
  strData+=String(IMU.getMagY_uT());
  strData+=";";
  strData+=String(IMU.getMagZ_uT());
  strData+=";";
  return strData;
}

String calcpower()
{
  unsigned long valmoy=0;
  for(int ii=0;ii<NB_POINTS;ii++)
  {
    valmoy=valmoy+pow((Tabpoints[ii]-noisefloor),2);
  }
  valmoy=valmoy*50/NB_POINTS;
  if(valmoy>65535)valmoy=5000;
  unsigned int valtosave = (unsigned int)valmoy;
  // print out the value you read:
  String strVal = String(valtosave);
  strVal+=";";
  //displayVal(strVal);//for debug
  return strVal;
}

void saveintoSD(String strVal)
{ 
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) 
  {
    dataFile.println(strVal);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else 
  {
    Serial.println("error opening datalog.csv");
  }
}

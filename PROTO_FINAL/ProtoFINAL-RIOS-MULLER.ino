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

MPU9250 accelgyro (MPU9250_ADDRESS_AD0_HIGH);
I2Cdev   I2C_M;

SoftwareSerial softSerial(A14, A15); //RX, TX //Hardware : TX0 à A14 et RXI à A15

//Table of structures to record EPC and associated RSSI of detected tags
strTagReadData TagDataFiltered[MAX_NB_TAG];

RFID nano; //Create instance

RTC_PCF8523 rtc;
 
// pin definition for the Uno
#define sd_cs  10
#define LED_R  6
#define LED_G  7

#define SW_SAVE_EPC   1
#define SW_SAVE_IMU   1
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
  
  // initialize the serial port
  // it will be used to print some diagnostic info
  Serial.begin(38400);

  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
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

  // initialize device
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU9250 connection successful" : "MPU9250 connection failed");

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
  Serial.println("Start");
}

void loop() 
{
  //acquire data periodically if sw_start = 1
  if(sw_start)
  {
    GetOneSample();
  }
  delay(2000);;
}

void GetOneSample()
{
  String strEPClist="";
  String strIMUval="";

  digitalWrite(LED_G,HIGH);
  if(SW_SAVE_EPC==1)
  {
    Serial.println(F("Start EPC"));
    strEPClist=GetEPCNearby();
  }
  if(SW_SAVE_IMU==1)
  {
    Serial.println(F("Start IMU"));
    strIMUval=GetIMUData();
  }
  PrintOut(strIMUval, strEPClist);
  digitalWrite(LED_G,LOW);
}

void PrintOut(String strIMUval, String strEPClist)
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
  String str="";
  uint8_t NbTag = GetTagsfilterMultipleEpc();
  
  Serial.print(F("Searching for tags : "));
  Serial.println(NbTag);
  
  strData+=NbTag;
  strData+=";";
  
  for(int i=0;i<NbTag;i++)
  {
    str="";
    for(int j=0;j<2;j++)
    {
      if (0 <= (nano.TagData[i].epcData[j],HEX) <= 15)
      {
        str+='0';
      }
      str+=String(nano.TagData[i].epcData[j],HEX); 
    }
    
    strData+=String(hexToDec(str));
    strData+=";";
    strData+=String(nano.TagData[i].rssi);
    strData+=";";  
  }
  Serial.println(strData);
  return strData;
}

unsigned int hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}

//////////////////////////////////////////////////////
// Get EPC of RFID tags nearby
// Filter EPC if detected several times
//////////////////////////////////////////////////////
uint8_t GetTagsfilterMultipleEpc(void)
{
  uint8_t NbTag=0;

  memset(TagDataFiltered,0,sizeof(strTagReadData)*MAX_NB_TAG);

  nano.startReadingSingle(200); //Begin scanning for tags during 200ms
  
  uint8_t NbTagRead = nano.cmdGetTagsRemaining();

  for(int i=0;i<NbTagRead;i++)
  {
    if(checkSameEPC(nano.TagData[i].epcData)==0)
    {
      memcpy(&TagDataFiltered[NbTag],&nano.TagData[i],sizeof(strTagReadData));
      NbTag++;
    }
  }
  return NbTag;
}

//////////////////////////////////////////////////////
// Check if the EPC is already recorded in the
// table TagDataFiltered. 
//////////////////////////////////////////////////////
uint8_t checkSameEPC(uint8_t epc[12])
{
  for(int i=0;i<MAX_NB_TAG;i++)
  {
    if(memcmp(epc,TagDataFiltered[i].epcData,12)==0)return 1;
  }
  return 0;
}

String GetIMUData()
{
  String strData = "";
  //Read the sensor
  getAccel_Data();
  getGyro_Data();
  getCompass_Data();

  //Acceleration(g) of X,Y,Z
  strData+=String(Axyz[0]); 
  strData+=";";
  strData+=String(Axyz[1]); 
  strData+=";";
  strData+=String(Axyz[2]); 
  strData+=";";
  //Gyro(degress/s) of X,Y,Z
  strData+=String(Gxyz[0]);
  strData+=";";
  strData+=String(Gxyz[1]); 
  strData+=";";
  strData+=String(Gxyz[2]);
  strData+=";";
  //Compass Value of X,Y,Z (Magneto)
  strData+=String(Mxyz[0]);
  strData+=";";
  strData+=String(Mxyz[1]); 
  strData+=";";
  strData+=String(Mxyz[2]);
  strData+=";";
  return strData;
}

void getAccel_Data(void)
{
    accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
    Axyz[0] = (double) ax / 16384;
    Axyz[1] = (double) ay / 16384;
    Axyz[2] = (double) az / 16384;
}

void getGyro_Data(void)
{
    accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
    Gxyz[0] = (double) gx * 250 / 32768;
    Gxyz[1] = (double) gy * 250 / 32768;
    Gxyz[2] = (double) gz * 250 / 32768;
}

void getCompass_Data(void)
{
    I2C_M.writeByte(MPU9150_RA_MAG_ADDRESS, 0x0A, 0x01); //enable the magnetometer
    delay(10);
    I2C_M.readBytes(MPU9150_RA_MAG_ADDRESS, MPU9150_RA_MAG_XOUT_L, 6, buffer_m);

    mx = ((int16_t)(buffer_m[1]) << 8) | buffer_m[0] ;
    my = ((int16_t)(buffer_m[3]) << 8) | buffer_m[2] ;
    mz = ((int16_t)(buffer_m[5]) << 8) | buffer_m[4] ;

    Mxyz[0] = (double) mx * 1200 / 4096;
    Mxyz[1] = (double) my * 1200 / 4096;
    Mxyz[2] = (double) mz * 1200 / 4096;
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

boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while (softSerial.isListening() == false); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (softSerial.available()) softSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
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

    delay(250);
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}

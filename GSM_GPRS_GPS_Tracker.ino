/*


 - GGGGGG - PPPPPP -- SSSSS ------- //  TTTTTTTT --------------------------- KK - © Antonis Maglaras -
  HG    GG  PP   PP  SS   SS       //   T  TT  T                             KK
  GG        PP   PP  SS           //       TT     RR RRR    AAAA     CCCCC   KK  KK   EEEEE   RR RRRR
  GG  GGGG  PPPPPP    SSSSS      //        TT      RRR RR      AA   CC   CC  KK KK   EE   EE   RRR  RR
  GG    GG  PP            SS    //         TT      RR       AAAAA   CC       KKKK    EEEEEE    RR
  GG    GG  PP       SS   SS   //          TT      RR      AA  AA   CC   CC  KK KK   EE        RR
   GGGGGG   PP        SSSSS   //          TTTT    RRRR      AAAA A   CCCCC   KK  KK   EEEEE   RRRR


  GPS Tracker (over GPRS)
  Version 2.01 (alpha)
  (c)2013, Antonis Maglaras
  
  For use with Arduino Leonardo, GSM Shield & Serial NMEA GPS (Fastrax UP501)
  
  Libraries:
  GSM Library: TDG (ALPHA_TiDiGino_IDE100_v100.zip) -- http://www.gsmlib.org/download.html
  GPS Library: TinyGPS -- http://arduiniana.org/libraries/tinygps/
  
*/



#ifdef SHOWMEMORY
  #include <MemoryFree.h>  
#endif
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "inetGSM.h"
#include "sms.h"
#include "call.h"
#include <TinyGPS.h>

InetGSM inet;
CallGSM call;
SMSGSM sms;
TinyGPS gps;

SoftwareSerial gpsserial(11, 12);                                      // Software Serial for GPS on pins 11 & 12
static void gpsdump(TinyGPS &gps);           
static bool feedgps();
int currentSats=0,currentAlt=0,currentSpeed=0,currentCourse=0;         // Keep the current & previous location, speed, stats etc.
float currentLat=0.0,currentLon=0.0,previousLat=1.0,previousLon=1.0;   // --//--
char currentTime[18];                                                  // Decoded time from GPS
boolean Bypass = true;
char msg[50];
int numdata;
char inSerial[50];
int i=0;
boolean started=false;
int Times; 
float currentTemp=0.0,SumTemp=0.0;
long GPSMillis, GPRSMillis;
#ifdef SHOWMEMORY
  long MemoryMillis;
#endif


#define SoftPowerPin       2                                           // GSM Shield Soft-Power-on Pin
#define ButtonPin         13                                           // Button for sending SMS
#define REDLed             8                                           // RED LED Pin
#define GREENLed           9                                           // GREEN LED Pin
#define YELLOWLed         10                                           // YELLOW LED Pin
#define LM35Pin           A3                                           // LM35 Temperature Sensor Pin
#define WHITELed          A1                                           // WHITE LED Pin
#define PulseInPin        A2                                           // Pulse In from GPS

#define MyPhoneNumber     "+3069XXXXXXXX"                              // My phone number. Used for SMS, CLID etc.
#define APN               "internet.vodafone.gr"                       // APN.
#define APNUser           "user"                                       // APN Username.
#define APNPass           "pass"                                       // APN Password.
#define HOSTNAME          "XXX.XXXXXX.XX"

#define EnableGPS         true                                         // GPS enabled/not
#define GPSDebug          false                                        // Display GPS data for debugging.
#define LM35Debug         false                                        // Display Temperature for debugging.
#define Debug             true                                         // Debug Mode -- Outputs data...  





void setup() 
{
  delay(5000);                                                    // Delay on start
  pinMode(REDLed, OUTPUT);                                        // Pins Setup
  pinMode(GREENLed, OUTPUT);
  pinMode(YELLOWLed, OUTPUT);
  pinMode(WHITELed, OUTPUT);
  pinMode(PulseInPin, INPUT);
  pinMode(SoftPowerPin, OUTPUT);
  pinMode(ButtonPin, INPUT);
  delay(500);
  if (digitalRead(ButtonPin)==HIGH)                               // If button is missing, disable button for extra security
    Bypass = true;
  else
    Bypass = false;
  if (Debug)    
    Serial.begin(9600);                                           // Serial Console Input/Output    
  if (EnableGPS)                                                  // If GPS is enabled, setup GPS 
  {
    if (Debug)
    {
      Serial.println("+-----------------------------+");
      Serial.println("| GPS Tracker (over GSM/GPRS) |");
      Serial.println("|  (c)2013, Antonis Maglaras  |");
      Serial.println("+-----------------------------+");
      Serial.println();
      Serial.print("-- GPS Setup...");
    }
    gpsserial.begin(9600);
    gpsserial.println("$PMTK301,2*2E");                           // Enable DGPS = WAAS
    delay(1000);
    gpsserial.println("$PMTK397,0.4*39");                         // Do not count speed less than 0,4m/s (1.4km/h)
    delay(500);
    if (Debug)
      Serial.println("Done!");
  }
  if (Debug)
    Serial.print("-- Power on GSM...");
  digitalWrite(SoftPowerPin,LOW);
  delay(100);
  digitalWrite(SoftPowerPin,HIGH);
  delay(2000);
  digitalWrite(SoftPowerPin,LOW);
  if (Debug)
    Serial.println("Done!");
  started=false;
  while (!started)
  {
    if (Debug)
      Serial.print("-- GSM Setup...");   
    if (gsm.begin(9600))                                            // Start GSM communications
    {            
      if (Debug)      
        Serial.println("Done!");
      started=true;  
    }
    else 
    if (Debug)
      Serial.println("Problem! Retrying...");
  }
  GPSMillis=millis();
  GPRSMillis=millis();
#ifdef SHOWMEMORY
  MemoryMillis=millis();
#endif
};





void loop() 
{
#ifdef SHOWMEMORY
    if ((millis()-MemoryMillis)>3000)
    {
      Serial.print("- Free memory: ");
      Serial.print(freeMemory());
      Serial.println(" bytes");
      MemoryMillis=millis();
    }
#endif

//  if (call.CallStatus()==CALL_INCOM_VOICE)
//  {
//    digitalWrite(WHITELed, HIGH);
//  }
//  else
//  {
//    digitalWrite(WHITELed, LOW);
//  }
    
  if (EnableGPS)                                                  // GPS data processing
  {
    bool newdata = false;
    if (millis()-GPSMillis<1000)
    {
      if (feedgps())
        newdata=true;
      gpsdump(gps);
    }
    GPSMillis=millis();
  }
  
  GetTemperature();                                              // Get temperature readings
  if (Times>1000)                                                // Every 1000 times, reset average readings
  {
    if ((LM35Debug) && (Debug))
    {
      Serial.print(" - Temperature: ");
      Serial.println(currentTemp,1);
    }
    Times=0;
    SumTemp=0.0;
  }
  
  if ((millis()-GPRSMillis>90000) && (currentLat != previousLat) && (currentLon != previousLon))                // Upload position data via GPRS 
  {
    digitalWrite(YELLOWLed, HIGH);
    UpdateOverGPRS();                                            
    digitalWrite(YELLOWLed, LOW);
    previousLat=currentLat;
    previousLon=currentLon;
    GPRSMillis=millis();
  }

  if (Debug)
  {  
    serialhwread();                                              // Read for new byte on serial hardware and write them on NewSoftSerial.
    serialswread();                                              // Read for new byte on NewSoftSerial.
  }
};





void serialhwread()
{
  i=0;
  if (Serial.available() > 0){            
    while (Serial.available() > 0) 
    {
      inSerial[i]=(Serial.read());
//      delay(10);
      i++;      
    }
    
    inSerial[i]='\0';
    if(!strcmp(inSerial,"/END")){
      Serial.println("_");
      inSerial[0]=0x1a;
      inSerial[1]='\0';
      gsm.SimpleWriteln(inSerial);
    }
    //Send a saved AT command using serial port.
    if(!strcmp(inSerial,"TEST"))
    {
      Serial.println("SIGNAL QUALITY");
      gsm.SimpleWriteln("AT+CSQ");
    }
    //Read last message saved.
    if(!strcmp(inSerial,"MSG"))
    {
      Serial.println(msg);
    }
    else
    {
      Serial.println(inSerial);
      gsm.SimpleWriteln(inSerial);
    }    
    inSerial[0]='\0';
  }
}



void serialswread()
{
  gsm.SimpleRead();
}




//************************************************************************************************************************
// GPSdump Procedure. 
// Process data (NMEA) received from GPS.
//************************************************************************************************************************

static void gpsdump(TinyGPS &gps)
{
  // Get GPS Satellites. If less than 3 means no fix, so return If it's 255, we have also no fix (cold-start).
  int Sats = gps.satellites();
  if (Sats<255)
  {
    if (Sats>=3)
      digitalWrite(GREENLed, HIGH);                              // Green LED On when valid GPS location
    else
    {
      digitalWrite(GREENLed, LOW);                               // Green LED off when not valid GPS location
      return;                                                    // Exit.
    }
  }
  else
  {
    digitalWrite(GREENLed, LOW);                                 // Green LED off. No signal yet.
    return;                                                      // Exit
  }
  float flat, flon;
  unsigned long age;
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  // DST Correction -- last sunday of march
  int beginDSTDate=  (31 - (5* year /4 + 4) % 7);
  int beginDSTMonth=3;
  //last sunday of october
  int endDSTDate= (31 - (5 * year /4 + 1) % 7);
  int endDSTMonth=10;
  // DST is valid as:
  if (((month > beginDSTMonth) && (month < endDSTMonth)) || ((month == beginDSTMonth) && (day >= beginDSTDate)) || ((month == endDSTMonth) && (day <= endDSTDate)))
    hour=hour+3;  // DST europe = utc +3 hour
  else
    hour=hour+2; // nonDST europe = utc +2 hour
  sprintf(currentTime,"%02d/%02d/%02d %02d:%02d:%02d", day, month, year, hour, minute, second);
  currentSats=Sats;
  gps.f_get_position(&flat, &flon, &age);
  currentLat=flat;
  currentLon=flon;
  currentAlt=gps.f_altitude();
  currentSpeed=gps.f_speed_kmph();
  currentCourse=gps.f_course();
  if ((GPSDebug) && (Debug))                                           // Show GPS data for debugging
  {
    Serial.print("- [GPS] Lat: ");
    Serial.print(currentLat,7);
    Serial.print(" - Lon: ");
    Serial.print(currentLon,7);
    Serial.print(" - Speed: ");
    Serial.print(currentSpeed);
    Serial.print("km/h - Course: ");
    Serial.print(currentCourse);
    Serial.print(" - Altitude: ");
    Serial.print(currentAlt);
    Serial.print(" - Satellites: ");
    Serial.println(currentSats);
  }
}




//************************************************************************************************************************
// FeedGPS Procedure. 
// Feed GPS routine with NMEA data from Serial Port.
//************************************************************************************************************************

static bool feedgps()
{
  while (gpsserial.available())
  {
    digitalWrite(REDLed, HIGH);             // Turn on GPSLed to indicate receiving of data
    if (gps.encode(gpsserial.read()))
    {
      digitalWrite(REDLed, LOW);            // Turn off GPSLed when are OK
      return true;
    }
  }
  digitalWrite(REDLed, LOW);                // Turn off GPSLed when not receiving data
  return false;
}



//************************************************************************************************************************
// GetTemperature Procedure. 
// Read temperature (and filter the data).
//************************************************************************************************************************

void GetTemperature()
{
  Times+=1;
  SumTemp+=(analogRead(LM35Pin) * 5.0 * 100.0) / 1024.0;
  currentTemp=SumTemp/Times;  
}











void UpdateOverGPRS()
{
  if (currentSats>=3)
  {
    if(started)
    {
      if (Debug)
        Serial.print("-- Starting GPRS Update...");
      String url="/track.php?lat=";
      url+=printFloat(currentLat,7);
      url+="&lon=";
      url+=printFloat(currentLon,7);
      url+="&wra=";
      url+=currentTime;
      url+="&sats=";
      url+=currentSats;
      url+="&speed=";
      url+=currentSpeed;
      url+="&course=";
      url+=currentCourse;
      url+="&alt=";
      url+=currentAlt;
      url+="&temp=";
      url+=printFloat(currentTemp,1);      
      
      //GPRS attach, put in order APN, username and password.
      //If no needed auth let them blank.
      if (inet.attachGPRS(APN, APNUser, APNPass))
      {
        if (Debug)
          Serial.println("[APN Set]");
      }
      else 
      {
        if (Debug)
          Serial.println("Error! [APN Problem]");
      }
      delay(1000);
      
      //Read IP address.
      gsm.SimpleWriteln("AT+CIFSR");
      delay(5000);
      //Read until serial buffer is empty.
      gsm.WhileSimpleRead();
      if (Debug)
        Serial.print("-- Sending data...");
      //TCP Client GET, send a GET request to the server and
      //save the reply.
      char tmp[url.length()+1];
      url.toCharArray(tmp,url.length()+1);
      numdata=inet.httpGET(HOSTNAME, 80, tmp, msg, 50);
//      //Print the results.
      Serial.println("nNumber of data received:");
      Serial.println(numdata);  
      Serial.println("\nData received:"); 
      Serial.println(msg); 
      if (Debug)
        Serial.println("Done!");
    }
  }
}

	



String printFloat(double number, int digits)
{
  String out="";
  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  out+=int_part;
  out+=".";

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    out+=toPrint;
    remainder -= toPrint; 
  } 
  return out;
}

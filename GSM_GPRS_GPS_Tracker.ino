/*


 - GGGGGG - PPPPPP -- SSSSS ------- //  TTTTTTTT --------------------------- KK - © Antonis Maglaras -
  HG    GG  PP   PP  SS   SS       //   T  TT  T                             KK
  GG        PP   PP  SS           //       TT     RR RRR    AAAA     CCCCC   KK  KK   EEEEE   RR RRRR
  GG  GGGG  PPPPPP    SSSSS      //        TT      RRR RR      AA   CC   CC  KK KK   EE   EE   RRR  RR
  GG    GG  PP            SS    //         TT      RR       AAAAA   CC       KKKK    EEEEEE    RR
  GG    GG  PP       SS   SS   //          TT      RR      AA  AA   CC   CC  KK KK   EE        RR
   GGGGGG   PP        SSSSS   //          TTTT    RRRR      AAAA A   CCCCC   KK  KK   EEEEE   RRRR


*/



// #define SHOWMEMORY                       // Uncomment thins for showing free memory every 3 seconds on console

#ifdef SHOWMEMORY
  #include <MemoryFree.h>  
#endif

#include <SoftwareSerial.h>
#include <TinyGPS.h>

TinyGPS gps;
SoftwareSerial gpsserial(11, 12);                                      // Software Serial for GPS on pins 11 & 12
static void gpsdump(TinyGPS &gps);           
static bool feedgps();
#define MaxBufferSize 64                                               // Buffer size
unsigned char buffer[MaxBufferSize];                                   // Buffer array for data received from GSM modem
int count=0;                                                           // Counter for buffer array 
String msg = "";                                                       // Buffer for processing incoming data from GSM
int currentSats=0,currentAlt=0,currentSpeed=0,currentCourse=0;         // Keep the current & previous location, speed, stats etc.
float currentLat=0.0,currentLon=0.0,previousLat=1.0,previousLon=1.0;   // --//--
int Times;  
float currentTemp=0.0,SumTemp=0.0;
char currentTime[18];                                                  // Decoded time from GPS
long UpdateMillis, StartMillis;         
#ifdef SHOWMEMORY
  long MemoryMillis;
#endif



#define SoftPowerPin       2                                           // GSM Shield Soft-Power-on Pin
#define ButtonPin         13                                           // Button for sending SMS
#define REDLed             8                                           // RED LED Pin
#define GREENLed           9                                           // GREEN LED Pin
#define YELLOWLed         10                                           // YELLOW LED Pin
#define LM35Pin           A3                                           // LM35 Temperature Sensor Pin

#define MyPhoneNumber     "+3069XXXXXXXX"                              // My phone number. Used for SMS, CLID etc.
#define APN               "XXXXXX"                                     // APN.
#define URL               "http://XXX.XXXXX.XX"                        // URL for sending the data over GPRS.

#define GPSOn             true                                         // GPS active or not.
#define GPSTempDebug      false                                        // Display GPS & temperature data for debugging.
#define Debug             false                                        // Show response messages from GMS.
#define ShowMessages      true                                         // Show info messages.



//************************************************************************************************************************
// Setup Procedure. 
// Initialize values.
//************************************************************************************************************************

void setup()  
{
  pinMode(REDLed, OUTPUT);
  pinMode(GREENLed, OUTPUT);
  pinMode(YELLOWLed, OUTPUT);
  pinMode(SoftPowerPin, OUTPUT);
  pinMode(ButtonPin, INPUT);
  if ((ShowMessages) || (Debug) || (GPSTempDebug))
    Serial.begin(19200);
  for (int tmp=0; tmp<3; tmp++)
  {
    delay(150);
    TurnOnLEDs(true);
    delay(150);
    TurnOnLEDs(false);
  }
  if ((ShowMessages) || (Debug))
  {
    Serial.println("+---------------------------+");
    Serial.println("|   GPS GSM/GPRS Tracking   |");
    Serial.println("| (c)2013, Antonis Maglaras |");
    Serial.println("+---------------------------+");
    Serial.println();
    Serial.println("- Power On GSM...");
  }
  Serial1.begin(19200);
  PowerOnOff();
  if (GPSOn)
  {
    gpsserial.begin(9600);
    gpsserial.println("$PMTK301,2*2E");              // Enable DGPS = WAAS
    delay(1000);
    gpsserial.println("$PMTK397,0.4*39");            // Do not count speed less than 0,4m/s (1.4km/h)
    delay(1000);
  }
  UpdateMillis=millis();
  StartMillis=millis();
#ifdef SHOWMEMORY
  MemoryMillis=millis();
#endif
}


//************************************************************************************************************************
// Loop Procedure. 
// Main program.
//************************************************************************************************************************

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
  if ((millis()-UpdateMillis>90000) && (currentSats>=3) && (currentLat != 0.0) && (currentLon != 0.0))     
  // Update every X time via GPRS (when valid gps lon/lat and position has not already sent)
  {
    msg="";
    UpdateOverGPRS();
    UpdateMillis=millis();
  }
  if (digitalRead(ButtonPin)==HIGH)     // On long button press, send sms
  {
    TurnOnLEDs(true);
    delay(500);
    if (digitalRead(ButtonPin)==HIGH)   // Check for long button press
    { 
      msg="";
      SendSMSMessage();
    }
  }
  if (GPSOn)
  {
    bool newdata = false;
    if (millis()-StartMillis<1000)
    {
      if (feedgps())
        newdata=true;
      gpsdump(gps);
    }
    StartMillis=millis();
  }
 ShowSerialData();
 if ((Debug) || (ShowMessages))
 {
   if (Serial.available())               // if data are available on console
     Serial1.write(Serial.read());       // send them to the Serial1 (GSM shield) // to pass commands
 }
  GetTemperature();                      // Get temperature readings
  if (Times>1000)                        // Every 1000 times, reset average readings
  {
    if (GPSTempDebug)
    {
      Serial.print(" - Temperature: ");
      Serial.println(currentTemp,1);
    }
    Times=0;
    SumTemp=0.0;
  }
}



//************************************************************************************************************************
// PowerOnOff Procedure. 
// Send a signal to the shield soft-power pin for turning on/off
//************************************************************************************************************************

void PowerOnOff()                      
{
 digitalWrite(SoftPowerPin,LOW);
 delay(100);
 digitalWrite(SoftPowerPin,HIGH);
 delay(2000);
 digitalWrite(SoftPowerPin,LOW);
}




//************************************************************************************************************************
// AnswerCall Procedure. 
// Answers the current call.
//************************************************************************************************************************

void AnswerCall()
{
  msg="";  
  Serial1.println("ATA");
  delay(500);
  ShowSerialData();
  msg="";  
}



//************************************************************************************************************************
// HangUpCall Procedure. 
// Hungup the current active call.
//************************************************************************************************************************

void HangUpCall()
{
  msg="";
  Serial1.println("+++");
  delay(500);
  Serial1.println("ATH");
  delay(100);
  ShowSerialData();
  msg="";
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
      digitalWrite(GREENLed, HIGH);
    else
      digitalWrite(GREENLed, LOW);    
  }
  else
  {
    digitalWrite(GREENLed, LOW);
    digitalWrite(YELLOWLed, LOW);
    return;
  }
  float flat, flon;
  unsigned long age;
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  // DST Correction
  // last sunday of march
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
  if (GPSTempDebug)
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


//************************************************************************************************************************
// UpdateOverGPRS Procedure. 
// Check for GPRS access and send (http get) the data.
//************************************************************************************************************************

void UpdateOverGPRS()
{
  msg="";
  if ((previousLat==currentLat) && (previousLon==currentLon))
    return;
  TurnOnLEDs(true);
  if ((ShowMessages) || (Debug))
    Serial.println("- Starting GPRS update..."); 
  Serial1.println("AT+CGATT?");
  delay(1000);
  if (HasResponded("CGATT:0"))               // GPRS is not active
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- No GPRS access. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.print("AT+CSTT=\"");
  Serial1.print(APN);
  Serial1.print("\",\"\",\"\"");       // Setup the APN.
//  Serial1.write(0x1A);
  delay(2000); 
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error setting APN. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  ShowSerialData();
  Serial1.println("AT+CIPSRIP=1");
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIICR");  
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIFSR");
//  Serial1.write(0x1A);
  delay(2000); 
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CDNSCFG?");
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIPHEAD=1");
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIPSTATUS");
//  Serial1.write(0x1A);
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.print("AT+CIPSTART=\"TCP\",\"");
  Serial1.print(URL);
  Serial1.println("\",\"80\"");
  Serial1.write(0x1A);
  delay(3000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIPSTATUS");
//  Serial1.write(0x1A);
  delay(2000);
  if (HasResponded("ERROR"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- Error. Discarding...");
    TurnOnLEDs(false);
    return;
  }
  Serial1.println("AT+CIPSEND");
  delay(5000);
  ShowSerialData();
  Serial1.print("GET /track.php?lat=");
  Serial1.print(currentLat,7);
  Serial1.print("&lon=");
  Serial1.print(currentLon,7);
  Serial1.print("&wra=");
  Serial1.print(currentTime);
  Serial1.print("&sats=");
  Serial1.print(currentSats);
  Serial1.print("&speed=");
  Serial1.print(currentSpeed);
  Serial1.print("&course=");
  Serial1.print(currentCourse);
  Serial1.print("&alt=");
  Serial1.print(currentAlt);
  Serial1.print("&temp=");
  Serial1.print(currentTemp,1);
  Serial1.print(" HTTP/1.1");
  Serial1.println("");
  Serial1.println("");
  Serial1.write(0x1A);
  delay(5000);
  ShowSerialData();
  Serial1.println("AT+CIPSHUT");
  delay(1000);
  ShowSerialData();
  previousLat=currentLat;
  previousLon=currentLon;
  TurnOnLEDs(false);
  if ((ShowMessages) || (Debug))
    Serial.println("- Done!");
  msg="";
}


//************************************************************************************************************************
// SendSMSMessage Procedure. 
// Send SMS with location details.
//************************************************************************************************************************

void SendSMSMessage()
{
  msg="";
  if ((ShowMessages) || (Debug))
    Serial.println("- Sending SMS...");
  Serial1.println("AT+CREG?");
  delay(1000);
  if (!HasResponded("0,1"))
  {
    if ((Debug) || (ShowMessages))
      Serial.println("- No network registration. Discarding...");
    return;
  }
  Serial1.println("AT+CMGF=1");
  delay(1000);
  Serial1.println("AT+CSMP=17,167,0,241");
  delay(1000);
  Serial1.print("AT+CMGS=\"");
  Serial1.print(MyPhoneNumber);
  Serial1.println("\"\r");
  delay(1000);
  Serial1.print("Lon: ");
  Serial1.print(currentLon,7);
  Serial1.print("\r\n");
  Serial1.print("Lat: ");
  Serial1.print(currentLat,7);
  Serial1.print("\r\n");
  Serial1.print("Speed: ");
  Serial1.print(currentSpeed);
  Serial1.print("km/h\r\n");
  Serial1.print("Course: ");
  Serial1.print(currentCourse);
  Serial1.print("\r\n");
  Serial1.print("Altitude: ");
  Serial1.print(currentAlt);
  Serial1.print("m\r\n");
  Serial1.print("Satellites: ");
  Serial1.print(currentSats);
  Serial1.print("\r\n");
  Serial1.print("Temp: ");
  Serial1.print(currentTemp,1);
  Serial1.print("C\r\n");
  Serial1.println();
  Serial1.write(0x1A);        // Send CTRL-Z
  ShowSerialData(); 
  if ((ShowMessages) || (Debug))
    Serial.println("- Done!");
  msg="";  
  TurnOnLEDs(false);
}


//************************************************************************************************************************
// ShowSerialData Procedure. 
// Display data received from GSM modem.
//************************************************************************************************************************

void ShowSerialData()
{
 if (Serial1.available())              // if we have data from GSM modem
 {
   while(Serial1.available())          
   {
     buffer[count++]=Serial1.read();   // Read the data and put them on buffer
     if (count == MaxBufferSize)
       break;
   }
   count = 0;   
   if (Debug)
     Serial.write(buffer,count);       // When data transmission ends, write buffer to console for debugging.
   clearBufferArray();                 // Call clearBufferArray function to clear the data and make some process.                       
 }
} 


//************************************************************************************************************************
// ClearBufferArray Procedure. 
// Clear data from buffer and process the output.
//************************************************************************************************************************

void clearBufferArray() 
{
  for (int i=0; i<=count; i++)
  { 
    if ((buffer[i] != 10) && (buffer[i]!=13) && (buffer[i]!=32) && (buffer[i]!=0) && (buffer[i]!=26))
      msg+=String(char(buffer[i]));              // Copy the buffer data to string for processing
    buffer[i]=NULL;                              // Empty the buffer
  }
  count = 0;
  if (msg.indexOf("CLIP") >= 0)                  // CallerID detected
  {
    String callerid = msg.substring(msg.indexOf("\"")+1,msg.length());
    callerid = callerid.substring(0,callerid.indexOf("\""));
    if ((ShowMessages) || (Debug))
    {
      Serial.print("- RING! Call from CallerID: ");
      Serial.println(callerid);
    }
    if (callerid==MyPhoneNumber)                  // It's me calling. Send SMS with information.
    {
      TurnOnLEDs(true);      
      if ((ShowMessages) || (Debug))
      {
        Serial.println("- It's me calling!");
        Serial.println("- Hanging Up Call");
      }
      HangUpCall();
      delay(5000);
      SendSMSMessage();
      ShowSerialData();
    }
    msg="";
  }

  if (msg.indexOf("RING") >= 0)                  // RING detected
  {
    msg="";
    if ((ShowMessages) || (Debug))
      Serial.println("- RING! Someone is calling!");
    TurnOnLEDs(true);
  }
  
  if (msg.length()>100)                          // Keep process input buffer < X chars
    msg="";
}



//************************************************************************************************************************
// HasResponded Procedure. 
// Check for the GSM modem response.
//************************************************************************************************************************

boolean HasResponded(String checkForString)
{
  msg="";
  if (Serial1.available())              
  {
    while(Serial1.available())          
    {
      buffer[count++]=Serial1.read();     
      if (count >= MaxBufferSize)
        break;
    }
    if (Debug)
      Serial.write(buffer,count);       
    for (int i=0; i<=count; i++)
    { 
      if ((buffer[i] != 10) && (buffer[i]!=13) && (buffer[i]!=32) && (buffer[i]!=0) && (buffer[i]!=26))
      {
        msg+=String(char(buffer[i]));
      }
      buffer[i]=NULL;
    }
    count = 0;   
 // checks for messages  
    if (msg.indexOf(checkForString) >= 0)  
    {
      msg="";
      return true;
    }
    else
    {
      msg="";
      return false;
    }
  }
  // if we are here, we have a problem? to check it in the near future...
  msg="";
  return false;
}

void TurnOnLEDs(boolean turnOn)
{
  if (turnOn)
  {
    digitalWrite(REDLed, HIGH);
    digitalWrite(GREENLed, HIGH);
    digitalWrite(YELLOWLed, HIGH);    
  }
  else
  {
    digitalWrite(REDLed, LOW);
    digitalWrite(GREENLed, LOW);
    digitalWrite(YELLOWLed, LOW);    
  }
  
}

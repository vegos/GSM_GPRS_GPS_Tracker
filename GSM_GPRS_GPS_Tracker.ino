// #define SHOWMEMORY       // Uncomment for showing free memory

#ifdef SHOWMEMORY
  #include <MemoryFree.h>  
#endif

#include <SoftwareSerial.h>
#include <TinyGPS.h>

TinyGPS gps;
SoftwareSerial gpsserial(11, 12);
static void gpsdump(TinyGPS &gps);
static bool feedgps();
int currentSats=0,currentAlt=0,currentSpeed=0,currentCourse=0;
float currentLat=0.0,currentLon=0.0,previousLat=1.0,previousLon=1.0;
int Times;
float currentTemp=0.0,SumTemp=0.0;
char currentTime[18];
long UpdateMillis, StartMillis;

unsigned char buffer[128];         // buffer array for data recieve over serial port
int count=0;                       // counter for buffer array 

#define SoftPowerPin   2
#define ButtonPin     13
#define  REDLed        8
#define  GREENLed      9
#define  YELLOWLed    10
#define  LM35Pin      A3

#define MobilePhoneNumber "+30XXXXXXXXXX"


#define GPSOn true
#define GPSDebug false
#define Debug false

#ifdef SHOWMEMORY
  long MemoryMillis;
#endif


void setup()  
{
  pinMode(REDLed, OUTPUT);
  pinMode(GREENLed, OUTPUT);
  pinMode(YELLOWLed, OUTPUT);
  pinMode(SoftPowerPin, OUTPUT);
  pinMode(ButtonPin, INPUT);
  if (Debug)  
    Serial.begin(19200);
  digitalWrite(REDLed, HIGH);
  delay(200);
  digitalWrite(REDLed, LOW);
  digitalWrite(GREENLed, HIGH);
  delay(200);
  digitalWrite(GREENLed, LOW);
  digitalWrite(YELLOWLed, HIGH);
  delay(200);
  digitalWrite(YELLOWLed, LOW);
  delay(200);
  digitalWrite(REDLed, HIGH);
  digitalWrite(GREENLed, HIGH);
  digitalWrite(YELLOWLed, HIGH);
  delay(200);
  digitalWrite(REDLed, LOW);
  digitalWrite(GREENLed, LOW);
  digitalWrite(YELLOWLed, LOW);
  if (Debug)  
  {
    Serial.println("+-------------------+");
    Serial.println("| GPS GPRS Tracking |");
    Serial.println("+-------------------+");
    Serial.println();
    Serial.println("Power On");
  }
  Serial1.begin(19200);
  PowerUp();

  if (GPSOn)
  {
    gpsserial.begin(9600);
    gpsserial.println("$PMTK301,2*2E");              // DGPS = WAAS
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

void loop() // run over and over
{
#ifdef SHOWMEMORY
    if ((millis()-MemoryMillis)>1000)
    {
      if (Debug)
      {
        Serial.print("Free memory: ");
        Serial.print(freeMemory());
        Serial.println(" bytes");
      }
      MemoryMillis=millis();
    }
#endif
  if ((millis()-UpdateMillis>60000) && (currentSats>=3) && (currentLat != 0.0) && (currentLon != 0.0))     // Update every 1 minute via GPRS (when valid gps lon/lat)
  {
    UpdateOverGPRS();
    UpdateMillis=millis();
  }
  
  
  if (digitalRead(ButtonPin)==HIGH)      // on button press, send sms
  {
    Serial1.println("AT+CMGF=1");
    delay(1000);
    Serial1.println("AT+CSMP=17,167,0,241");
    delay(1000);
    Serial1.print("AT+CMGS=\"");
    Serial1.print(MobilePhoneNumber);
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
    Serial1.write(0x1A);  //Equivalent to sending Ctrl+Z 
    if (Debug)  
      ShowSerialData();
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
  
 if (Debug)  
 {
   ShowSerialData();
   if (Serial.available())            // if data is available on hardwareserial port ==> data is comming from PC or notebook
     Serial1.write(Serial.read());       // write it to the Serial1 shield
 }


  GetTemperature();
  // clean temperature readings   
  if (Times>1000)
  {
    Times=0;
    SumTemp=0.0;
  }

}



void clearBufferArray()              // function to clear buffer array
{
  for (int i=0; i<count; i++)
  { 
    buffer[i]=NULL;
  }
}

void PowerUp()                      // Software power-up GSM shield
{
 digitalWrite(SoftPowerPin,LOW);
 delay(100);
 digitalWrite(SoftPowerPin,HIGH);
 delay(2000);
 digitalWrite(SoftPowerPin,LOW);
}




/*
void AnswerCall()
{
  Serial1.println("ATA");
}

void HangUpCall()
{
  Serial1.println("+++");
  delay(50);
  Serial1.println("ATH");
}
*/






static void gpsdump(TinyGPS &gps)
{
  // Get GPS Satellites. If less than 4, means no fix, so return
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
    return;
  }
  
  float flat, flon;
  unsigned long age;
  int year;
  byte month, day, hour, minute, second, hundredths;

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    
  // DST Correction ---
  // last sunday of march
  int beginDSTDate=  (31 - (5* year /4 + 4) % 7);
  int beginDSTMonth=3;
  //last sunday of october
  int endDSTDate= (31 - (5 * year /4 + 1) % 7);
  int endDSTMonth=10;
  // DST is valid as:
  if (((month > beginDSTMonth) && (month < endDSTMonth)) || ((month == beginDSTMonth) && (day >= beginDSTDate)) || ((month == endDSTMonth) && (day <= endDSTDate)))
    hour=hour+3;  // DST europe = utc +2 hour
  else
    hour=hour+2; // nonDST europe = utc +1 hour
  
  sprintf(currentTime,"%02d/%02d/%02d %02d:%02d:%02d", day, month, year, hour, minute, second);

  currentSats=Sats;
  gps.f_get_position(&flat, &flon, &age);
  currentLat=flat;
  currentLon=flon;
  currentAlt=gps.f_altitude();
  currentSpeed=gps.f_speed_kmph();
  currentCourse=gps.f_course();
  
  if (GPSDebug)
  {
    Serial.print("[GPS] Lat: ");
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
    Serial.print(currentSats);
    Serial.print(" - Temp: ");
    Serial.println(currentTemp,1);
  }
}


static bool feedgps()
{
  while (gpsserial.available())
  {
    digitalWrite(REDLed, HIGH);              // Turn on GPSLed to indicate receiving of data
    if (gps.encode(gpsserial.read()))
    {
      digitalWrite(REDLed, LOW);            // Turn off GPSLed when are OK
      return true;
    }
  }
  digitalWrite(REDLed, LOW);                // Turn off GPSLed when not receiving data
  return false;
}


void GetTemperature()
{
  Times+=1;
  SumTemp+=(analogRead(LM35Pin) * 5.0 * 100.0) / 1024.0;
  currentTemp=SumTemp/Times;  
}


void UpdateOverGPRS()
{
  if ((previousLat==currentLat) && (previousLon==currentLon))
    return;
  digitalWrite(REDLed, HIGH);
  digitalWrite(GREENLed, HIGH);
  digitalWrite(YELLOWLed, HIGH); 
  if (Debug)  
    Serial.println("--- STARTING GPRS UPDATE"); 
  Serial1.println("AT+CSTT=\"APN_NAME\",\"USERNAME\",\"PASSWORD\"");//setting the APN
  Serial1.write(0x1A);
  delay(2000); 
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSRIP=1");
  if (Debug)  
    ShowSerialData();
  delay(2000);
  Serial1.println("AT+CIICR");  
  delay(2000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIFSR");
  Serial1.write(0x1A);
  delay(2000); 
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CDNSCFG?");
  delay(2000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPHEAD=1");
  delay(2000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSTATUS");
  Serial1.write(0x1A);
  delay(2000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSTART=\"TCP\",\"YOUR_SERVER_IP_GOES_HERE\",\"80\"");
  Serial1.write(0x1A);
  delay(3000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSTATUS");
  Serial1.write(0x1A);
  delay(2000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSEND");
  delay(5000);
  if (Debug)  
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
  Serial1.print(currentTemp);
  Serial1.print(" HTTP/1.1");
  Serial1.println("");
  Serial1.println("");
  Serial1.write(0x1A);
  delay(5000);
  if (Debug)  
    ShowSerialData();
  Serial1.println("AT+CIPSHUT");
  delay(1000);
  if (Debug)  
    ShowSerialData();
  previousLat=currentLat;
  previousLon=currentLon;
  digitalWrite(REDLed, LOW);
  digitalWrite(GREENLed, LOW);
  digitalWrite(YELLOWLed, LOW);  
}

void ShowSerialData()
{
 if (Serial1.available())              // if date is comming from GSM Shield port ==> data show on Serial port
 {
   while(Serial1.available())          // reading data into char array 
   {
     buffer[count++]=Serial1.read();     // writing data into array
     if (count == 128)
       break;
   }
   Serial.write(buffer,count);            // if data transmission ends, write buffer to serial port
   clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
   count = 0;                       // set counter of while loop to zero
 }
} 

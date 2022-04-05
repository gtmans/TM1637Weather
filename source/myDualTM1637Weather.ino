/*
https://github.com/gtmans/TM1637Weather
based on LilyGO TTGO T7 Mini32 ESP32-WROVER 
pinout https://github.com/LilyGO/TTGO-T7-Demo
but you can use any esp board with WIFI
also used a TM1637Display with decimal point or semicolums
case: https://www.tinkercad.com/things/hqS0UfUXTsO-wrover32-tm1637-casev1
In Arduino IDE: Boards ESP32 Arduino /ESP32 Wrover Module

this version has 2 TM1637 displays

Displays alternating:
  DISPLAY 1                                 DISPLAY 2
- internet time                           - CO2 readings in PPM using a MQ135
- date                                    - ppnn (meaning PPM)
- outside temperature                     - inside temperature using BME280
- windspeed and direction uu means west
- outside humidity                        - inside temperature using BME280
- internet time
- outside minimum temperature             - inside pressure
- outside maximum temperature             - nnb (meaning Millibar)

and 2 leds:
red   led meaning outside it is freezing
green led meaning outside it is >= 1 degree celcius (meaning I can turn on airco for heating)

*/
bool debug   = false;
bool precise = false;
bool setmeup = false;
bool neg;
#define RLED 5
#define GLED 23

// TM1637
#include <TM1637Display.h>
// Define the connections pins:
#define CLK  27 // display1
#define DIO  25
#define CLK2 18 // display2
#define DIO2 26
// Create display objects of type TM1637Display:
TM1637Display display  = TM1637Display(CLK, DIO);
TM1637Display display2 = TM1637Display(CLK2, DIO2);


// MQ135
// https://iotbyhvm.ooo/interface-mq135-gas-sensor-with-nodemcu/
#define         Pin135                    (35)    //Analog input 0 of your arduino
#include        "MQ135.h"
MQ135 mq135_sensor = MQ135(Pin135);

// BME280 left2right
//vcc
//gnd
//SCL 22 white
//SDA 21 yellow
//cs8 not used
//sd0 not used
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

float    BMEtemp=20;
float    BMEpres;
float    BMEhumi=25;
String   SBMEtemp;
String   SBMEpres;
String   SBMEhumi;

#define S1 6
#define S2 2
#define S3 3
#define S4 3
#define S5 2
#define S6 6
#define S7 2
#define S8 2
/*
NEWdisplay 1 display2 TIME 
S1 TIME      PPM      6    
S2 DATE      C02      2   
S3 TEMPO     TEMPI    3  
S4 WIND               3  
S5 HUMX      HUMI     2
S6 TIME               6
S7 CMIN      PRES     2
S8 CMAX      hpa      2
*/

// display special chars
/*https://www.makerguides.com/tm1637-arduino-tutorial/
 *   A
 * F   B
 *   G
 * E   C
 *   D   DP
 */
// Create array that turns all segments on:
const uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
// Create array that turns all segments off:
const uint8_t blank[] = {0x00, 0x00, 0x00, 0x00};
// Create north/east/south/west:
const uint8_t north[] = {
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F };        // N
const uint8_t east[] = {
//  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G         // E east
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F}; // O oost
const uint8_t south[] = {
//  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G         // S
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G };        // Z
const uint8_t west[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F };        // U or UU = west 
// done
const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G };         // E
const uint8_t hpa[] = {
  SEG_C | SEG_E | SEG_F | SEG_G,                   // h
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // p
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G }; // a
const uint8_t co2[] = {
  SEG_A | SEG_D | SEG_E | SEG_F,                   // c
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // o
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G };         // 2
const uint8_t pp[] = {
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // P
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,           // P
  SEG_C | SEG_E | SEG_G ,                          // n
  SEG_C | SEG_E | SEG_G };                         // n
const uint8_t mb[] = {
  SEG_C | SEG_E | SEG_G ,                          // n
  SEG_C | SEG_E | SEG_G ,                          // n
  SEG_C | SEG_D | SEG_E | SEG_F | SEG_G };         // b
  // Create degree Celsius symbol:
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Circle
  SEG_A | SEG_D | SEG_E | SEG_F };   // C
const uint8_t C[] = {
  SEG_A | SEG_D | SEG_E | SEG_F };   // C
// Create % symbol:
const uint8_t percent[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Circle high
  SEG_C | SEG_D | SEG_E | SEG_G };// Circle low
// Create circle high symbol:
const uint8_t circleh[] = {SEG_A | SEG_B | SEG_F | SEG_G };   // Circle high
// Create circle low symbol:
const uint8_t circlel[] = {SEG_C | SEG_D | SEG_E | SEG_G };   // Circle low

#include <WiFi.h>
//from ESP32_Client_TTGO_8.4
#define rightbutton 39
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include "secrets.h"
//RTC
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//NEW from myDST-final
String  yearStamp;
int     Tyear;
int     daynr;
int     mon;
int     DOW;            // Day Of Week 0=sunday 6=saturday
int     offset=6;       // 202200101 -> 6
int     checkyear;
int     StartDST;
int     EndDST;
bool    LEAP=false;
bool    DEBUG=false;
String weekdays[7]  = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
String   months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};    
int   monthdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
int     chkhour=0;
int     today=0;

//HTTP
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
String payload= "";              //whole json 
String tmp    = "";              //temperatur
String hum    = "";              //humidity
String tmpmi  = "";              //temp max
String tmpma  = "";              //temp min
String tmps   = "";              //windspeed
String tmpr   = "";              //wind direction
String tmpf   = "";              //wind direction
String tmpp   = "";              //wind direction
String tmpd   = "";
String tmpm   = "";              //main description
String tmpv   = "";              //visibility
StaticJsonDocument<1000> doc;
WiFiUDP ntpUDP;                  // Define NTP Client to get time
NTPClient timeClient(ntpUDP);    // Variables to save date and time
String formattedDate;
String dayStamp;
String hourStamp;
String minsStamp;
String DDStamp;
String MMStamp;
String LastdayStamp;
String TimeStamp;
String LastTimeStamp;
String wholetemp;
String decstemp;

String monthStamp;
String daysStamp;
int    Tdays;
bool   DST;

int    whole; 
int    decs;
int    pos;
int    len;
int    Thour;
int    Tmins;  
int    Tday;
int    Tmonth;  
int    angle;   
int    timer=0;
int    timemax=30;               //30=every 15 minutes
String Ftemp;                    //formatted temp sensor1
String Fhum;                     //formatted hum  sensor1
String Ftemp2;                   //formatted temp sensor2
String Fhum2;                    //formatted hum  sensor2
String Ftempi;                   //formatted internet temp
String Fhumi;                    //formatted internet hum
String Fmin;
String Fmax;
String Fmindec;
String Fmaxdec;
float  windspeed;
int    beauf;
int    winddeg;

void setup(void)
{
  Serial.begin            (115200);
  while(!Serial);         // time to get serial running
  Serial.println          (__FILE__);//name of this doc
  display.clear();
  display2.clear();
  delay(1000);

  Serial.print  ("SDA-Y");
  Serial.println(SDA);
  Serial.print  ("SCL-W");
  Serial.println(SCL);

  pinMode(RLED, OUTPUT);
  pinMode(GLED, OUTPUT);
  
  Serial.println    ("blinking now");
  digitalWrite(RLED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(RLED, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(GLED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(GLED, LOW);    // turn the LED off by making the voltage LOW
 
  Serial.println  (F("BME280 test"));
  unsigned status;
  status = bme.begin(0x76);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print  ("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print  ("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print  ("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print  ("        ID of 0x60 represents a BME 280.\n");
      Serial.print  ("        ID of 0x61 represents a BME 680.\n");
      // while (1) delay(10);
  }
  Serial.println("-- Default Test --");
  Serial.println();
  readBMEValues ();       // BMEtemp BMEhumi BMEpres
  printBMEValues(); 
  
  display.setBrightness   (0); // red display
  display2.setBrightness  (2); // green display
  WiFi.begin              (ssid, password);
  Serial.println          ("Connecting");
  while (WiFi.status() != WL_CONNECTED) {delay(500);}  
  Serial.println          ("Connected to ");
  Serial.println          (ssid);
  Serial.println          ("with IP Address: ");
  Serial.println          (WiFi.localIP());
  display.setSegments     (done);
  display2.setSegments    (done);
  delay(500);
  display.clear();

  float rzero = mq135_sensor.getRZero();
  float correctedRZero = mq135_sensor.getCorrectedRZero(BMEtemp, BMEhumi);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(BMEtemp, BMEhumi);
  Serial.print("MQ135 RZero: ");
  Serial.print(rzero);
  Serial.print("\t Corrected RZero: ");
  Serial.print(correctedRZero);
  Serial.print("\t Resistance: ");
  Serial.print(resistance);
  Serial.print("\t PPM: ");
  Serial.print(ppm);
  Serial.print("\t Corrected PPM: ");
  Serial.print(correctedPPM);
  Serial.println("ppm");

//RTC Initialize a NTPClient to get time
  timeClient.begin(); 
  timeClient.setTimeOffset(3600);  
  while(!timeClient.update()) {timeClient.forceUpdate();}
  // 2018-05-28T16:00:13Z
  formattedDate   = timeClient.getFormattedDate();
/*
  pos             = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, pos);
  monthStamp      = dayStamp.substring(5,7);
  daysStamp       = dayStamp.substring(8,10);
  Tmonth          = monthStamp.toInt();
  Tdays           = daysStamp.toInt();
  int today       = (Tmonth-1)*30 + Tdays;
  if  (today < ((2*30)+26) || today > ((9*30)+26)){DST=false;} else {DST=true;} // rough auto DST
*/

  pos             = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, pos);  //2022-05-28
  yearStamp       = dayStamp.substring(0,4);          //2022
  monthStamp      = dayStamp.substring(5,7);          //01
  daysStamp       = dayStamp.substring(8,10);         //01
  Tyear           = yearStamp.toInt();
  Tmonth          = monthStamp.toInt();
  Tday            = daysStamp.toInt();

  getDST(2022);
  if  (DST){
    timeClient.setTimeOffset(7200);  
    while(!timeClient.update()) {timeClient.forceUpdate();}
  }
   Serial.print            ("dayStamp:>");
   Serial.print            (dayStamp);
   Serial.print            ("< Dagnr:");
   Serial.print            (today);
   Serial.print            (" DST:");
   Serial.println          (DST);

  
  TimeStamp       = formattedDate.substring(pos+1, formattedDate.length()-1);
  pos             = TimeStamp.indexOf(":");
  hourStamp       = TimeStamp.substring(0, pos);
  minsStamp       = TimeStamp.substring(pos+1, TimeStamp.length()-1);
  Thour           = hourStamp.toInt();
  Tmins           = minsStamp.toInt();
  display.clear   ();
  getWeatherData  (); 
  windangle       (angle);
  Beaufort        (windspeed);
} // end of setup

void loop()
{
//GETSENSORS
//readBMEValues(); // BMEtemp BMEhumi BMEpres valeus are more reliable when not read at same time
//https://github.com/Phoenix1747/MQ135/blob/master/examples/MQ135/MQ135.ino
//  float rzero = mq135_sensor.getRZero();
  float rzero = 0.76;
  float correctedRZero = mq135_sensor.getCorrectedRZero(BMEtemp, BMEhumi);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(BMEtemp, BMEhumi);
  Serial.print("MQ135 RZero: ");
  if (debug){
    Serial.print(rzero);
    Serial.print("\t Corrected RZero: ");
    Serial.print(correctedRZero);
    Serial.print("\t Resistance: ");
    Serial.print(resistance);
    Serial.print("\t PPM: ");
    Serial.print(ppm);
    Serial.print("\t Corrected PPM: ");
    Serial.print(correctedPPM);
    Serial.println("ppm");
  } else {
    Serial.println(rzero);

  }
  float PPM   = correctedPPM;
//int  PPM = analogRead(Pin135);               // alternate way to read values
//map(value, fromLow, fromHigh, toLow, toHigh) // do we need to remap val 0-4096

//GETTIME
while(!timeClient.update()) {timeClient.forceUpdate();}
  // 2022-01-30T11:30:30Z
  formattedDate   = timeClient.getFormattedDate();
  pos             = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, pos);
  TimeStamp       = formattedDate.substring(pos+1, formattedDate.length()-1);
  pos             = TimeStamp.indexOf(":");
  hourStamp       = TimeStamp.substring(0, pos);
  minsStamp       = TimeStamp.substring(pos+1, TimeStamp.length()-1);
  DDStamp         = dayStamp.substring(8,10);
  MMStamp         = dayStamp.substring(5,7);
  Thour           = hourStamp.toInt();
  Tmins           = minsStamp.toInt();
  Tday            = DDStamp.toInt();
  Tmonth          = MMStamp.toInt();
/*
S1 TIME      PPM      6    
S2 DATE      C02      2   
S3 TEMPO     TEMPI    3  
S4 WIND               3  
S5 HUMX      HUMI     2
S6 TIME               6
S7 CMIN      PRES     2
S8 CMAX      hpa      2
*/
//S1 TIME      PPM      6   
  display.clear         ();
  display2.clear        (); 
  display2.showNumberDec(PPM,false,4,0);
  disptime              (S1);                 //time 2 display time

//S2 DATE      C02      2   
  display.clear         ();
  display2.clear        ();
//display2.setSegments  (co2,3,1);
  display2.setSegments  (pp,4,0);
  display.showNumberDec(Tday,   true,2,0);    //05    DD
  display.showNumberDec(Tmonth, true,2,2);    //12    MM
  delay                 (S2*1000);

//S3 TEMPO     TEMPI    3  
  // calculations 
  BMEtemp   =    bme.readTemperature();
  BMEtemp   =    BMEtemp-2.1;                 // set correction
  Serial.print  ("BMEtemp:");
  Serial.println(BMEtemp);
  SBMEtemp  =    String(BMEtemp);
  // display
  display.clear ();
  display2.clear();
  disptemp      (Ftempi,1);
  disptemp      (SBMEtemp,2);
//display2.setSegments (C,1,3); 
  display2.setSegments (circleh,1,3); 
  delay         (S3*1000);  

//S4 WIND               3  
  display.clear ();                           //show wind
  windangle     (angle);
  Beaufort      (windspeed);
  delay         (S4*1000); 
  
//S5 HUMX      HUMI     2
  // calculations 
  int Chum     = Fhumi.toInt();
  len          = Fhumi.length();
  BMEhumi      = bme.readHumidity();
  SBMEhumi     = String(BMEhumi);
  int Chum2    = SBMEhumi.toInt();
  Chum2       += 11;//correction
  Serial.print  ("BMEhumi:");
  Serial.println(BMEhumi);
  if (Chum ==100){Chum =99;}                //cannot display 100%
  if (Chum2>99  ){Chum2=99;}                //cannot display 100%
  // display
  display.clear ();
  display2.clear();
  display.setSegments     (percent,2,2);
  display.showNumberDec   (Chum,false,2,0); //humidity
  display2.setSegments    (percent,2,2);
  display2.showNumberDec  (Chum2,false,2,0); //humidity
  delay                   (S5*1000); 

//S6 TIME               6
  display.clear ();       //show time
  disptime(S6);           //seconds display time   

//S7 CMIN      PRES     2
  BMEpres     = bme.readPressure()/100;
  SBMEpres    = String(BMEpres);
  int Pres    = SBMEpres.toInt();
  Serial.print  ("BMEpres:");
  Serial.println(BMEpres);
  
  display2.clear (); 
  display2.showNumberDec  (Pres,false,4,0);  
  display.clear       (); 
  display.setSegments (circlel,1,0);
  display.setSegments (celsius,1,3);
  int Cmin     =      Fmin.toInt();
  int Cmindec  =      Fmindec.toInt();
  if (Cmindec >= 5)   {                  //lets roundup
   if (Cmin<0){Cmin--;}else{Cmin++;}}    // round 10,5 to 11 or -10,5 to -11
  display.showNumberDec  (Cmin, false , 2, 1); //mintemp
  delay         (S7*1000); 
  
//S8 CMAX      hpa      2
  int Cmax     =        Fmax.toInt();
  int Cmaxdec  =        Fmaxdec.toInt();
  if (Cmaxdec >= 5)     { //lets roundup
  if (Cmax<0){Cmax--;}else{Cmax++;}}    // round 10,5 to 11 or -10,5 to -11

  display.clear         (); 
  display2.clear        (); 
  display.setSegments   (circleh,1,0);
  display.setSegments   (celsius,1,3);
  display.showNumberDec (Cmax, false , 2, 1); //mintemp
//  display2.setSegments  (hpa,3,1);
//  srzero = String(rzero,2);
//  Serial.print  ("rzero:");
//  Serial.println(rzero);
//  int irzero = rzero*100;
  display2.setSegments  (mb,3,1); //millibar
//  display2.showNumberDecEx  (rzero*100, 0b01000000, false , 4, 0); 
  delay                 (S8*1000); 

  timer++; // for internet updates
  if (timer>timemax){
    display.clear ();
    for (int x=0;x<4;x++) {display.setSegments(circleh,1,x);delay(500);}        //animation
    getWeatherData(); 
    timer=0;
    
    chkhour++;
    if (chkhour>3)
    {
      chkhour=0;
      getDST(Tyear);
      if  (DST){ timeClient.setTimeOffset(7200); } else {timeClient.setTimeOffset(3600);} 
      while(!timeClient.update()) {timeClient.forceUpdate();}
    }    
  }
}
    
void  disptime(int times){
 for (int t=0;t<times;t++){
   display.showNumberDec  ((Thour*100)+Tmins,              true , 4, 0); //05 23 (time)
   delay   (500);
   display.showNumberDecEx((Thour*100)+Tmins, 0b01000000 , true , 4, 0); //05.23 (time)
   delay   (500);
 }
}

//        * 0.000 (0b10000000)
//        * 00.00 (0b01000000)
//        * 000.0 (0b00100000)  
void  disptemp(String mytemp,int dispnum){ //10.5
  pos        = mytemp.indexOf   (".");
  wholetemp  = mytemp.substring (0, pos);
  decstemp   = mytemp.substring (pos+1);
  whole      = wholetemp.toInt  ();
  decs       = decstemp.toInt   ();
  if (dispnum==1){     
    if (whole<0){neg=true;}else{neg=false;} 
    if   (precise==false){
      if (decs>=5)
      {
        if (neg==true){whole--;}else{whole++;}    // round 10,5 to 11 or -10,5 to -11
      }
      
      if (whole<=-10){
        display.showNumberDec   (whole, false, 3, 0);     //-11°
        display.setSegments     (celsius,      2, 3);   
      } else { //positive
        display.showNumberDec   (whole, false, 2, 0);     //11°C or " 1°C" or "-1°C" 
        display.setSegments     (celsius,      2, 2);//
      }   
    } // not precise   
    else {display.showNumberDecEx((whole*100)+decs, 0b01000000 , true , 4, pos);}   //11.11
    } else {
      display2.showNumberDecEx((whole*100)+decs, 0b01000000 , true , 4, 0);}   //11.11   
}//end void
  
void getWeatherData(){  
   if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    Serial.println("Gathering new weatherdata");
    HTTPClient http; 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request 
    if (httpCode > 0) { //Check for the returning code 
         payload = http.getString();
         if (debug==true){
            Serial.println(payload);}   
         }     
    http.end(); //Free the resources
   
    /*//{"coord":{"lon":5.9694,"lat":52.21},
    "weather":[{"id":803,"main":"Clouds","description":"broken clouds","icon":"04d"}],
    "base":"stations",
    "main":{"temp":2.7,"feels_like":-1.04,"temp_min":1.57,"temp_max":4.36,"pressure":1001,"humidity":96},
    "visibility":10000,
    "wind":{"speed":4.12,"deg":270},
    "clouds":{"all":75},
    "dt":1638438824,
    "sys":{"type":2,"id":2010138,"country":"NL","sunrise":1638429867,"sunset":1638458825},"timezone":3600,"id":2759706,"name":"Apeldoorn","cod":200}
    */
    
    char inp[1000];
    payload.toCharArray(inp,1000);
    deserializeJson(doc,inp);
    String tmp_main = doc["main"];//not working
    String tmp_desc = doc["description"];//not working
    String tmp_icon = doc["weather"]["icon"];
    String tmp_temp = doc["main"]["temp"];
    String tmp_min  = doc["main"]["temp_min"];
    String tmp_max  = doc["main"]["temp_max"];
    String tmp_pres = doc["main"]["pressure"];
    String tmp_humi = doc["main"]["humidity"];       
    String tmp_visi = doc["visibility"];
    String tmp_spee = doc["wind"]["speed"];
    String tmp_degr = doc["wind"]["deg"];

    tmp   = tmp_temp;
    hum   = tmp_humi;
    tmpmi = tmp_min;
    tmpma = tmp_max;
    tmps  = tmp_spee;
    tmpr  = tmp_degr;
    tmpp  = tmp_pres;  
    tmpd  = tmp_desc;
    tmpm  = tmp_main;
    tmpv  = tmp_visi;
     
    angle           = tmpr.toInt();
    windspeed       = tmps.toFloat();
    Beaufort          (windspeed);      // returs int beauf
    float windtemp  = tmpr.toFloat();
    int   windtemp2 = windtemp/10;
    pos             = tmp.indexOf    (".");
    Ftempi          = tmp.substring  (0,pos+2);
    pos             = hum.indexOf    (".");
    Fhumi           = hum.substring  (0,pos+3);
    pos             = tmpmi.indexOf  (".");
    Fmin            = tmpmi.substring(0,pos+2);
    pos             = tmpma.indexOf  (".");
    Fmax            = tmpma.substring(0,pos+2);
    Fmaxdec         = tmpma.substring(pos+1,pos+2);
    Fmindec         = tmpmi.substring(pos+1,pos+2);

    pos        = Ftempi.indexOf   (".");
    wholetemp  = Ftempi.substring (0, pos);
    decstemp   = Ftempi.substring (pos+1);
    whole      = wholetemp.toInt  ();
    decs       = decstemp.toInt   (); 
    if (whole<0){
      neg=true;
      digitalWrite(RLED, HIGH);
      digitalWrite(GLED, LOW);
      }
    else{
      digitalWrite(RLED, LOW);
      neg=false;
//      if ((whole*10)+decs)
        if (whole>=1){
          digitalWrite(GLED, HIGH);
        }
    } 
    
 } else {Serial.println("WiFi Disconnected");}  
}

void windangle(int degree){
  if (degree>337.5 || degree <= 22.5){display.setSegments(north,1,0);pos=1;}                              //N
  if (degree> 22.5 && degree <= 67.5){display.setSegments(north,1,0);pos=2;display.setSegments(east,1,1);}//NE
  if (degree> 67.5 && degree <=112.5){display.setSegments(east, 1,0);pos=1;}                              //E
  if (degree>112.5 && degree <=157.5){display.setSegments(south,1,0);pos=2;display.setSegments(east,1,1);}//SE
  if (degree>157.5 && degree <=202.5){display.setSegments(south,1,0);pos=1;}                              //S
  if (degree>202.5 && degree <=247.5){display.setSegments(south,1,0);pos=3;display.setSegments(west,1,1);display.setSegments(west,1,2);}//SW= SUU
  if (degree>247.5 && degree <=292.5){display.setSegments(west, 1,0);pos=2;display.setSegments(west,1,1);}                              //W = UU
  if (degree>292.5 && degree <=337.5){display.setSegments(north,1,0);pos=3;display.setSegments(west,1,1);display.setSegments(west,1,2);}//NW= NUU
}

void Beaufort(float wspeed){
  if (wspeed<  0.3)               {beauf= 0;}
  if (wspeed>= 0.3 && wspeed< 1.6){beauf= 1;}
  if (wspeed>= 1.6 && wspeed< 3.4){beauf= 2;}
  if (wspeed>= 3.4 && wspeed< 5.5){beauf= 3;}
  if (wspeed>= 5.5 && wspeed< 8.0){beauf= 4;}
  if (wspeed>= 8.0 && wspeed<10.8){beauf= 5;}
  if (wspeed>=10.8 && wspeed<13.9){beauf= 6;}
  if (wspeed>=13.9 && wspeed<17.2){beauf= 7;}
  if (wspeed>=17.2 && wspeed<20.8){beauf= 8;}
  if (wspeed>=20.8 && wspeed<24.5){beauf= 9;}
  if (wspeed>=24.5 && wspeed<28.5){beauf=10;}
  if (wspeed>=28.5 && wspeed<32.7){beauf=11;}
  if (wspeed> 32.7)               {beauf=12;}
  if (beauf<10){display.showNumberDec(beauf,false,1,pos);}else {display.showNumberDec(beauf,false,2,pos);}
}

void readBMEValues() {
    BMEtemp=bme.readTemperature();
    BMEpres=bme.readPressure()/100;
    BMEhumi=bme.readHumidity();
    SBMEtemp=String(BMEtemp);
    SBMEpres=String(BMEpres);
    SBMEhumi=String(BMEhumi);
}

void printBMEValues() {
    Serial.print    ("Temperature = ");
    Serial.print    (BMEtemp);
    Serial.println  (" *C");
    Serial.print    ("Pressure = ");
    Serial.print    (BMEpres);
    Serial.println  (" hPa");
    Serial.print    ("Approx. Altitude = ");
    Serial.print    (bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println  (" m");
    Serial.print    ("Humidity = ");
    Serial.print    (BMEhumi);
    Serial.println  (" %");
    Serial.println  ();
}

// get the day of the week for 1st day of year
void getDOW(int d){
  int h = (d-1+offset)/7;
  DOW   =  d-1+offset - h*7;
 if (DEBUG){Serial.print(weekdays[DOW]);}
  mon = 0;
  while (d>monthdays[mon]){
    d=d-monthdays[mon];
    mon++;
  }
  daynr=d;
  if (DEBUG){
    Serial.print      (" ");
    Serial.print      (daynr);
    Serial.print      (" ");
    Serial.println    (months[mon]);    
  }
}

// check if year is a leap and set bool LEAP
void getLEAP(int y){ // leapyear div 4 or 400 but not 100 has 366 days io 365
  bool test1 = ((y-((y/400)*400))==0);  
  bool test2 = ((y-((y/100)*100))==0);  
  bool test3 = ((y-((y/4)*4))==0);  
  if (test1)    {LEAP=true;} 
  else 
    {if (test2) {LEAP=false;}
    else 
      {if (test3){LEAP=true;} 
      else {LEAP=false;} 
      }
    }
  if (LEAP){
    if (DEBUG){
      Serial.print      (y);
      Serial.print      (":");
      Serial.println    ("leapyear!");      
    }
    monthdays[1]=29;
  } else {
    monthdays[1]=28;}
}

// 01012022 sa 6 6 + 365 = 371 / 7 = 53 + 0
// 01012023 su 0 0 + 365 = 365 / 7 = 52 + 1
// 01012024 mo 1 1 + 366 = 367 / 7 = 52 + 3 leap but counts next year
// 01012025 we 3 3 + 365 = 368 / 7 = 52 + 4
void getDOWyear(int y){ // changes LEAP
  offset=6; // 202200101 = saturday = 6
  if (y==2022){DOW=offset;}
  else {
    for (checkyear = 2023; checkyear<=y;checkyear++){
      getLEAP(checkyear-1);
      if (LEAP){offset=offset+366;} else {offset=offset+365;}
      DOW   =  offset - 7*(offset/7);
    }  
  }  
  if (DEBUG){
    Serial.print      ("01-01-");
    Serial.print      (y);
    Serial.print      (" is een ");
    Serial.println    (weekdays[DOW]);
  }
  offset=DOW;// set first day of week for this year
}

void getDST(int t){
  Tyear=t;
  Serial.print      ("Finding  summertime period for ");
  Serial.println    (Tyear);
  getDOWyear        (Tyear);    // sets value of "offset" = day of week of 1/1/Tyear
  getLEAP           (Tyear);    // check if this year is a leap year sets bool LEAP


  StartDST=90+LEAP; // if bool LEAP == true then StartDST=91
  for (int test=90+LEAP; test>83+LEAP; test--) { // find last sunday in march
    getDOW(test);
    if (DOW==0){break;}
    StartDST--;
  }

//  if (DEBUG){
    Serial.print      ("Start    summertime is ");
    Serial.print      (weekdays[DOW]);
    Serial.print      (" ");
    Serial.print      (daynr);
    Serial.print      (" ");
    Serial.print      (months[mon]);
    Serial.print      (" day ");
    Serial.println    (StartDST);  
//  }

    EndDST=304+LEAP;
    for (int test=304+LEAP; test>287+LEAP; test--) { // find last sunday in october
    getDOW(test);
    if (DOW==0){break;}
    EndDST--;
}

//  if (DEBUG){
    Serial.print      ("End of   summertime is ");
    Serial.print      (weekdays[DOW]);
    Serial.print      (" ");
    Serial.print      (daynr);
    Serial.print      (" ");
    Serial.print      (months[mon]);
    Serial.print      (" day ");
    Serial.println    (EndDST);
//  }

/*}

void getDaynr(){
  getLEAP(Tyear);*/
  today=0;
  for (mon=1; mon<Tmonth; mon++) { // get current day of year
    today+=monthdays[mon-1];
  }
  today=today+Tday;
  Serial.print      ("Today is day ");
  Serial.print      (today);
  Serial.print      (" of ");
  Serial.print      (Tyear);
  Serial.print      (" so it's ");
  if (today>=StartDST&&today<EndDST){DST=true;} else {DST=false;} 
  if (DST){Serial.println(" summertime!");} else {Serial.println("wintertime...");} 

}

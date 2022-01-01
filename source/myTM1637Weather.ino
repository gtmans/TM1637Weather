/*
https://github.com/gtmans/TM1637Weather
In Arduino IDE: Boards ESP32 Arduino /ESP32 Wrover Module
based on LilyGO TTGO T7 Mini32 ESP32-WROVER 
but you can use any esp board with WIFI
also used a TM1637Display with decimal point or semicolums
case: https://www.tinkercad.com/things/hqS0UfUXTsO-wrover32-tm1637-casev1

Displays alternating:
- internet time
- date
- outside temperature
- windspeed and direction U means west
- internet time
- outside minimum temperature
- outside maximum temperature

*/
#include <TM1637Display.h>
#define CLK 27
#define DIO 25
// Create display object of type TM1637Display:
TM1637Display display = TM1637Display(CLK, DIO);

bool debug  = true;
bool precise= false;
bool neg;

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
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F };        // U
// Create min:
  const uint8_t mini[] = {
  SEG_D | SEG_E }; 
// Create max:
const uint8_t maxi[] = {
  SEG_A | SEG_F };
// done
const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G };           // E
// Create degree Celsius symbol:
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Circle
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
//bool debug=true;
#define rightbutton 39
//#include <ESP32Ping.h>
#include <HTTPClient.h>
#include "my_secrets.h"
/*
// my_secrets.h: wifi passwords and weather.api get yours at api.openweathermap.org
const char* ssid     = "mySSID";        
const char* password = "myWIFIpassword";
String town="Apeldoorn";//weather api           
String Country="NL";               
const String key = "095e789fe1a290c29b29bbb364346bcd";//get your own
*/

//RTC
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
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
  Serial.println          (__FILE__);//name of this doc
  display.clear();
  delay(1000);
  display.setBrightness   (1);
  WiFi.begin              (ssid, password);
  Serial.println          ("Connecting");
  while (WiFi.status() != WL_CONNECTED) {delay(500);}  
  Serial.println          ("Connected to ");
  Serial.println          (ssid);
  Serial.println          ("with IP Address: ");
  Serial.println          (WiFi.localIP());
  display.setSegments     (done);
  delay(500);
  display.clear();

//RTC Initialize a NTPClient to get time
  timeClient.begin(); 
  timeClient.setTimeOffset(3600);  
  while(!timeClient.update()) {timeClient.forceUpdate();}
  // 2018-05-28T16:00:13Z
  formattedDate   = timeClient.getFormattedDate();
  pos             = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, pos);
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
}

void loop()
{
  timer++;
  while(!timeClient.update()) {timeClient.forceUpdate();}
  // 2018-05-28T16:00:13Z
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

  display.clear (); //show time
  disptime(5);//prox 5 seconds display time
  
  display.clear (); //show date
  display.showNumberDec(Tday,   true,2,0);                                //05    DD
  display.showNumberDec(Tmonth, true,2,2);                                //12    MM
  delay         (2500);
  
  display.clear (); //show temp
  disptemp      (Ftempi);
  delay         (2500);  

  display.clear (); //show wind
  windangle     (angle);
  Beaufort      (windspeed);
  delay         (2500); 

  display.clear (); //hum%
  int Chum     = Fhumi.toInt();
  len          = Fhumi.length();
  if (Chum==100){Chum=99;}//cannot display 100%
  display.setSegments    (percent,2,2);
  display.showNumberDec  (Chum,false,2,0); //humidity
  delay         (2000); 

  display.clear (); //show time
  disptime(5);//prox 5 seconds display time
   
  display.clear (); //min
  display.setSegments(mini,1,0);
  display.setSegments(celsius,1,3);
  int Cmin     = Fmin.toInt();
  int Cmindec  = Fmindec.toInt();
  if (Cmindec >= 5){ //lets roundup
  if (Cmin<0){Cmin--;}else{Cmin++;}}    // round 10,5 to 11 or -10,5 to -11
  display.showNumberDec  (Cmin, false , 2, 1); //mintemp
  delay         (3000); 

  display.clear (); //max
  display.setSegments(maxi,1,0);
  display.setSegments(celsius,1,3);
  int Cmax     = Fmax.toInt();
  int Cmaxdec  = Fmaxdec.toInt();
  if (Cmaxdec >= 5){ //lets roundup
   if (Cmax<0){Cmax--;}else{Cmax++;}}    // round 10,5 to 11 or -10,5 to -11
  display.showNumberDec  (Cmax, false , 2, 1); //mintemp
  delay         (2000); 

  if (timer>timemax){
    display.clear ();
    for (int x=0;x<4;x++) {display.setSegments(circleh,1,x);delay(500);}        //animation
    timer=0;
    getWeatherData(); 
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
//Ftempi 10.5
void  disptemp(String mytemp){
  pos        = Ftempi.indexOf   (".");
  wholetemp  = Ftempi.substring (0, pos);
  decstemp   = Ftempi.substring (pos+1);
  whole      = wholetemp.toInt  ();
  decs       = decstemp.toInt   (); 
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
   
    /*sample output:
    {"coord":{"lon":5.9694,"lat":52.21},
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
    pos             = Ftempi.indexOf   (".");
    wholetemp       = Ftempi.substring (0, pos);
    decstemp        = Ftempi.substring (pos+1);
    whole           = wholetemp.toInt  ();
    decs            = decstemp.toInt   (); 
    if (whole<0){neg=true;}else{neg=false;} 
 } else {Serial.println("WiFi Disconnected");}  
}

void windangle(int degree){
  if (degree>337.5 || degree <= 22.5){display.setSegments(north,1,0);pos=1;}                              //N
  if (degree> 22.5 && degree <= 67.5){display.setSegments(north,1,0);pos=2;display.setSegments(east,1,1);}//NE
  if (degree> 67.5 && degree <=112.5){display.setSegments(east, 1,0);pos=1;}                              //E
  if (degree>112.5 && degree <=157.5){display.setSegments(south,1,0);pos=2;display.setSegments(east,1,1);}//SE
  if (degree>157.5 && degree <=202.5){display.setSegments(south,1,0);pos=1;}                              //S
  if (degree>202.5 && degree <=247.5){display.setSegments(south,1,0);pos=2;display.setSegments(west,1,1);}//SW
  if (degree>247.5 && degree <=292.5){display.setSegments(west, 1,0);pos=1;}                              //W
  if (degree>292.5 && degree <=337.5){display.setSegments(north,1,0);pos=2;display.setSegments(west,1,1);}//NW
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

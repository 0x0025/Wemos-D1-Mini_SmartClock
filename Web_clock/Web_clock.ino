#include <NTPClient.h>
#include <ESP8266WiFi.h> // предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <WiFiUdp.h> // обрабатывает отправку и получение пакетов UDP
#include <OLED_I2C.h>

OLED  myOLED(4, 5); 

extern uint8_t TinyFont[];
extern uint8_t SmallFont[];
extern uint8_t BigNumbers[];

const char *ssid     = "SSID";
const char *password = "PASSWORD";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String arr_days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
String date_time;
// Вы можете указать пул сервера времени и смещение (в секундах, может быть изменено позже с помощью setTimeOffset ())
// Кроме того, вы можете указать интервал обновления (в миллисекундах, можно изменить с помощьфю setUpdateInterval ())

byte hh, mm, ss;
const byte nightBr = 22;

void setup(){
  if(!myOLED.begin(SSD1306_128X64))
    while(1);
    
  myOLED.setFont(SmallFont);
    
  myOLED.print("Starting OLED", LEFT, 8);
  myOLED.update();
    
  Serial.begin(115200);
  
  myOLED.print("Starting Serial", LEFT, 16);
  myOLED.update();
  
  WiFi.begin(ssid, password);

  
  myOLED.print("Connecting to Wi-FI", LEFT, 24);
  myOLED.update();
  
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 100 );
    Serial.print ( "." );
  }
  
  myOLED.print("Starting NTP Client", LEFT, 32);
  myOLED.update();
  
  timeClient.begin();
  timeClient.setTimeOffset(10800);//3600
  
  myOLED.print("Done", LEFT, 40);
  myOLED.update();
  
  delay(500);
  myOLED.clrScr();
}

void loop() {
  timeClient.update(); //Обновление времени
  
  hh = timeClient.getHours();
  mm = timeClient.getMinutes(); //Получение времени
  ss = timeClient.getSeconds();

  if (hh >= nightBr) {        //Управление яркостью
    myOLED.setBrightness(1);  //nightBr - кол-во часов,
  }else {                     //когда включается ночная яркость
    myOLED.setBrightness(255);
  }
  
  scr1();
  
  //Serial.println( timeClient.getFormattedDate() );
  //delay(200);
  
}

void scr1() {
  
  String timeStr; //Строка с временем с нулями через точку

  if (hh <= 9){ //Форматирование строки 
    timeStr = timeStr + "0" + String(hh) + ".";
  } else {
    timeStr = timeStr + String(hh) + ".";
  }
  
  if (mm <= 9){
    timeStr = timeStr + "0" + String(mm) + ".";
  } else {
    timeStr = timeStr + String(mm) + ".";
  }

  if (ss <= 9){
    timeStr = timeStr + "0" + String(ss);
  } else {
    timeStr = timeStr + String(ss);
  }
 
  //int day = timeClient.getDay();
  String ForDate = timeClient.getFormattedDate(); //Дата + время

  String justDate = ForDate.substring(0 , ForDate.indexOf( "T" ) ); //Только дата
  
  
  myOLED.clrScr();
  
  myOLED.setFont(BigNumbers);
  myOLED.print(timeStr , CENTER, 8 );  //Время

  myOLED.setFont(SmallFont);
  myOLED.print(justDate, CENTER, 40); //Дата
  myOLED.update();


}

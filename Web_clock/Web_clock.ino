///////////////Библиотеки////////////////
#include <NTPClient.h>
#include <ESP8266WiFi.h> // предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <WiFiUdp.h> // обрабатывает отправку и получение пакетов UDP
#include <OLED_I2C.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          
////////////////Настройки/////////////////

//Когда часы не могут подключиться будет создана сеть,
// спомощью которой можно настроить Wi-Fi
//SSID:Connect-WIFI password:PASSWORD

const byte nightBr = 19; //Час включения ночной подсветки

//////////////////OLED////////////////////

OLED  myOLED(4, 5); //D2,D1

extern uint8_t TinyFont[];
extern uint8_t SmallFont[]; //Шрифты 
extern uint8_t BigNumbers[];

//////////////////NTPClient///////////////

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org");

String arr_days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

///////////////Переменные/////////////////
byte hh, mm, ss;//Часы минуты секунды

unsigned long timing; // Переменная для хранения точки отсчета

String timeStr; //Строка с временем с нулями через точку

byte curScr = 1; //Текущий экран

//const char APssid = "Connect-WIFI";
//const char APpass = "PASSWORD";


void setup(){
  if(!myOLED.begin(SSD1306_128X64))
    while(1);
    
  myOLED.clrScr();
  myOLED.setFont(SmallFont);
    
  myOLED.print("Starting OLED", LEFT, 8);
  myOLED.update();
    
  Serial.begin(115200);
  
  myOLED.print("Starting Serial", LEFT, 16);
  myOLED.update();
  
  myOLED.print("Connecting to Wi-FI", LEFT, 24);
  myOLED.print("If its frozen,connect", LEFT, 32);
  myOLED.print("to Connect-WIFI", LEFT, 40);  //Информация о точке доступа
  myOLED.print("pwd: PASSWORD", LEFT, 48);
  myOLED.update();
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("Connect-WIFI", "PASSWORD");
  
  
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 100 );
    Serial.print ( "." );
  }
  
  myOLED.print("Starting NTP Client", LEFT, 56);
  myOLED.update();
  
  timeClient.begin();
  timeClient.setTimeOffset(10800);//Часовой ооффсет для МСК
                                  //Ищется: оффсет в часах * 3600
  
  myOLED.print("Done", LEFT, 64);
  myOLED.update();
  
  delay(500);
  myOLED.clrScr();
  timing = millis();
}

void loop() {
  timeClient.update(); //Обновление времени
  
  hh = timeClient.getHours();
  mm = timeClient.getMinutes(); //Получение времени
  ss = timeClient.getSeconds();
  
  timeStr = "";
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
 

  if (hh >= nightBr) {        //Управление яркостью
    myOLED.setBrightness(1);  //nightBr - кол-во часов,
  }else {                     //когда включается ночная яркость
    myOLED.setBrightness(255);
  }
  
  switch(curScr) {
    
    case 1:
      if (millis() - timing > 10000){
        timing = millis(); 
        Serial.println ("curScr 2");
        curScr = 2;
        scr2();
      }else{
        scr1();
        break;
      }
    break;

    case 2:
      if (millis() - timing > 5000){ 
        timing = millis(); 
        Serial.println ("curScr 3");
        curScr = 3;
        scr3();
      }else{
        scr2();
        break;
      }
    break;

    case 3:
      if (millis() - timing > 5000){ 
        timing = millis(); 
        Serial.println ("curScr 1");
        curScr = 1;
        scr1();
      }else{
        scr3();
        break;
      }
    break;
    
  }
  
  
  
  
  //scr1();
  
  //Serial.println( timeClient.getFormattedDate() );
  //delay(200);
  
}

void scr1() {
  
  String ForDate = timeClient.getFormattedDate(); //Дата + время

  String justDate = ForDate.substring(0 , ForDate.indexOf( "T" ) ); //Только дата
  
  
  myOLED.clrScr();
  
  myOLED.setFont(BigNumbers);
  myOLED.print(timeStr , CENTER, 8 );  //Время

  myOLED.setFont(SmallFont);
  myOLED.print(justDate, CENTER, 40); //Дата
  myOLED.update();

}


void scr2(){
  myOLED.clrScr();
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);
  myOLED.print("Scr 2", CENTER, 40); 
  myOLED.update();
}

void scr3(){
  myOLED.clrScr();
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);
  myOLED.print("Scr 3", CENTER, 40); 
  myOLED.update();
}

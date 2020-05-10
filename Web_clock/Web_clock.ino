///////////////Библиотеки////////////////
#include <NTPClient.h>                  //NTP клиент 
#include <ESP8266WiFi.h>                //Предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <WiFiUdp.h>                    //Обрабатывает отправку и получение пакетов UDP
#include <OLED_I2C.h>                   //Библиотека дисплея
#include <DNSServer.h>                  //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>           //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>                //Библиотека для удобного подключения к WiFi
#include <ESP8266HTTPClient.h>          //HTTP клиент
#include <ArduinoJson.h>                //Библиотека для работы с JSON
////////////////Настройки/////////////////

//Когда часы не могут подключиться будет создана сеть,
//спомощью которой можно настроить Wi-Fi
//SSID:Connect-WIFI password:PASSWORD

const byte nightBr = 19;                //Час включения ночной подсветки

const byte logEn = 1;                   // 1 - Лог вкл 0 - лог выкл

const String lat     = "55.75";         //Географические координаты
const String lon     = "37.62";         //Москва

                                        //API ключ для openweathermap.org
const String appid   = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; 

const int timeOffset = 10800;           //Часовой ооффсет для МСК
                                        //Ищется: оффсет в часах * 3600

//////////////////OLED////////////////////

OLED  myOLED(4, 5);                     //Пины D2,D1 у Wemos D1 mini

extern uint8_t TinyFont[];
extern uint8_t SmallFont[];             //Шрифты 
extern uint8_t BigNumbers[];

//////////////////NTPClient///////////////

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org");

String arr_days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

///////////////Переменные/////////////////

WiFiManager wifiManager;
HTTPClient http;
DynamicJsonDocument doc(1500); 

byte hh, mm, ss;                        //Часы минуты секунды

                                        //Переменные для хранения точек отсчета
unsigned long timing, LostWiFiMillis, lastUpd; 

String timeStr;                         //Строка с временем с нулями через точку

byte curScr = 1;                        //Текущий экран

bool LostWiFi = false;                  //Флаг потери WiFi

int temp, temp_min, temp_max;           //Переменные погоды: Тепература: сейчас, минимальная, максимальная
byte humidity, clouds;                  //Влажность и облака в процентах
String location, weather, description;  //Местоположение, погода, подробное описание погоды 
float wind;                             //Ветер в м/с



void setup(){
  myOLED.begin(SSD1306_128X64);         //Инициализация дисплея
  myOLED.clrScr();  
  myOLED.setFont(SmallFont);            //Установка шрифта
  myOLED.print("Starting OLED", LEFT, 8);
  myOLED.update();
  
  myOLED.print("Starting Serial", LEFT, 16);
  myOLED.update();
  Serial.begin(115200);
                                        //Информация о точке доступа
  myOLED.print("Connecting to Wi-FI", LEFT, 24);
  myOLED.print("If its frozen,connect", LEFT, 32);
  myOLED.print("to Connect-WIFI", LEFT, 40);  
  myOLED.print("pwd: PASSWORD", LEFT, 48);
  myOLED.update();
  
  wifiManager.setDebugOutput(logEn);    //Вкл выкл лога wifiManager
                                        //Подключение к сети
  wifiManager.autoConnect("Connect-WIFI", "PASSWORD");

  while (WiFi.status() != WL_CONNECTED){}
  
  myOLED.print("Starting NTP Client", LEFT, 56);
  myOLED.update();
  
  timeClient.begin();                   //Инициализация NTP клиента
  timeClient.setTimeOffset(timeOffset); //Установка оффсета времени

  weatherUpdate();                      //Обновление погоды
  lastUpd = millis();
  
  myOLED.print("Done", LEFT, 64);
  myOLED.update();
  
  delay(500);
  myOLED.clrScr();
  timing = millis();
}



void loop() {
  timeClient.update();                //Обновление времени

  if (WiFi.status() != WL_CONNECTED) {//Если нет подключения
    if (LostWiFi == 0){
      LostWiFi = 1;                   //Ставим флаг
      LostWiFiMillis = millis();
      } else if(millis() - LostWiFiMillis > 180000) {
                                      //Если прошло 3 мин
      myOLED.clrScr();
      myOLED.setFont(SmallFont);      //Выводим сообщение
      myOLED.print("Connection lost!", CENTER, 4);
      myOLED.print("Please connest ", LEFT, 16); 
      myOLED.print("to Connect-WIFI", LEFT, 24);
      myOLED.print("pwd: PASSWORD", LEFT, 32);
      myOLED.print("You can try to reset", LEFT, 40);
      myOLED.print("the clock", LEFT, 48);
      myOLED.update();
                                      //Запускаем конфиг портал
      wifiManager.startConfigPortal("Connect-WIFI", "PASSWORD");
    }
    
  } else{
    LostWiFi = 0;                     //Иначе сбрасываем флаг
  }

  if (millis() - lastUpd > 120000){   //Обновление раз в 2 минуты
    weatherUpdate();
    lastUpd = millis();
  }
  
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();       //Запись времени без нулей
  ss = timeClient.getSeconds();       //в переменные

  
  timeStr = "";                       //Форматирование строки времени 
  if (hh <= 9){                       //Добавляем нули где нужно
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
 
  if (hh >= nightBr) {                //Управление яркостью
    myOLED.setBrightness(1);          //nightBr - кол-во часов,
  } else{                             //когда включается пониженая яркость
    myOLED.setBrightness(255);
  }
                                      //Конструкция смены экранов
  switch(curScr) {                    
    case 1:                           //Если сейчас экран 1
      if (millis() - timing > 10000){ //Если прошло 10 сек
        timing = millis(); 
        if (logEn) {
          Serial.println ("curScr 2");
        }
        curScr = 2;                   //То сейчас экран 2
        scr2();                       //Вызов функции отрисовки 2 дисплея
      }else{
        scr1();                       //Иначе вызов функции отрисовки 1 дисплея
        break;
      }
    break;

    case 2:                           //Если сейчас экран 2
      if (millis() - timing > 5000){  //Если прошло 5 сек
        timing = millis(); 
        if (logEn) {
          Serial.println ("curScr 3");
        }
        curScr = 3;                   //То сейчас экран 3
        scr3();                       //Вызов функции отрисовки 3 дисплея
      }else{
        scr2();                       //Иначе вызов функции отрисовки 2 дисплея
        break;
      }
    break;

    case 3:                           //Если сейчас экран 3
      if (millis() - timing > 5000){  //Если прошло 5 сек
        timing = millis(); 
        if (logEn) {
          Serial.println ("curScr 1");
        }
        curScr = 1;                   //То сейчас экран 1
        scr1();                       //Вызов функции отрисовки 1 дисплея
      }else{
        scr3();                       //Иначе вызов функции отрисовки 3 дисплея
        break;
      }
    break;
    
  }
  
}

void weatherUpdate() {                 //Функция обновления погоды 
  if (WiFi.status() == WL_CONNECTED) { //Выполняется только если WiFi подключен
    if (logEn) {
      Serial.println("Updating weather");
    }
                                       //Формирование строки запроса
    String httpStr = String("http://api.openweathermap.org/data/2.5/weather") + String("?lat=") + String(lat) + String("&lon=") + String(lon) + String("&appid=") + String(appid) + String("&units=metric&lang=en");
    http.begin(httpStr);
    
    if (logEn) {
      Serial.println("Accessing: " + httpStr);
    }
    
    int httpCode = http.GET();         //Запрос
    String json = http.getString();    //Получение строки ответа
    if (logEn) {
      Serial.println("Http Code: " + String(httpCode) );
      Serial.println("Got JSON: " + json);
    }
    http.end();
                                       //Парсинг JSON
    DeserializationError error = deserializeJson(doc, json);

    if (error) {                       //Если не парсится
      if (logEn) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
      }
      return;
    }

    temp = doc["main"]["temp"];        //Запись значений в переменные
    temp_min = doc["main"]["temp_min"];
    temp_max = doc["main"]["temp_max"];
    wind = doc["wind"]["speed"];
    description = doc["weather"][0]["description"].as<String>();
    weather = doc["weather"][0]["main"].as<String>();
    humidity = doc["main"]["humidity"];
    clouds = doc["clouds"]["all"];
    location = doc["name"].as<String>();
    
    if (logEn) {
      Serial.println("Temp: " + String(temp));
    }
  }
}

void scr1() {
                                        //Форматирование строк
  String ForDate = timeClient.getFormattedDate();                   
  String justDate = ForDate.substring(0 , ForDate.indexOf( "T" ) ); 
  
  myOLED.clrScr();                      //Очистка буфера дисплея
  
  myOLED.setFont(BigNumbers);
  myOLED.print(timeStr , CENTER, 8 );   //Вывод информации на дисплей

  myOLED.setFont(SmallFont);
  myOLED.print(justDate, CENTER, 40);                               
  myOLED.print(String(temp) + " C", LEFT, 56);                      
  
  myOLED.update();                      //Обновление дисплея
}


void scr2(){
  myOLED.clrScr();                      //Очистка буфера дисплея
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);     //Вывод информации на дисплей
  myOLED.print(location, LEFT, 12);
  myOLED.print("Max: " + String(temp_max) + " C", LEFT, 22);
  myOLED.print("Min: " + String(temp_min) + " C", LEFT, 32);
  myOLED.print("Wind: " + String(wind) + "m/s", LEFT, 56);
  
  myOLED.print("C", 120, 14);
  
  myOLED.setFont(BigNumbers);

  if(temp <= -10){
    myOLED.print(String(temp), 70, 14); //От -10 до -бесконечности
  }
  if ((temp > -1) && (temp < -10) ) {   //От -1 до -10
    myOLED.print(String(temp), 89, 14);
  }
  if ((temp < 10) && (temp > -1)){      //От 0 до 10
    myOLED.print(String(temp), 102, 14);
  }
  if (temp >= 10){                      //От 10
    myOLED.print(String(temp), 89, 14);
  }
  
  myOLED.update();                      //Обновление дисплея
}

void scr3(){
  myOLED.clrScr();                              //Очистка буфера дисплея
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);             //Вывод информации на дисплей
  myOLED.print(location, LEFT, 12);
  myOLED.print(weather, LEFT, 22);
  myOLED.print("Clouds: " + String(clouds) + "%", LEFT, 32); 
  myOLED.print("%", 120, 14);

  if (description.length() <= 21){              
    myOLED.print(description, LEFT, 56);
  } else {                                      //Если строка больше 21 символа
    myOLED.setFont(TinyFont);                   //то уменьшаем шрифт 
    myOLED.print(description, LEFT, 56);
  }
  
  myOLED.setFont(BigNumbers);
  
  if (humidity == 100){                         //Если 100
    myOLED.print(String(humidity), 75, 14);
  } 
  if ((humidity >= 10) && (humidity != 100)) {  //Если От 10
    myOLED.print(String(humidity), 89, 14);
  } 
  if (humidity < 10) {                          //Если от 0 до 10
    myOLED.print(String(humidity), 102, 14);
  }
  
  myOLED.update();                              //Обновление дисплея
}

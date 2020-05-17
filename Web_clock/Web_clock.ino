///////////////Библиотеки////////////////
#include <NTPClient.h>                  //NTP клиент 
#include <ESP8266WiFi.h>                //Предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <WiFiUdp.h>                    //Обрабатывает отправку и получение пакетов UDP
#include <OLED_I2C.h>                   //Библиотека дисплея
#include <DNSServer.h>                  //Локальный DNS сервер для перенаправления всех запросов на страницу конфигурации
#include <ESP8266WebServer.h>           //Локальный веб сервер для страници конфигурации WiFi
#include <WiFiManager.h>                //Библиотека для удобного подключения к WiFi
#include <ESP8266HTTPClient.h>          //HTTP клиент
#include <ArduinoJson.h>                //Библиотека для работы с JSON
////////////////Настройки/////////////////

//При первом включении создана сеть,
//спомощью которой можно настроить Wi-Fi
//SSID:Connect-WIFI password:PASSWORD

const byte hhOn = 23;                   //Час включения ночного режима
const byte hhOff = 7;                   //Час выключения ночного режима

const byte devMode = 0;                 // 1 - Лог вкл 0 - лог выкл

const String lat = "55.75";             //Географические координаты
const String lon = "37.62";             //Москва

                                        //API ключ для openweathermap.org
const String appid = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; 

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
unsigned long timing, rndTiming, LostWiFiMillis, lastUpd; 

String timeStr;                         //Строка с временем с нулями через точку

byte curScr = 1;                        //Текущий экран

bool LostWiFi = false;                  //Флаг потери WiFi

int temp, temp_min, temp_max, wID;      //Переменные погоды: Тепература: сейчас, минимальная, максимальная. ID погоды
byte humidity, clouds;                  //Влажность и облака в процентах
String location, weather, description;  //Местоположение, погода, подробное описание погоды 
float wind;                             //Ветер в м/с
long timeOffset;                        //Оффсет времени
byte httpErrCount = 0;                  //Счетчик ошибок получения погоды 

int nightX, nightY;                     //Координаты надписи в ночном режиме

void setup(){
  unsigned long timer = millis();       //Таймер загрузки
  myOLED.begin(SSD1306_128X64);         //Инициализация дисплея
  bootScr("Starting Serial", 0);        //Отрисовка загрузочного экрана
  Serial.begin(115200);                 //Инициализация последовательного соединения
  
  wifiManager.setDebugOutput(devMode);  //Вкл выкл лога wifiManager
                                        //Подключение к сети
  bootScr("autoConnect", 25);
  wifiManager.autoConnect("Connect-WIFI", "PASSWORD");
  
  while (WiFi.status() != WL_CONNECTED){}

  bootScr("Updating weather", 50);      //Отрисовка загрузочного экрана
  int code = weatherUpdate();           //Обновление погоды
  if ( code != 200){                    //Если не обновляется
      myOLED.clrScr();                  //Выводим ошибку
      myOLED.print("Could not Access", CENTER, 16);
      myOLED.print("OpenWeatherMap", CENTER, 24);
      myOLED.print("Code:" + String( code ), CENTER, 32);
      myOLED.update();
      delay(1000);
      myOLED.invert(1);
      myOLED.update();
      delay(1000);
      myOLED.invert(0);
      myOLED.print("Reseting...", CENTER, 42);
      myOLED.update();
      delay(1000);
      ESP.reset();                      //Перезагружаемся
  }
  lastUpd = millis();

  bootScr("Starting NTPClient", 75);    //Отрисовка загрузочного экрана
  timeClient.begin();                   //Инициализация NTP клиента
  timeClient.setTimeOffset(timeOffset); //Установка оффсета времени

  randomSeed(millis());                 //Установка ключа генерации рандомных чисел

                                        //Отрисовка загрузочного экрана
  bootScr("Done in " + String( millis() - timer ) + "ms", 100);
  delay(700);
  myOLED.clrScr();
  timing = millis();                    //Таймер смены экранов
  rndTiming = 0;                        //Таймер рандомного перемещения часов в ночном режиме
}



void loop() {
  timeClient.update();                //Обновление времени
  
  if (millis() - lastUpd > 120000){   //Обновление погоды раз в 2 минуты
      weatherUpdate();
      lastUpd = millis();
  }

  if (WiFi.status() != WL_CONNECTED) {//Если нет подключения
    if (LostWiFi == 0){
      LostWiFi = 1;
      LostWiFi = 1;                   //Ставим флаг
      LostWiFiMillis = millis();
      } else if(millis() - LostWiFiMillis > 180000) {
        
                                      //Если прошло 3 мин
      myOLED.clrScr();                //Выводим ошибку
      myOLED.print("Wifi connection", CENTER, 16);
      myOLED.print("lost", CENTER, 24);
      myOLED.update();
      delay(1000);
      myOLED.invert(1);
      myOLED.update();
      delay(1000);
      myOLED.invert(0);
      myOLED.print("Reseting...", CENTER, 42);
      myOLED.update();
      delay(1000);
      ESP.reset();                    //Перезагружаемся
    }
  }
  
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();       //Запись времени без нулей
  ss = timeClient.getSeconds();       //в переменные
  
  updScr();                           //Обновляем экран
}



int weatherUpdate() {                  //Функция обновления погоды 
  if (WiFi.status() == WL_CONNECTED) { //Выполняется только если WiFi подключен
    
    logIf("Updating weather");
                                       //Формирование строки запроса, знаю, что тупо сделал
    String httpStr = String("http://api.openweathermap.org/data/2.5/weather") + String("?lat=") + String(lat) + String("&lon=") + String(lon) + String("&appid=") + String(appid) + String("&units=metric&lang=en");
    http.begin(httpStr);
    
    logIf("Accessing: " + httpStr);
    
    int httpCode = http.GET();         //Запрос + получение http кода
    String json = http.getString();    //Получение строки ответа
    logIf("Http Code: " + String(httpCode) );
    logIf("Got JSON: " + json);
    http.end();

    if(httpCode != 200) {              //Если код не 200
      httpErrCount++;                  //Инкремент счетчика ошибок
      logIf( "httpErrCount: " + String(httpErrCount) );
      return httpCode;                 //Возвращаем код
    }
                                       //Парсинг JSON
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {                       //Если не парсится
      if (devMode) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
      }
      return httpCode;                 //Возвращаем http код
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
    timeOffset = doc["timezone"];
    wID = doc["weather"][0]["id"];
    
    httpErrCount = 0;                  //Обнуляем счетчик ошибок
    
    return httpCode;                   //Возвращаем http код
  }
}


                                         //Вывод лога если включен режим отладки
void logIf(String msg){if(devMode){Serial.println(msg);}}



void bootScr(String str, byte percent) { //Функция отрисовки загрузочного экрана
  myOLED.setFont(SmallFont);             //Установка шрифта
  myOLED.clrScr();  
  myOLED.print(">:" + str, LEFT, 56);    //Строка состояния
  String percentBar = "[";               //Формирование прогресбара
  for ( byte i = 1; i <= (percent / 10); i++){
    percentBar = percentBar + "=";
  }
  for ( byte i = 0; i < (10 - (percent / 10) ); i++){
    percentBar = percentBar + "-";
  }                                      //Вывод на экран
  percentBar = percentBar + "] | " + String(percent) + "%";
  myOLED.print(percentBar, CENTER, 48);
  myOLED.update();                       //Обновление дисплея
}



void scr1() {                           //Функция отрисовки 1-го экрана
                                        //Форматирование строк
  String ForDate = timeClient.getFormattedDate();                   
  String justDate = ForDate.substring(0 , ForDate.indexOf( "T" ) ); 
  
  myOLED.clrScr();                      //Очистка буфера дисплея
  
  myOLED.setFont(BigNumbers);
  myOLED.print(timeStr , CENTER, 8 );   //Вывод информации на дисплей

  myOLED.setFont(SmallFont);
  myOLED.print(justDate, CENTER, 40);       
  
  if (httpErrCount < 10){               //Если счетчик ошибок больше 10, то не пишем темп-ру
    myOLED.print(String(temp) + " C", LEFT, 56);
  }                      
  
  myOLED.update();                      //Обновление дисплея
}



void scr2(){                            //Функция отрисовки 2-го экрана
  myOLED.clrScr();                      //Очистка буфера дисплея
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);     //Вывод информации на дисплей
  myOLED.print("Max: " + String(temp_max) + " C", LEFT, 14);
  myOLED.print("Min: " + String(temp_min) + " C", LEFT, 24);
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
  if (httpErrCount < 10){               //Если счетчик ошибок меньше 10, то
    myOLED.update();                    //Обновление дисплея
  } else {
    myOLED.clrScr();                    //Иначе выводим сообщение об ошибке
    myOLED.setFont(SmallFont);
    myOLED.print("Could not Access", CENTER, 16);
    myOLED.print("OpenWeatherMap", CENTER, 24);
    myOLED.update();                    //Обновление дисплея
  }
}



void scr3(){                                    //Функция отрисовки 3-го экрана
  myOLED.clrScr();                              //Очистка буфера дисплея
  myOLED.setFont(SmallFont);
  myOLED.print(timeStr, CENTER, 4);             //Вывод информации на дисплей
  
  if (wID > 800) {
    myOLED.print(weather + ":" + String(clouds) + "%", LEFT, 14);
  } else {
    myOLED.print(weather, LEFT, 14);
  }
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
  
  if (httpErrCount < 10){                       //Если счетчик ошибок меньше 10, то
    myOLED.update();                            //Обновление дисплея
  } else {
    myOLED.clrScr();                            //Иначе выводим сообщение об ошибке
    myOLED.setFont(SmallFont);
    myOLED.print("Could not Access", CENTER, 16);
    myOLED.print("OpenWeatherMap", CENTER, 24);
    myOLED.update();                            //Обновление дисплея
  }
}



void nightScr() {                             //Функция отрисовки ночного экрана
  //Размер надписи 48 x 8
  if (millis() - rndTiming > 60000){          //Если 1 минута прошла
    nightX = random(0, 80);                   //Генерим новые координаты для текста
    nightY = random(0, 56);
    rndTiming = millis();
  }
  myOLED.setBrightness(1);                    //Минимальная яркость
  myOLED.clrScr();
  myOLED.setFont(SmallFont);                  //Установка шрифта
  myOLED.print(timeStr, nightX, nightY);      //Выводим надпись по координатам
  myOLED.update();                            //Обновление дисплея
}



void updScr() {                       //Функция обновления и смены экранов 
   
  timeStr = "";                       //Форматирование строки времени 
  if (hh <= 9){                       //Добавляем точки и нули где нужно
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
  
  if ((hh >= hhOn) || (hh < hhOff)){  //Если время в нужном промежутке
    nightScr();                       //Отрисовываем ночной экран
    return;
  } else {
    myOLED.setBrightness(255);        //Выставляем макс. яркость
  }
  
  switch(curScr) {                    //Смена экранов
    case 1:                           //Если сейчас экран 1
      if (millis() - timing > 10000){ //Если прошло 10 сек
        timing = millis(); 
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
        curScr = 1;                   //То сейчас экран 1
        scr1();                       //Вызов функции отрисовки 1 дисплея
      }else{
        scr3();                       //Иначе вызов функции отрисовки 3 дисплея
        break;
      }
    break;
  }
}

//7 SEGMENT LED DISPLAY
#include "TM1637.h"
//YOUTUBE API
#include <YoutubeApi.h>
//ESP
#include <ESP8266WiFi.h>
//Wi-Fi Manger library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>//https://github.com/tzapu/WiFiManager

//YOUTUBE CONSTANTS
#define API_KEY "AIzaSyCLDE0nNfdyniru85b_F4b2VwQ9sv3yDsQ"  // My Google apps API Token
#define CHANNEL_ID "UC9I28GfJSS-s6pSZpqMt7IQ" //URL CHANNEL ID

//7 SEGMENT PIN CONSTANTS
#define CLK D4
#define DIO D5

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);
TM1637 tm1637(CLK,DIO);

unsigned long api_mtbs = 60000; //intervalo de tempo para cada pedido de stats no youtube
unsigned long api_lasttime;   //tempo do ultumo pedido
bool firstPush = true;
void setup() {
  //SETUP SERIAL Apenas para Debug
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("MyYoutubeStats");
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
}

void loop() {
 
  if (millis() - api_lasttime > api_mtbs|| firstPush)  {
    if(api.getChannelStatistics(CHANNEL_ID)){
      firstPush = false;
      int subs = api.channelStats.subscriberCount;
      tm1637.display(0, ((uint8_t) subs / 1000) % 10);
      tm1637.display(1,((uint8_t) subs / 100) % 10);
      tm1637.display(2,((uint8_t) subs / 10) % 10);
      tm1637.display(3,((uint8_t) subs / 1) % 10);
    }else{
      tm1637.display(0,0);
      tm1637.display(1,0);
      tm1637.display(2,0);
      tm1637.display(3,0);
      }
    api_lasttime = millis();
  }
  
}

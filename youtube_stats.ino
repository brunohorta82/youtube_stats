#include "DHT.h"
#include <PubSubClient.h>
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
#define API_KEY "SECRET"  // My Google apps API Token
#define CHANNEL_ID "UC9I28GfJSS-s6pSZpqMt7IQ" //URL CHANNEL ID
#define PHOTO_RESISTOR_PIN A0
//7 SEGMENT PIN CONSTANTS
#define CLK 5 //D1
#define DIO 14 //D5
#define SENSOR_NOTIFICATION_DELAY  5000
#define API_PUSH_DELAY  6000
DHT dht;
// Update these with values suitable for your network.
IPAddress server(192, 168,187,203);
WiFiClientSecure clientSec;
WiFiClient wclient;
PubSubClient client(server,1883,wclient);
YoutubeApi api(API_KEY, clientSec);
TM1637 tm1637(CLK,DIO);
long lastNotifTime = 0;
unsigned long api_lasttime;   //tempo do ultumo pedido
bool firstPush = true;
double totalTemp = 0;
double totalHum = 0;
double totalLdr = 0;
double totalRead = 0;
void setup() {
  //SETUP SERIAL Apenas para Debug
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  Serial.begin(9600);
  WiFiManager wifiManager;
  //reset saved settings
   // wifiManager.resetSettings();
   //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  wifiManager.autoConnect("MyYoutubeStats");
 // dht.setup(4);
}
void loop() {
 if (WiFi.status() == WL_CONNECTED) {
  
    if (!client.connected()) {
      if (client.connect("MailBoxSensor","moscas", "moscasMoscas82")) {
       delay(dht.getMinimumSamplingPeriod());
      double humidity = dht.getHumidity();
      double temperature = dht.getTemperature();
      double lux = light(analogRead(PHOTO_RESISTOR_PIN));
      totalTemp +=  temperature ;
      totalHum +=  humidity ;
      totalLdr += lux;
      totalRead++;
      unsigned long currentMillis = millis();
      if ((unsigned long)(currentMillis - lastNotifTime) >= SENSOR_NOTIFICATION_DELAY) {
        lastNotifTime = currentMillis;
        String hum = String(totalHum / totalRead,1);
        String temp = String(totalTemp / totalRead ,1);
        String ldr = String(totalLdr /totalRead ,1);
        client.publish("home-assistant/bunker/sensor/humidity",hum.c_str(), true);
        client.publish("home-assistant/bunker/sensor/temperature",temp.c_str(), true);
        client.publish("home-assistant/bunker/sensor/ldr",ldr.c_str(), true);
        totalTemp = 0;
        totalLdr= 0;
        totalHum = 0;
        totalRead = 0; 
      }
         if (millis() - api_lasttime > API_PUSH_DELAY|| firstPush)  {
    if(api.getChannelStatistics(CHANNEL_ID)){
      firstPush = false;
      int subs = api.channelStats.subscriberCount;
      int d1 = ((int) subs / 1000) % 10;
      if(d1 > 0){
        tm1637.display(0, d1);
      }
      tm1637.display(1,((int) subs / 100) % 10);
      tm1637.display(2,((int) subs / 10) % 10);
      tm1637.display(3,((int) subs / 1) % 10);
      String subsStr = String(subs);
      client.publish("home-assistant/social/youtube/suscribers",subsStr.c_str(), true);
    }
    api_lasttime = millis();
  }
      }else{
        Serial.println("MQTT ERROR!");
     }
    

  }

  if (client.connected()){
      client.loop();
   }
}
  
  
}

//Lux
double light (int RawADC0){
double Vout=RawADC0*0.004887585533;
int lux=(2500/Vout-500)/10;
return lux;
}


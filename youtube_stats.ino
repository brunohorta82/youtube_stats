#include "DHT.h"
#include <PubSubClient.h>
#include <Timing.h>
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
//OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//YOUTUBE CONSTANTS
#define API_KEY "AIzaSyCLDE0nNfdyniru85b_F4b2VwQ9sv3yDsQ"  // My Google apps API Token
#define CHANNEL_ID "UC9I28GfJSS-s6pSZpqMt7IQ" //URL CHANNEL ID
#define PHOTO_RESISTOR_PIN A0
//7 SEGMENT PIN CONSTANTS
#define CLK 5 //D1
#define DIO 14 //D5

const String HOSTNAME  = "MyYoutubeStats";
const char * OTA_PASSWORD  = "otapower";
const String mqttLog = "ota/log/"+HOSTNAME;
const String mqttSystemControlTopic = "system/set/"+HOSTNAME;
DHT dht;
// Update these with values suitable for your network.
IPAddress server(192, 168,187,203);
WiFiClientSecure clientSec;
WiFiClient wclient;
PubSubClient client(server,1883,wclient);
YoutubeApi api(API_KEY, clientSec);
TM1637 tm1637(CLK,DIO);

bool OTA = false;
bool OTABegin = false;
double humidity = 0;
double temperature = 0;
  int lux = 0;
  double lastLdrValue =0;
Timing notifTimer;
void setup() {
  Serial.begin(115200);
  notifTimer.begin(0);
  //SETUP SERIAL Apenas para Debug
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  WiFiManager wifiManager;
  //reset saved settings
   // wifiManager.resetSettings();
   //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  wifiManager.autoConnect(HOSTNAME.c_str(),"xptoxpto");
  client.setCallback(callback);
  dht.setup(4);  
}


void callback(char* topic, byte* payload, unsigned int length) {
  String payloadStr = "";
 if(String(topic) == mqttSystemControlTopic){
  for (int i=0; i<length; i++) {
    payloadStr += (char)payload[i];
  }
  if(payloadStr.equals("OTA_ON")){
    OTA = true;
    OTABegin = true;
  }else if (payloadStr.equals("OTA_OFF")){
    OTA = true;
    OTABegin = true;
  }else if (payloadStr.equals("REBOOT")){
    ESP.restart();
  }
 } 
} 
  
  
bool checkMqttConnection(){
  if (!client.connected()) {
    if (client.connect(HOSTNAME.c_str(),"homeassistant","moscasMoscas82")) {
      client.subscribe(mqttSystemControlTopic.c_str());
      getSubscribersAndPublish();
      messureTemperatureHumidityAndPublish();
      lastLdrValue =0 ;
      Serial.println("CONNECT");
    }
  }
  return client.connected();
}

void messureTemperatureHumidityAndPublish(){
     humidity = 0;
         temperature = 0;
         delay(dht.getMinimumSamplingPeriod());
          for(int i = 0 ;i <1000; i++){
               humidity += dht.getHumidity();
               temperature += dht.getTemperature();
                client.loop();
          } 
              
              
        String hum = String(humidity / 1000,1);
        String temp = String(temperature / 1000 ,1);
        Serial.println(temp);
        client.publish("home-assistant/bunker/sensor/humidity",hum.c_str(), true);
        client.publish("home-assistant/bunker/sensor/temperature",temp.c_str(), true);
}
void getSubscribersAndPublish(){
  if(api.getChannelStatistics(CHANNEL_ID)){
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
  }
void messureLuxAndPublish(){
         lux = 0;
      for(int i = 0 ;i <10; i++){
         lux+=  analogRead(PHOTO_RESISTOR_PIN);
         delay(50);  
      }
     lux = light(lux/10);
       if(abs(lux-lastLdrValue) > 1 ){
        client.publish("home-assistant/bunker/sensor/ldr",String(lux).c_str(), true);
        lastLdrValue = lux;
        }
 }
void loop() {
  
 if (WiFi.status() == WL_CONNECTED) {
  
    if (checkMqttConnection()){
     client.loop();
     if (notifTimer.onTimeout(1800000)){
          messureTemperatureHumidityAndPublish();
          getSubscribersAndPublish();
      }
       messureLuxAndPublish();
         //CHECK IF OTA IS ON OR OFF
       if(OTA){
        if(OTABegin){
          setupOTA();
          OTABegin= false;
        }
        ArduinoOTA.handle();
      }
    }
    }
}

void setupOTA(){
  if (WiFi.status() == WL_CONNECTED && checkMqttConnection()) {
    client.publish(mqttLog.c_str(),"OTA SETUP");
    ArduinoOTA.setHostname(HOSTNAME.c_str());
    ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
    
    ArduinoOTA.onStart([]() {
    client.publish(mqttLog.c_str(),"START");
  });
  ArduinoOTA.onEnd([]() {
    client.publish(mqttLog.c_str(),"END");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    String p = "Progress: "+ String( (progress / (total / 100)));
    client.publish(mqttLog.c_str(),p.c_str());
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) client.publish(mqttLog.c_str(),"Auth Failed");
    else if (error == OTA_BEGIN_ERROR)client.publish(mqttLog.c_str(),"Auth Failed"); 
    else if (error == OTA_CONNECT_ERROR)client.publish(mqttLog.c_str(),"Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)client.publish(mqttLog.c_str(),"Receive Failed");
    else if (error == OTA_END_ERROR)client.publish(mqttLog.c_str(),"End Failed"); 
  });
 ArduinoOTA.begin();
 }  
}

int light (int RawADC0){
int Vout=RawADC0*0.004887585533;
int lux=500/((9.72/(5-Vout))*Vout);
return lux;
}


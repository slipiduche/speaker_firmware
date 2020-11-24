#include "includes.h"

void loop() ///nfc LOOP
{           //Serial.print("nfcloop running on core ");
  // Serial.println(xPortGetCoreID());
  wifiLedBlink();
  if (guardarAp == 1)
  {
    save_config1_spiff();
    ESP.restart();
  }

  mp3Loop();
}
void WebComm(void *parameter) ///webloop
{
  for (;;)
  {
    if ((inicio == 0) && (apMode == 0))
    {
      wifi_mqtt_setup();
    }
    if ((inicio == 1) && (apMode == 0))
    { //DEBUG_PRINT("inicio1:");
      //DEBUG_PRINTLN(inicio);
      wifi_mqtt_reconnect(MQTTTopic, MQTTTopic2); //mqtt protocol
    }
    //Serial.print("WebComm() running on core ");
    // Serial.println(xPortGetCoreID());
    //MQTT
    if ((inicio == 2) && (apMode == 0))
    { //DEBUG_PRINT("inicio2:");
      //DEBUG_PRINTLN(inicio);
      //DEBUG_PRINT("client state:");
      //DEBUG_PRINTLN(mqttclient.state());
      if (mqttclient.state() != 0 || subscribed == 0)
      {
        if (mqttclient.state() != -1) ///-1 disconnected
        {
          mqttclient.disconnect();
        }
        subscribed = 0;
        wifi_mqtt_reconnect(MQTTTopic, MQTTTopic2); //mqtt protocol
      }
      if (subscribed == 0)
      {
        String topic_s = "";
        topic_s = "SERVER/POLL";
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
        topic_s = "";
        topic_s = "SERVER/" + chipid;
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
      }
    }
    wifi_mqtt_loop();
  }
  vTaskDelay(10000);
}
void APmode(void *parameter) ///APmode
{
  for (;;)
  {
    if ((apMode) && (!apActivate))
    {
      apActivate = 1;
      setupAPSSID(0);
      web_setup();
    }
    if (cambioIp)
    {
      apActivate = 1;
      cambioIp=0;
      setupAPSSID(1);
      web_setup();
    }
    
    //Serial.print("APmode() running on core ");
    //Serial.println(xPortGetCoreID());
    apweb_loop();
  }
  vTaskDelay(3000);
}

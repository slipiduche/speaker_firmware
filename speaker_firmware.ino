#include <ArduinoJson.h>

#include "includes.h"

void loop() ///nfc LOOP
{           //Serial.print("nfcloop running on core ");
  // Serial.println(xPortGetCoreID());
  if (!digitalRead(0) && (apMode == 0) && (abs(millis() - apDelay) >= 500))
  {

    apDelayCount++;
    apDelay = millis();
    if (apDelayCount > 5)
    { count = 0;
      apMode = 1;
      apActivate = 0;
    }
  }
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
    if ((inicio == 0) && ((apMode == 0)))
    {
      claimSPI("WebComm"); // Claim SPI bus
      wifi_mqtt_setup();
      releaseSPI(); // Release SPI bus
    }
    if ((inicio == 1) && ((apMode == 0)))
    { //DEBUG_PRINT("inicio1:");
      //DEBUG_PRINTLN(inicio);
      claimSPI("WebComm");                        // Claim SPI bus
      wifi_mqtt_reconnect(MQTTTopic, MQTTTopic2); //mqtt protocol
      releaseSPI();                               // Release SPI bus
    }
    //Serial.print("WebComm() running on core ");
    // Serial.println(xPortGetCoreID());
    //MQTT
    if ((inicio == 2) && ((apMode != 1)))
    {
      // DEBUG_PRINT("inicio2:");
      // DEBUG_PRINTLN(inicio);
      // DEBUG_PRINT("client state:");
      // DEBUG_PRINTLN(mqttclient.state());
      if (mqttclient.state() != 0 || subscribed == 0)
      {
        if (mqttclient.state() != -1) ///-1 disconnected
        {
          mqttclient.disconnect();
          subscribed = 0;
        }
        if (mqttclient.state() == -1)
        {
          apMode = 0;
          DEBUG_PRINT("esta pasando esto");
          claimSPI("WebComm");                        // Claim SPI bus
          wifi_mqtt_reconnect(MQTTTopic, MQTTTopic2); //mqtt protocol
          releaseSPI();                               // Release SPI bus
        }
      }
      if (subscribed == 0 && (mqttclient.state() == 0))
      {
        DEBUG_PRINT("esta pasando esto?");
        String topic_s = "";
        topic_s = "SERVER/POLL";
        DEBUG_PRINTLN(topic_s);
        claimSPI("WebComm");
        wifi_mqtt_subscribe(topic_s.c_str());
        releaseSPI(); // Release SPI bus
        topic_s = "";
        topic_s = "SERVER/" + chipid;
        DEBUG_PRINTLN(topic_s);
        claimSPI("WebComm");
        wifi_mqtt_subscribe(topic_s.c_str());
        releaseSPI(); // Release SPI bus
      }
    }

    if (mqttclient.state() == 0 && (apMode != 1))
    {
      claimSPI("WebComm"); // Claim SPI bus
      wifi_mqtt_loop();
      releaseSPI(); // Release SPI bus
    }
  }
  vTaskDelay(10000);
}
void APmode(void *parameter) ///APmode
{
  for (;;)
  {

    if ((apMode) && (!apActivate)) //forzado
    {
      apActivate = 1;
      setupAPSSID(0);
      web_setup();
    }
    if (cambioIp) ///ya conectado a una red wifi ip asignada
    {

      apActivate = 1;
      cambioIp = 0;
      setupAPSSID(1);
      web_setup();
      apMode = 2;
    }
    if (apMode != 0) /// apMode 1 forzado  2 ya conectado a una red wifi
    {                //Serial.print("APmode() running on core ");
      //Serial.println(xPortGetCoreID());
      claimSPI("apmode"); // Claim SPI bus
      apweb_loop();
      releaseSPI(); // Release SPI bus
    }
  }
  vTaskDelay(3000);
}

#ifdef USEWIFIMQTT

void callback(char *topic, byte *payload, unsigned int length)
{
  memset(datarecvd, 0, 512);
  DEBUG_PRINT("Message arrived in topic: ");
  DEBUG_PRINTLN(topic);

  DEBUG_PRINT("Message:");
  for (int i = 0; i < length; i++)
  {
    DEBUG_PRINT((char)payload[i]);
    datarecvd[i] = payload[i];
  }

  DEBUG_PRINTLN();
  DEBUG_PRINTLN("-----------------------");
  loadMqttCommand(String(datarecvd));
}

void wifi_mqtt_setup()
{
  bussyMqtt = 1;
  count = 1;
  WiFi.begin(ssid, password);
  DEBUG_PRINTLN("wifi connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DEBUG_PRINT(".");
    count++;
    if (count > 50)
    {
      count = 0;
      apMode = 1;
      setupAPSSID(0);
      break;
    }
    wifiLedBlink();
  }
  if (count > 0)
  {
    DEBUG_PRINTLN("");
    DEBUG_PRINT(ssid);
    DEBUG_PRINT(" RSSI:");
    WRSSI = String(WiFi.RSSI());
    DEBUG_PRINTLN(WRSSI);
    DEBUG_PRINTLN("WiFi connected");
    DEBUG_PRINTLN("STATION IP address: ");

    ipRed = String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
    DEBUG_PRINTLN(ipRed);
    cambioIp = 1;
    count = 0;
  }
  String MQTTPORT = String(MQTTPort);
  DEBUG_PRINTLN(MQTTHost);
  DEBUG_PRINTLN(MQTTPORT.toInt());

  mqttclient.setServer(MQTTHost, MQTTPORT.toInt());
  mqttclient.setCallback(callback);

  wifi_mqtt_reconnect_setup(MQTTTopic, MQTTTopic2);
  bussyMqtt = 0;
}

void wifi_mqtt_reconnect(char mqtttopic[120], char mqtttopic2[120])
{
  bussyMqtt = 1;
  char topicCh[120];

  String topic_s = "";

  if ((WiFi.status() == WL_CONNECTED) && (apMode == 0))
  {
    while (!mqttclient.connected())
    {
      DEBUG_PRINT("Attempting MQTT connection...");

      String topic_s = clientId;

      DEBUG_PRINTLN(MQTTUsername);
      DEBUG_PRINTLN(MQTTPassword);
      if (mqttclient.connect(clientId.c_str(), MQTTUsername, MQTTPassword))
      {
        DEBUG_PRINTLN("WCONNECT PACKET SUCCESS");
        inicio = 2;
        wifi_mqtt_publish(topic_s.c_str(), "{\"mqtt\": \"RECONNECTED\"}");
        topic_s = "";

        topic_s = "SERVER/POLL";
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
        topic_s = "";
        topic_s = "SERVER/" + chipid;
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
        reconnect = 0;
        bussyMqtt = 0;
      }
      else
      {
        DEBUG_PRINT("failed, rc=");
        DEBUG_PRINT(mqttclient.state());
        DEBUG_PRINTLN(" try again in 5 seconds");
#ifdef AP

#endif
        reconnect++;

        if ((reconnect > 4) && (apMode == 0))
        {
          apMode = 1;
          reconnect = 0;
          WiFi.reconnect();
          inicio = 1;
        }
      }
      wifiLedBlink();
    }
  }
}

void wifi_mqtt_reconnect_setup(char mqtttopic[120], char mqtttopic2[120])
{
  String topic_s = "";
  bussyMqtt = 1;
  if ((WiFi.status() == WL_CONNECTED) && (apMode == 0))
  {
    while (!mqttclient.connected())
    {
      DEBUG_PRINT("Attempting MQTT connection...");

      topic_s = clientId;
      DEBUG_PRINTLN(topic_s);

      DEBUG_PRINTLN(MQTTUsername);
      DEBUG_PRINTLN(MQTTPassword);

      if (mqttclient.connect(clientId.c_str(), MQTTUsername, MQTTPassword))
      {
        DEBUG_PRINTLN("WCONNECT PACKET SUCCESS");
        inicio = 2;
        wifi_mqtt_publish(topic_s.c_str(), "{\"mqtt\": \"RECONNECTED\"}");
        topic_s = "";

        topic_s = "SERVER/POLL";
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
        topic_s = "";
        topic_s = "SERVER/" + chipid;
        DEBUG_PRINTLN(topic_s);
        wifi_mqtt_subscribe(topic_s.c_str());
        reconnect = 0;
        bussyMqtt = 0;
      }
      else
      {
        DEBUG_PRINT("failed, rc=");
        DEBUG_PRINT(mqttclient.state());
        DEBUG_PRINTLN(" try again in 5 seconds");
#ifdef AP

#endif
        reconnect++;

        if ((reconnect > 4) && (apMode == 0))
        {
          apMode = 1;
          reconnect = 0;
          WiFi.reconnect();
          inicio = 1;
          break;
        }
      }
      wifiLedBlink();
    }
  }
}

int wifi_mqtt_publish(String mqtttopic, String msg)
{
  bussyMqtt = 1;
  DEBUG_PRINTLN("mqtt_connected:");
  bool mqtt_connected = mqttclient.connected();
  DEBUG_PRINTLN(mqtt_connected);
  if (mqttclient.connected())
  {
    mqttclient.publish(mqtttopic.c_str(), msg.c_str());
    DEBUG_PRINTLN("WPUBLISH PACKET SENT");
    bussyMqtt = 0;
    return 1;
  }
  else
  {
    bussyMqtt = 0;
    return 0;
  }
}

int wifi_mqtt_subscribe(String mqtttopic)
{
  bussyMqtt = 1;
  DEBUG_PRINTLN("mqtt_connected:");
  bool mqtt_connected = mqttclient.connected();
  DEBUG_PRINTLN(mqtt_connected);
  if (mqtt_connected)
  {
    if (mqttclient.subscribe((mqtttopic).c_str()))
    {
      wifi_mqtt_publish((clientId), "{\"mqtt\": \"SUBSCRIBED\"}");
      DEBUG_PRINTLN("WSUBSCRIBE PACKET SENT");
      subscribed = 1;
      apMode = 0;
      minutosEnApMode = 0;
      bussyMqtt = 0;
      return 1;
    }
  }
  else
  {
    bussyMqtt = 0;
    return 0;
  }
}

void wifi_mqtt_loop()
{

  mqttclient.loop();
  if (abs(millis() - mqttdelay) >= 500)
  {
    

    
    if (serverPoll)
    {

      if (wifi_mqtt_publish(("SPEAKER/INFO"), "{\"NAME\": \"" + String(devName) + "\",\"CHIP_ID\":\"" + chipid + "\"}"))

      {
        Serial.print("SE ENVIO POLL");
        serverPoll = 0;
      }
    }
    if(statusPlay==1)
    {
      if (wifi_mqtt_publish(("SPEAKER/"+String(chipid)), "{\"STATUS\":\"PLAYING\"}"))

      {
        Serial.print("SE ENVIO STATUS Playing");
        statusPlay=0;
        hostreq = true;
      }
    }
    else if(statusPlay==2)
    {
      if (wifi_mqtt_publish(("SPEAKER/"+String(chipid)), "{\"STATUS\":\"OK\"}"))

      {
        Serial.print("SE ENVIO STATUS ok");
        statusPlay=0;
      }
    }
    mqttdelay = millis();
  }
}

#else
void callback(char *topic, byte *payload, unsigned int length) {}
void wifi_mqtt_setup() {}
void wifi_mqtt_reconnect(char mqtttopic[120], char mqtttopic2[120]) {}
int wifi_mqtt_publish(char mqtttopic[120], char msg[512]) {}
int wifi_mqtt_subscribe(char mqtttopic[120]) {}
void wifi_mqtt_loop() {}
void mqtt_send_changes() {}
#endif


void web_setup(void)
{ 
  DEBUG_PRINTLN("Creating Accesspoint");
  WiFi.softAP(ssid2, password2); //Create Access Point
  DEBUG_PRINT("AP IP address:\t");
  DEBUG_PRINTLN(WiFi.softAPIP());

  server.on("/", dsetup);
  server.on("/getData", getData);
  server.on(
      "/putData", putData); ///get request
  server.begin();
  DEBUG_PRINTLN("HTTP server started");
}

#ifdef AP
void apweb_loop(void)
{
  server.handleClient(); //
}
#else
void apweb_loop()
{
}
#endif

void dsetup()
{
  minutosEnApMode = 0;
  if (server.args() > 6)
  {

    if (server.hasArg(String("SSID")))
    {

      memset(ssid, '\0', sizeof(ssid));
      (server.arg(String("SSID"))).toCharArray(ssid, (server.arg(String("SSID"))).length() + 1);
    }
    if (server.hasArg(String("Password")))
    {

      memset(password, '\0', sizeof(password)); //
      (server.arg(String("Password"))).toCharArray(password, (server.arg(String("Password"))).length() + 1);
    }
    if (server.hasArg(String("APSSID")))
    {

      memset(devName, '\0', sizeof(devName)); //
      (server.arg(String("APSSID"))).toCharArray(devName, (server.arg(String("APSSID"))).length() + 1);
    }
    if (server.hasArg(String("AP_Password")))
    {

      memset(password2, '\0', sizeof(password2)); //
      (server.arg(String("AP_Password"))).toCharArray(password2, (server.arg(String("AP_Password"))).length() + 1);
    }
    if (server.hasArg(String("WEB_Host")))
    {

      memset(MQTTHost, '\0', sizeof(MQTTHost)); //
      (server.arg(String("WEB_Host"))).toCharArray(MQTTHost, (server.arg(String("WEB_Host"))).length() + 1);
    }
    if (server.hasArg(String("URL1")))
    {

      memset(MQTTPort, '\0', sizeof(MQTTPort)); //
      (server.arg(String("URL1"))).toCharArray(MQTTPort, (server.arg(String("URL1"))).length() + 1);
    }
    if (server.hasArg(String("URL2")))
    {

      memset(MQTTUsername, '\0', sizeof(MQTTUsername)); //
      (server.arg(String("URL2"))).toCharArray(MQTTUsername, (server.arg(String("URL2"))).length() + 1);
    }
    if (server.hasArg(String("URL3")))
    {

      memset(MQTTPassword, '\0', sizeof(MQTTPassword)); //
      (server.arg(String("URL3"))).toCharArray(MQTTPassword, (server.arg(String("URL2"))).length() + 1);
    }

    if (modo_nowc == 1)
    {
      setupAPSSID(1);
      SendHTML_Header();
      webpage += F("<script>");
      webpage += F("  function pulsar(e) {");
      webpage += F("    tecla = (document.all) ? e.keyCode :e.which;");
      webpage += F("    return (tecla!=13);");
      webpage += F("  }");
      webpage += F("  </script>");
      webpage += F("<h3>Parameters Setup</h3>");
      webpage += F("<FORM action='/' method='post' enctype='multipart/form-data'>");
      webpage += F("<p>SSID:<input type='text' name='SSID' value='");
      webpage += String(ssid);
      webpage += F("' minlength='4' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>Password:<input type='text' name='Password' value='");
      webpage += String(password);
      webpage += F("' minlength='8' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>Network IP: <b>");
      webpage += ipRed;
      webpage += F("</b></p>");
      webpage += F("<p>AP SSID:<input type='text' name='APSSID' value='");
      webpage += String(devName);
      webpage += F("' minlength='4' maxlength='11' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>AP Password:<input type='text' name='AP_Password' value='");
      webpage += String(password2);
      webpage += F("' minlength='8' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>AP Network IP: <b>");
      webpage += F("192.168.4.1</b></p>");
      webpage += F("<p>MQTT_Host:<input type='text' name='WEB_Host' value='");
      webpage += String(MQTTHost);
      webpage += F("' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>MQTT_Port:<input type='text' name='URL1' value='");
      webpage += String(MQTTPort);
      webpage += F("' required onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>MQTT_UserName:<input type='text' name='URL2' value='");
      webpage += String(MQTTUsername);
      webpage += F("' onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>MQTT_Password:<input type='text' name='URL3' value='");
      webpage += String(MQTTPassword);
      webpage += F("' onkeypress=\"return pulsar(event)\"></p>");
      webpage += F("<p>ChipID: <b>");
      webpage += String(chipid);
      webpage += F("</b></p>");
      webpage += F("<br>Rebooting... wait until device turn on AP mode.<br>");
      webpage += F("</FORM>");
      webpage += F("<a href='/'>[Back]</a><br><br> ");
      append_page_footer();
      SendHTML_Content();
      SendHTML_Stop();
      guardarAp = 1;
    }
  }
  else
  {
    SendHTML_Header();
    webpage += F("<script>");
    webpage += F("  function pulsar(e) {");
    webpage += F("    tecla = (document.all) ? e.keyCode :e.which;");
    webpage += F("    return (tecla!=13);");
    webpage += F("  }");
    webpage += F("  </script>");
    webpage += F("<h3>Parameters Setup</h3>");
    webpage += F("<FORM action='/' method='post' enctype='multipart/form-data'>");
    webpage += F("<p>SSID:<input type='text' name='SSID' value='");
    webpage += String(ssid);
    webpage += F("' minlength='4' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>Password:<input type='text' name='Password' value='");
    webpage += String(password);
    webpage += F("' minlength='8' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>Network IP: <b>");
    webpage += ipRed;
    webpage += F("</b></p>");
    webpage += F("<p>AP SSID:<input type='text' name='APSSID' value='");
    webpage += String(devName);
    webpage += F("' minlength='4' maxlength='11' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>AP Password:<input type='text' name='AP_Password' value='");
    webpage += String(password2);
    webpage += F("' minlength='8' maxlength='60' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>AP Network IP: <b>");
    webpage += F("192.168.4.1</b></p>");
    webpage += F("<p>MQTT_Host:<input type='text' name='WEB_Host' value='");
    webpage += String(MQTTHost);
    webpage += F("' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>MQTT_Port:<input type='text' name='URL1' value='");
    webpage += String(MQTTPort);
    webpage += F("' required onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>MQTT_UserName:<input type='text' name='URL2' value='");
    webpage += String(MQTTUsername);
    webpage += F("' onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>MQTT_Password:<input type='text' name='URL3' value='");
    webpage += String(MQTTPassword);
    webpage += F("' onkeypress=\"return pulsar(event)\"></p>");
    webpage += F("<p>ChipID: <b>");
    webpage += String(chipid);
    webpage += F("</b></p>");
    webpage += F("<br><button class='buttons'  type='submit'>Send</button><br>");
    webpage += F("</FORM>");
    webpage += F("<a href='/'>[Back]</a><br><br> ");
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
  }
}

void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
void SendJson(String json)
{
  server.send(200, "application/json", json);
}

void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}

void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop();
}

void getData()
{
  webpage = "";
  webpage += F("{\"SSID\":\"");
  webpage += String(ssid);
  webpage += F("\",\"PASSWORD\":\"");
  webpage += String(password);
  webpage += ("\",\"CHIPID\":\"");
  webpage += String(chipid);
  webpage += F("\",\"NAME\":\"");
  webpage += String(devName);
  webpage += F("\"}");
  SendJson(webpage);

  SendHTML_Stop();
}

void putData()
{
  Serial.print("GET....");
  Serial.print(server.args());
  Serial.print(server.arg("plain"));
  String body = server.arg("plain");
  Serial.print(body);
  if (server.hasArg(String("SSID")))
  {

    if (server.hasArg(String("SSID")))
    {

      memset(ssid, '\0', sizeof(ssid));
      (server.arg(String("SSID"))).toCharArray(ssid, (server.arg(String("SSID"))).length() + 1);
    }
    if (server.hasArg(String("PASSWORD")))
    {
      memset(password, '\0', sizeof(password)); //
      (server.arg(String("PASSWORD"))).toCharArray(password, (server.arg(String("PASSWORD"))).length() + 1);
    }
    if (server.hasArg(String("NAME")))
    {
      memset(devName, '\0', sizeof(devName)); //
      (server.arg(String("NAME"))).toCharArray(devName, (server.arg(String("NAME"))).length() + 1);
    }
    if (modo_nowc == 1)
    {
      setupAPSSID(1);
      webpage = "";
      webpage += F("{\"MESSAGE\":\"SUCCESS\"}");
      SendJson(webpage);
      SendHTML_Stop();
      guardarAp = 1;
    }
  }
  else
  {

    server.send(405, "application/json", F("{\"ERROR\":\"Method Not Allowed\"}"));
    SendHTML_Content();
    SendHTML_Stop();
  }
}

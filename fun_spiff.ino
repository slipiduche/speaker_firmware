#ifdef fun_spiff

void logValue2() //
{
  logFile.print("set1");
  logFile.print(",");
  logFile.print(ssid);
  logFile.print(",");
  logFile.print(password);
  logFile.print(",");
  logFile.print(ssid2);
  logFile.print(",");
  logFile.print(password2);
  logFile.print(",");
  logFile.print(String(MQTTHost));
  logFile.print(",");
  logFile.print(String(MQTTPort));
  logFile.print(",");
  logFile.print(String(MQTTUsername));
  logFile.print(",");
  logFile.print(String(MQTTPassword));
  logFile.print(",");
  logFile.print("1");
  logFile.print(",");
  logFile.print("\r\n");
  logFile.close();
}

void save_config1_spiff()
{
  if (SPIFFS.exists("/config1.txt"))
  {
    SPIFFS.remove("/config1.txt");

    DEBUG_PRINTLN(F("ERRASED"));
  }
  DEBUG_PRINTLN("saving in SPIFFS:");
  fun_spiff_setup();
  logFile = SPIFFS.open("/config1.txt", FILE_APPEND);
  DEBUG_PRINTLN("config1.txt");

  if (logFile)
  {
    logValue2();
    logFile.close();
  }
  else
  {
    logFile.close();
    DEBUG_PRINTLN("ERROR OPENNIG FILE");
    fun_spiff_setup();
  }
  logFile.close();
}

void read_spiffconfig1()
{
  fun_spiff_setup();
  logFile = SPIFFS.open("/config1.txt");
  DEBUG_PRINT(F("OPENNING FILE"));
  int totalBytes = logFile.size();
  String cadena = "";
  if (logFile)
  {
    if (lastposition >= totalBytes)
      lastposition = 0;
    logFile.seek(lastposition);
    while (logFile.available())
    {
      char caracter = logFile.read();
      cadena += String(caracter);
      lastposition = logFile.position();
      if (caracter == 10)
      {
        break;
      }
    }

    logFile.close();
    DEBUG_PRINT("length:");
    DEBUG_PRINTLN(cadena.length());
    DEBUG_PRINT("position:");
    DEBUG_PRINTLN(lastposition);

    DEBUG_PRINT("String readed:");
    DEBUG_PRINT(cadena);
    loadsdconfig(cadena);
  }
  else
  {
    logFile.close();
    DEBUG_PRINTLN(F("OPENNIG FILE ERROR"));
    fun_spiff_setup();
  }
}

void fun_spiff_setup()
{

  if (!SPIFFS.begin())
  {
    DEBUG_PRINTLN("SPIFFS Mount Failed");
    return;
  }
}

void fun_spiff_loop()
{
}
#else
void save_config1_spiff() {}
void save_config2_spiff() {}
void save_horarios_spiff() {}
void read_spiffconfig1() {}
void read_spiffconfig2() {}
void read_spiffhorarios() {}
void save_modo_spiff() {}
void read_spiffmodo() {}
void fun_spiff_setup() {}
void fun_spiff_loop() {}
#endif

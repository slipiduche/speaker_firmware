void loadMqttCommand(String datar)
{
    StaticJsonDocument<256> command;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(command, datar.c_str());

    // Test if parsing succeeds.
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    if (modo_nowc == 0)
    {
        DEBUG_PRINTLN("Ignored...");
    }
    else if (command["REQUEST"]=="INFO")//(datar.startsWith("{\"REQUEST\":\"INFO\"}"))
    {
        serverPoll = 1;
    }
    else if (command["ACTION"]=="START")
    {   mp3host=command["HOST"];
        mp3path=command["PORT"];
        mp3port=command["PATH"];
        hostreq = true;
        dbgprint("MQTT command host %s : %d %s",mp3host,mp3port,mp3path);
    }
    else if (command["ACTION"]=="STOP")//(datar.startsWith("{\"ACTION\":\"STOP\"}"))
    {
        datamode = STOPREQD;
    }
}

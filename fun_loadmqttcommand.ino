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
    else if (command["REQUEST"]=="INFO")//{"REQUEST":"INFO"}
    {
        serverPoll = 1;
    }
    else if (command["ACTION"]=="START")//{"ACTION":"START","HOST":"192.168.0.103","PORT":3412,"PATH":"/audio/andrew_rayel_impulse.mp3"}
    {   mp3host=command["HOST"];
        mp3path=command["PATH"];
        mp3port=command["PORT"];
        dbgprint("MQTT command host %s : %d %s",mp3host,mp3port,mp3path);
        
        statusPlay = 1;
        
    }
    else if (command["ACTION"]=="STOP")//4edcfab6224 med
    {
        datamode = STOPREQD;
    }
}

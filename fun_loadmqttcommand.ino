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
    { //{“ACTION”:/*”START” OR “STOP”*/,”URL”:”URL to song”}
        hostreq = true;
        Serial.println(datar);
    }
    else if (command["ACTION"]=="STOP")//(datar.startsWith("{\"ACTION\":\"STOP\"}"))
    {
        datamode = STOPREQD;
    }
}

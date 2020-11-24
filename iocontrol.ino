bool wifiLedOff()
{
  digitalWrite(wifiled, OFF);
  return false;
}

bool wifiLedOn()
{
  digitalWrite(wifiled, ON);
  return true;
}
bool NFCPOWEROn()
{
  digitalWrite(NFCPOWER, OFF);
  return true;
}
bool NFCPOWEROff()
{
  digitalWrite(NFCPOWER, ON);
  return true;
}
void wifiLedBlink()
{
  if ((subscribed == 0) || (WiFi.status() != WL_CONNECTED) || (mqttclient.state() != 0))
  {
    if (abs(millis() - blikDelay) >= 250)
    {

      blikDelay = millis();
      (wifiLedState) ? wifiLedState = wifiLedOff() : wifiLedState = wifiLedOn();
    }
  }
  else
  {
    wifiLedOn();
  }
}

void print_glcd_setup()
{
  Serial.println("emonGLCD solar PV monitor - gen and use");
  Serial.println("openenergymonitor.org");
  Serial.print("Node: "); 
  Serial.print(MYNODE); 
  Serial.print(" Freq: "); 
  if (freq == RF12_433MHZ) Serial.print("433Mhz");
  if (freq == RF12_868MHZ) Serial.print("868Mhz");
  if (freq == RF12_915MHZ) Serial.print("915Mhz"); 
  Serial.print(" Network: "); 
  Serial.println(group);
}

void print_emontx_payload()
{
  Serial.println();
  Serial.print("1 emontx: ");
  Serial.print(emontx.power1);
  Serial.print(' ');
  Serial.print(emontx.power2);
  Serial.print(' ');
  Serial.println(emontx.Vrms);
}

void print_emonbase_payload()
{
  Serial.print("2 base: ");
  Serial.print(emonbase.hour);
  Serial.print(':');
  Serial.print(emonbase.mins);
  Serial.print(':');
  Serial.print(emonbase.sec);
  Serial.print(' ');
  Serial.print(emonbase.outsideTemperature);
  Serial.print(' ');
  Serial.println(emonbase.newTime);
}

void print_immersion_payload()
{
  Serial.print("5 emonbase: ");
  Serial.print(ImmerCtl.power1);
  Serial.print(' ');
  Serial.print(ImmerCtl.Vrms);
  Serial.print(' ');
  Serial.println(ImmerCtl.SSRcontrol);
}

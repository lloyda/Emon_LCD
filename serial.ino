void print_glcd_setup()
{
  Serial.println("emonGLCD solar PV monitor");
  Serial.print("Node: ");
  Serial.print(MYNODE);
  Serial.print(" Freq: ");
  if (freq == RF12_433MHZ) Serial.print("433Mhz");
  if (freq == RF12_868MHZ) Serial.print("868Mhz");
  Serial.print(" Network: ");
  Serial.println(group);
  Serial.flush();
}

void print_emontx_payload()
{
  // Serial.println();
  Serial.print("1:");
  Serial.print(emontx.power1);
  Serial.print(' ');
  Serial.print(emontx.power2);
  Serial.print(' ');
  Serial.println(emontx.Vrms);
  Serial.flush ();
}

void print_emonbase_payload()
{
  Serial.print("2:");
  Serial.print(emonbase.hour);
  Serial.print(':');
  Serial.print(emonbase.mins);
  Serial.print(':');
  Serial.print(emonbase.sec);
  Serial.print(' ');
  Serial.print(emonbase.outsideTemperature);
  Serial.print(' ');
  Serial.println(emonbase.newTime);
  Serial.flush ();
}

void print_immersion_payload()
{
  Serial.print("5:");
  Serial.print(ImmerCtl.temperature);
  Serial.print (' ');
  Serial.print(ImmerCtl.power1);
  Serial.print(' ');
  Serial.print(ImmerCtl.Vrms);
  Serial.print(' ');
  Serial.println(ImmerCtl.SSRcontrol);
  Serial.flush ();
}

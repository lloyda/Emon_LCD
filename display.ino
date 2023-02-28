//GLCD fonts - part of GLCD lib
#include "utility/font_helvB14.h" 	//big numberical digits 
#include "utility/font_clR6x8.h" 	//title
#include "utility/font_clR4x6.h" 	//kwh

#define OFF 0
#define GREEN 1
#define RED 2
#define ORANGE 3

double cval, cval2, cval3, cval4;   //values to calulate smoothing
// int direction = -1;
short int ledlevel = 200;
int ledstate = OFF;
short int redledlevel;
short int greenledlevel;

void draw_main_screen()
{
  glcd.clear();
  glcd.fillRect(0, 0, 128, 64, 0);

  glcd.drawLine(64, 0, 64, 64, WHITE);      //top vertical line
  glcd.drawLine(0, 32, 128, 32, WHITE);     //middle horizontal line

  char str[40];    			 //variable to store conversion
  glcd.setFont(font_clR6x8);
  glcd.drawString_P(12, 0, PSTR("using"));
  glcd.drawString_P(69, 0, PSTR("solar PV"));
  if (immersionOn) glcd.drawString_P(69, 34, PSTR("immersion")); else glcd.drawString_P(69, 34, PSTR("outside"));

  if (importing == 1) glcd.drawString_P(2, 34, PSTR("importing")); else glcd.drawString_P(2, 34, PSTR("exporting"));

  glcd.setFont(font_helvB14);  		//big bold font

  cval = cval + (consuming - cval) * 0.75; // was .5
  itoa((int)cval, str, 10);
  strcat(str, "w");
  glcd.drawString(3, 9, str);     		//ammount of power currently being used

  cval2 = cval2 + (gen - cval2) * 0.75;
  itoa((int)cval2, str, 10);
  strcat(str, "w");
  glcd.drawString(71, 9, str);    		//pv

  cval3 = cval3 + (grid - cval3) * 0.75;
  itoa((int)cval3, str, 10);
  strcat(str, "w");
  glcd.drawString(3, 42, str);    		//importing / exporting

  if (immersionOn) {
    cval4 = cval4 + (immersion - cval4) * 0.75;
    itoa((int)cval4, str, 10);
    strcat(str, "w");
    glcd.drawString(74, 42, str);
  } else {
    dtostrf(outsideTemp, 0, 1, str);
    strcat(str, "C");
    glcd.drawString(74, 42, str);
  }

  glcd.setFont(font_clR4x6);   		//small font - Kwh
  dtostrf((wh_consuming / 1000), 0, 1, str);
  strcat(str, "Kwh today");
  glcd.drawString(6, 26, str);

  dtostrf((wh_gen / 1000), 0, 1, str);
  strcat(str, "Kwh today");
  glcd.drawString(71, 26, str);



  if (immersionOn) {
    dtostrf((double)ImmerCtl.temperature / 100, 0, 1, str);
    strcat(str, "C");
    glcd.drawString(70, 58, str);
    dtostrf(outsideTemp, 0, 1, str);
    strcat(str, "C");
    glcd.drawString(105, 58, str);
  } else {
    glcd.drawLine(70, 59, 70, 63, WHITE);
    glcd.drawLine(68, 61, 70, 63, WHITE);
    glcd.drawLine(72, 61, 70, 63, WHITE);
    //        itoa((int)mintemp,str,10);
    dtostrf((double)mintemp, 0, 1, str);
    strcat(str, "C");
    //        glcd.drawString_P(68,58,PSTR("min"));
    glcd.drawString(74, 58, str);
    //                glcd.drawString(82,58,str);

    //	itoa((int)maxtemp,str,10);
    glcd.drawLine(100, 59, 100, 63, WHITE);
    glcd.drawLine(100, 59, 98, 61, WHITE);
    glcd.drawLine(100, 59, 102, 61, WHITE);
    dtostrf((double)maxtemp, 0, 1, str);
    strcat(str, "C");
    //	glcd.drawString_P(97,58,PSTR("max"));
    //	glcd.drawString(111,58,str);
    glcd.drawString(104, 58, str);
  }



  if ((millis() - last_emontx) > 10000) glcd.drawString_P(2, 58, PSTR("--Tx RF fail--"));
  else if ((millis() - last_emonbase) > 6000000) glcd.drawString_P(1, 58, PSTR("-Base RF fail--"));
  else {

    DateTime now = RTC.now();
    //  glcd.drawString_P(5,58,PSTR("Time:"));
    char str2[3];
    itoa((int)now.hour(), str, 10);
    if  (now.minute() < 10) strcat(str, ":0"); else strcat(str, ":");
    itoa((int)now.minute(), str2, 10);
    strcat(str, str2);
    glcd.drawString(5, 58, str);
    dtostrf(insideTemp, 0, 1, str);
    strcat(str, "C");
    glcd.drawString(40, 58, str);
  }

  glcd.refresh();

}

void draw_page_two()
{
  glcd.clear;
  glcd.fillRect(0, 0, 128, 64, 0);

  glcd.setFont(font_clR6x8);
  glcd.drawString_P(2, 0, PSTR("Current Time:"));
  glcd.drawString_P(2, 30, PSTR("Water Temperature:"));

  DateTime now = RTC.now();
  char str[20];
  char str2[5];
  itoa((int)now.hour(), str, 10);
  if  (now.minute() < 10) strcat(str, ":0"); else strcat(str, ":");
  itoa((int)now.minute(), str2, 10);
  strcat(str, str2);

  glcd.setFont(font_helvB14);  		//big bold font
  glcd.drawString(2, 10, str);

  dtostrf((double)ImmerCtl.temperature / 100, 0, 1, str);
  strcat(str, "C");
  glcd.drawString(2, 40, str);

  glcd.refresh();

  delay(2000);

}

void draw_page_three(int ldr)
{
  glcd.clear;
  glcd.fillRect(0, 0, 128, 64, 0);

  glcd.setFont(font_clR6x8);
  glcd.drawString_P(2, 0, PSTR("Demand:"));


  char str[20];

  itoa(ImmerCtl.SSRcontrol, str, 10);


  glcd.setFont(font_helvB14);  		//big bold font
  glcd.drawString(2, 10, str);

  glcd.setFont(font_clR6x8);
  glcd.drawString_P(0, 30, PSTR("Actual:"));

  dtostrf(immersion, 0, 0, str);
  strcat(str, "w");
  glcd.setFont(font_helvB14);  		//big bold font
  glcd.drawString(0, 42, str);

  glcd.refresh();

  delay(2000);

}

void backlight_control(int ldr)
{
  //--------------------------------------------------------------------
  // Turn off backlight and indicator LED's between 11pm and 7am
  //--------------------------------------------------------------------
  DateTime now = RTC.now();
  int hour = now.hour();                  //get hour digit in 24hr from software RTC

  if ((hour > 22) ||  (hour < 7)) {
    night = 1;
    glcd.backLight(0);
  } else {
    night = 0;
    glcd.backLight(ldr);
  }
}

//--------------------------------------------------------------------
//Change color of LED on top of emonGLCD, red if consumption exceeds gen, green if gen is greater than consumption
// or orange if immersion on.
//--------------------------------------------------------------------
void led_control()
{

  /* if (((gen>0) && (night==0))||(ImmerCtl.temperature < 4500 && hour >16 && hour <20 )) {*/
  if ((gen > 0) && (night == 0)) {
    if (immersion > 10) {            // show orange if immersion on
      ledstate = ORANGE;
      analogWrite(greenLED, greenledlevel);
      analogWrite(redLED, redledlevel);
    }
    else if (gen > consuming) {      //show green LED when gen>consumption
      ledstate = GREEN;
      analogWrite(greenLED, greenledlevel);
      analogWrite(redLED, 0);
    } else {      //red if consumption>gen
      ledstate = RED;
      analogWrite(redLED, redledlevel);
      analogWrite(greenLED, 0);
    }
  } else
  {
    ledstate = OFF;
    analogWrite(redLED, 0); //Led's off at night and when solar PV is not generating
    analogWrite(greenLED, 0);
  }
}

void led_intensity()
{
  /*   if (ImmerCtl.temperature < 4500 && hour >16 && hour <20 )
  	  {
      ledlevel =ledlevel+(direction*25);
      if (ledlevel > 200) {
        ledlevel = 200;
        direction = -1;
      }
      if (ledlevel < 25) {
    //     ledlevel = 25;
        direction = 1;
      }
    } else ledlevel=200;  */
  ledlevel = 200;

  switch (ledstate) {
    case ORANGE:
      greenledlevel = ledlevel;
      redledlevel = ledlevel;
      break;

    case GREEN:
      greenledlevel = ledlevel;
      redledlevel = 0;
      break;

    case RED:
      greenledlevel = 0;
      redledlevel = ledlevel;
      break;

    case OFF:
      greenledlevel = 0;
      redledlevel = 0;
      break;
  }
  analogWrite(greenLED, greenledlevel);
  analogWrite(redLED, redledlevel);


}

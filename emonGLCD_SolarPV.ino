//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Solar PV monitor example - compatiable with Arduino 1.0

// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd
// solar PV monitor build documentation: http://openenergymonitor.org/emon/applications/solarpv

// For use with emonTx setup with 2CT one monitoring consumption and the other monitoring gen
// RTC to reset Kwh counters at midnight is implemented is software.
// Correct time is updated via NanodeRF which gets time from internet

// Temperature recorded on the emonglcd is also sent to the NanodeRF for online graphing

// this sketch is currently setup for type 1 solar PV monitoring where CT's monitor generation and consumption separately
// The sketch assumes emonx.power1 is consuming/grid power and emontx.power2 is solarPV generation
// to use this sketch for type 2 solar PV monitoring where CT's monitor consumption and grid import/export using an AC-AC adapter to detect current flow direction
//    -change line 220-221- see comments in on specific lines. See Solar PV documentation for explination

// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// Authors: Glyn Hudson and Trystan Lea
// Part of the: openenergymonitor.org project
// Licenced under GNU GPL V3
// http://openenergymonitor.org/emon/license

//-------------------------------------------------------------------------------------------------------------------------------------------------
#define DEBUG 1

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)

//JeeLab libraires		       http://github.com/jcw
#include <JeeLib.h>		    // ports and RFM12 - used for RFM12B wireless
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC to reset kWh counters at midnight
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib

#include <GLCD_ST7565.h>            // Graphical LCD library 
#include <avr/pgmspace.h>           // Part of Arduino libraries - needed for GLCD lib

GLCD_ST7565 glcd;

#define ONE_WIRE_BUS 5              // temperature sensor connection - hard wired 
const int greenLED = 6;             // Green tri-color LED - dig 8 for emonGLCD V1.2
const int redLED = 9;               // Red tri-color LED
const int enterswitchpin = 15;		  // digital pin of onboard pushswitch
const int LDRpin = 4;    		        // analog pin of onboard lightsensor
const int upswitchpin = 16;         // digital pin of up switch - low when pressed
const int downswitchpin = 19;       // digital pin of down switch - low when pressed
//--------------------------------------------------------------------------------------------
// RFM12B Setup
//--------------------------------------------------------------------------------------------
#define MYNODE 20            //Should be unique on network, node ID 30 reserved for base station
#define freq RF12_868MHZ     //frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            //network group, must be same as emonTx and emonBase

const int Immersion_nodeID = 5;
const int emonTx_nodeID = 10;
const int emonBase_nodeID = 15;


//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
typedef struct {
  int power1, power2, power3, Vbat, Vrms;
} PayloadTX;       // assume that power1 is consuming/grid and power 2 is solar PV generation
// LA - power1 = gen , power2 = consuming
PayloadTX emontx;

//typedef struct { int temperature; } PayloadGLCD;
//PayloadGLCD emonglcd;

typedef struct {
  int hour, mins, sec, outsideTemperature;
  boolean newTime;
} PayloadBase;
PayloadBase emonbase;

typedef struct {
  int power1, temperature, Vrms, SSRcontrol;
} PayloadImm;
PayloadImm ImmerCtl;
//---------------------------------------------------

//--------------------------------------------------------------------------------------------
// Power variables
//--------------------------------------------------------------------------------------------
int importing, night;                                  //flag to indicate import/export
double consuming, gen, grid, wh_gen, wh_consuming, immersion;     //integer variables to store ammout of power currenty being consumed grid (in/out) +gen
unsigned long whtime;                    	       //used to calculate energy used per day (kWh/d)
boolean immersionOn = false;
//--------------------------------------------------------------------------------------------
// DS18B20 temperature setup - onboard sensor
//--------------------------------------------------------------------------------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double insideTemp, outsideTemp, maxtemp, mintemp;

//--------------------------------------------------------------------------------------------
// Software RTC setup
//--------------------------------------------------------------------------------------------
RTC_Millis RTC;
int hour;
int gmtOffset = 0;

//--------------------------------------------------------------------------------------------
// Flow control
//--------------------------------------------------------------------------------------------
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                   // Used to count time from last emontx update
unsigned long slow_update;                   // Used to count time for slow 10s events
unsigned long fast_update;                   // Used to count time for fast 100ms events

int LDR;

//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup () {
  rf12_initialize(MYNODE, freq, group);

  glcd.begin(0x18);    //begin glcd library and set contrast 0x20 is max, 0x18 seems to look best on emonGLCD
  glcd.backLight(150); //max 255

#ifdef DEBUG
  Serial.begin(230400);
  print_glcd_setup();
#endif

  pinMode(greenLED, OUTPUT); pinMode(redLED, OUTPUT);
  pinMode(enterswitchpin, INPUT); pinMode(upswitchpin, INPUT); pinMode(downswitchpin, INPUT);
  digitalWrite(enterswitchpin, HIGH); digitalWrite(upswitchpin, HIGH); digitalWrite(downswitchpin, HIGH); //enable Atmega328 10K internal pullup resistors

  sensors.begin();                         // start up the DS18B20 temp sensor onboard
  //   sensors.requestTemperatures();
  //   intemp = (sensors.getTempCByIndex(0));     // get inital temperture reading
  mintemp = 50; maxtemp = -50;          // reset min and max

  RTC.begin(DateTime("Dec  8 2011" , "12:00:00")); //start up software RTC - this time will be updated to correct time from the internet via the emonBase

}
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop () {

  //--------------------------------------------------------------------------------------------
  // 1. On RF recieve
  //--------------------------------------------------------------------------------------------
  if (rf12_recvDone()) {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);

      if (node_id == emonTx_nodeID)                        // === EMONTX node ID ====
      {
        last_emontx = millis();                 // set time of last update to now
        emontx = *(PayloadTX*) rf12_data;       // get emontx payload data
#ifdef DEBUG
        print_emontx_payload();               // print data to serial
        //           delay(100);                             // delay to make sure printing finished
#endif
        power_calculations();                   // do the power calculations
      }

      if (node_id == Immersion_nodeID)                        // === Immersion node ID ====
      {
        ImmerCtl = *(PayloadImm*) rf12_data;    // get emontx payload data
#ifdef DEBUG
        print_immersion_payload();
        //                     delay(100);                             // delay to make sure printing finished
#endif
      }

      if (node_id == emonBase_nodeID)                        // ==== EMONBASE node ID ====
      {
        last_emonbase = millis();
        emonbase = *(PayloadBase*) rf12_data;   // get emonbase payload data


#ifdef DEBUG
        print_emonbase_payload();             // print data to serial
#endif
        outsideTemp = emonbase.outsideTemperature / 10.0;
        if (outsideTemp > maxtemp) maxtemp = outsideTemp;
        if (outsideTemp < mintemp) mintemp = outsideTemp;

        if (emonbase.newTime == 1) {
          int hour = emonbase.hour + gmtOffset;
          if (hour == 24) hour = 0;
          RTC.adjust(DateTime(2012, 1, 1, hour, emonbase.mins, emonbase.sec));  // adjust emonglcd software real time clock
          delay(100);                                                           // delay to make sure printing and clock setting finished
        }





        //      emonglcd.temperature = (int) (insideTemp * 100);                          // set emonglcd payload
        //       int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}  // if ready to send + exit loop if it gets stuck as it seems too
        //       rf12_sendStart(0, &emonglcd, sizeof emonglcd);                      // send emonglcd data
        //       rf12_sendWait(0);
        //         Serial.println("3 emonglcd sent");                                // print status
        //      #endif
      }
    }
  }

  //    if (millis()<50000){   //reset min and max at the beginning of the sketch
  //      mintemp=50;
  //      maxtemp=-50;}

  //   if ((millis()>50000) && (millis() < 60000))
  //     mintemp=outsideTemp;

  //--------------------------------------------------------------------
  // Things to do every 10s
  //--------------------------------------------------------------------
  if ((millis() - slow_update) > 10000)
  {
    slow_update = millis();

    // Control led's
    LDR = analogRead(LDRpin);
    backlight_control(LDR / 3);
    led_control();

    // Get temperatue from onboard sensor
    sensors.requestTemperatures();
    insideTemp = (sensors.getTempCByIndex(0));
    //      if (temp > maxtemp) maxtemp = temp;
    //      if (temp < mintemp) mintemp = temp;
    if (immersion > 0) immersionOn = true; else immersionOn = false;

  }

  //--------------------------------------------------------------------
  // Update the display every 200ms
  //--------------------------------------------------------------------
  if ((millis() - fast_update) > 200)
  {
    fast_update = millis();
    draw_main_screen();
    led_intensity();
  }

  //Read switches
  int S1 = digitalRead(enterswitchpin); //low when pressed
  int S2 = digitalRead(upswitchpin);  //low when pressed
  int S3 = digitalRead(downswitchpin); //low when pressed

  if (S1 == 0) draw_page_two();       //if enter switch (top) is pressed display 2nd page
  if (S2 == 0) draw_page_three(LDR);
  if (S3 == 0) {
    DateTime now = RTC.now();
    if (gmtOffset != 0) {
      gmtOffset = 0;
      RTC.adjust(DateTime(2012, 1, 1, now.hour() - 1, now.minute(), now.second()));
    }  else {
      gmtOffset = 1;
      RTC.adjust(DateTime(2012, 1, 1, now.hour() + 1, now.minute(), now.second()));
    }
    draw_page_two();
  }  ;

} //end loop
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------
// Calculate power and energy variables
//--------------------------------------------------------------------
void power_calculations()
{
  DateTime now = RTC.now();
  int last_hour = hour;
  hour = now.hour();
  if (last_hour == 23 && hour == 00) {
    wh_gen = 0;
    wh_consuming = 0;
    mintemp = 50;
    maxtemp = -50;
  }

  gen = emontx.power1;  if (gen < 20) gen = 0;	// remove noise offset
  consuming = emontx.power2; 		        // for type 1 solar PV monitoring
  grid = consuming - gen;		        // for type 1 solar PV monitoring
  immersion = ImmerCtl.power1;  if (immersion < 11) immersion = 0;            // power going to immersion

  //grid=emontx.power1; 		         	// for type 2 solar PV monitoring
  // consuming=gen + emontx.power1; 	        // for type 2 solar PV monitoring - grid should be positive when importing and negastive when exporting. Flip round CT cable clap orientation if not

  if (gen > consuming) {
    importing = 0; 			      //set importing flag
    grid = grid * -1;			     //set grid to be positive - the text 'importing' will change to 'exporting' instead.
  } else importing = 1;

  //--------------------------------------------------
  // kWh calculation
  //--------------------------------------------------
  unsigned long lwhtime = whtime;
  whtime = millis();
  double whInc = gen * ((whtime - lwhtime) / 3600000.0);
  wh_gen = wh_gen + whInc;
  whInc = consuming * ((whtime - lwhtime) / 3600000.0);
  wh_consuming = wh_consuming + whInc;
  //----------------------------------------------------------------------
}

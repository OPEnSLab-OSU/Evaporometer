
// SD Card library //-------------------
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>

// RTC library //--------------------------
#include "RTClibExtended.h"
#include "LowPower.h"

// Load Cell library //---------------------
#include "HX711.h"

// Humidity/Temperature Sensor library //--------------------------
#include <Wire.h>
#include "Adafruit_SHT31.h"

//  Light Sensor library //--------------------------
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

// Debug mode for testing //
#define DEBUG 1

// Calibration Variable for 5 kg load cell //
#define calibration_factor -890

// Load Cell Variables //
#define DOUT 12
#define CLK 13

// global setup //
String IDstring, tempString, humidityString, loadCellString, lightIRString, lightFullString,  RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString, stringTransmit;;
const int ID = 100;
uint16_t lightIR, lightFull;

// set load cell pins //
HX711 scale(DOUT, CLK);

//declare/init transmitter flag for radio
volatile bool resFlag = false; //Flag is set to true once response is found

// Evap code from Manuel instance of temp/humidity sensor
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// light sensor instance //
// Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

// rtc instance //
RTC_DS3231 RTC;

// declare/init RTC variables//
volatile bool TakeSampleFlag = false; // Flag is set with external Pin A0 Interrupt by RTC
volatile bool LEDState = false; // flag t toggle LED
int volatile HR = 8; // Hr of the day we want alarm to go off
volatile int MIN = 0; // Min of each hour we want alarm to go off
volatile int WakePeriodMin = 5;  // Period of time to take sample in Min, reset alarm based on this period (Bo - 5 min)
const byte wakeUpPin = 11;

const int chipSelect = 9;
File myFile;


void setup() {

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
#if DEBUG == 1
  while (Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif
  // convert ID into String //
  IDstring = String(ID, DEC);
  pinMode(wakeUpPin, INPUT_PULLUP);

  //-----------------------------------------------------------------------
  // Load Cell Calibration //

  scale.set_scale(calibration_factor);
  scale.tare();
  scale.power_down();


  //--------------------------------------------------------------------
  // SD Card Setup Test //

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect))
  {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
#if DEBUG==1
  Serial.println("card initialized.");
#endif

  //----------------------------------------------------------------
  // Humidity Temp Sensor Setup Test //

#if DEBUG==1
  sht31.begin(0x44);

  while (!Serial){
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens
  }
  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
#endif
  sht31.begin(0x44);

  // ------------------------------------------------------------
  // Light Sensor Test ///
  /*
    // #if DEBUG==1
     // while (!tsl.begin())
     //   {
       #if DEBUG==1
         Serial.print("No Light Sensor Found");
       #endif
       while (1);
     }
      #if DEBUG==1
        Serial.print("Light Sensor found!");
      #endif
      configureSensor();
    #endif
  */
  // -------------------------------------------
  // RTC stuff init//

  InitalizeRTC();
#if DEBUG == 1
  Serial.print("Alarm set to go off every "); Serial.print(WakePeriodMin); Serial.println(" min from program time");
#endif
  delay(1000);
}

void loop() {

  if (!DEBUG)
  {
    // Enable SQW pin interrupt
    // enable interrupt for PCINT7...
    pciSetup(11);

    // Enter into Low Power mode here[RTC]:
    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    // <----  Wait in sleep here until pin interrupt
    // On Wakeup, proceed from here:
    PCICR = 0x00;         // Disable PCINT interrupt
    clearAlarmFunction(); // Clear RTC Alarm
    scale.power_up();
  }
  else
  {
    delay(30000); // period in DEBUG mode to wait between samples
    scale.power_up();
    TakeSampleFlag = 1;
  }
  if (TakeSampleFlag)
  {
    // read sensors //
    delay(100);
    float temp = sht31.readTemperature();
    delay(100);
    float humidity = sht31.readHumidity();
    delay(100);
    float loadCell = scale.get_units();

    // advancedRead();

    // power down load cell //
    scale.power_down();
    // tsl.disable();

    // convert sensor data into string for concatenation //
    tempString =  String(temp, 2); // 2 decimal places
    humidityString =  String(humidity, 2); // same
    loadCellString =  String(loadCell, 6); // 6 decimal places
    // lightIRString = String(lightIR, DEC);
    // lightFullString = String(lightFull, DEC);

    // prints temp, humidity, and load cell values to serial monitor in debug mode //
#if DEBUG==1
    if (! isnan(temp)) {  // check if 'is not a number'
      Serial.print("Temp *C = "); Serial.println(temp);
    } else {
      Serial.println("Failed to read temperature");
    }

    if (! isnan(humidity)) {  // check if 'is not a number'
      Serial.print("Hum. % = "); Serial.println(humidity);
    } else {
      Serial.println("Failed to read humidity");
    }
    if (! isnan(loadCell)) {  // check if 'is not a number'
      Serial.print("Load = "); Serial.println(loadCell);
    } else {
      Serial.println("Failed to read load cell");
    }
    /* if (! isnan(lightIR)) {  // check if 'is not a number'
      Serial.print("LightIR = "); Serial.println(lightIR);
      } else {
      Serial.println("Failed to read infared light");
      }
      if (! isnan(lightFull)) {  // check if 'is not a number'
      Serial.print("LightFull = "); Serial.println(lightFull);
      } else {
      Serial.println("Failed to read full light");
      }
      Serial.println();
    */
#endif

    // get RTC timestamp string
    DateTime now = RTC.now(); //get us the correct time
    uint8_t mo = now.month();
    uint8_t d = now.day();
    uint8_t h = now.hour();
    uint8_t mm = now.minute();

    RTC_monthString = String(mo, DEC);
    RTC_dayString = String(d, DEC);
    RTC_hrString = String(h, DEC);
    RTC_minString = String(mm, DEC);
    RTC_timeString = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString;
    Serial.println(RTC_timeString);

    // writes data values to sd card file //
    myFile = SD.open("Michael.txt", FILE_WRITE);
    if (myFile) {
      myFile.print(RTC_timeString);
      myFile.print(",");
      myFile.print(temp);
      myFile.print(",");
      myFile.print(humidity);
      myFile.print(",");
      myFile.println(loadCell);
      // myFile.print(",");
      // myFile.print(lightIR);
      // myFile.print(",");
      //  myFile.println(lightFull);

      // close the file:
      myFile.close();
#if DEBUG ==1
      Serial.println("done.");
#endif
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening test.txt");
    }
    delay(1000);

    // Stuff that NEEDS to happen at the end
    // Reset alarm1 for next period
    setAlarmFunction();
    delay(75);  // delay so serial stuff has time to print out all the way
    TakeSampleFlag = false; // Clear Sample Flag
    resFlag = false; //clear resFlag for next transmission time
  }
}


//******************
// TSL2591 Subroutines
//******************
/**************************************************************************
    Configures the gain and integration time for the TSL2591
**************************************************************************/
/* void configureSensor(void)
  {
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  //tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)
*/

/*
  #if DEBUG == 1
   Display the gain and integration time for reference sake
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         ");
  tsl2591Gain_t gain = tsl.getGain();
  switch (gain)
  {
    case TSL2591_GAIN_LOW:
      Serial.println("1x (Low)");
      break;
    case TSL2591_GAIN_MED:
      Serial.println("25x (Medium)");
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println("428x (High)");
      break;
    case TSL2591_GAIN_MAX:
      Serial.println("9876x (Max)");
      break;
  }
  Serial.print  ("Timing:       ");
  Serial.print((tsl.getTiming() + 1) * 100, DEC);
  Serial.println(" ms");
  Serial.println("------------------------------------");
  Serial.println("");
  #endif
  }
*/

//**************************************************************************/
//    Show how to read IR and Full Spectrum at once and convert to lux
//**************************************************************************/
/* void advancedRead(void)

  {
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  // uint32_t lum = tsl.getFullLuminosity();
  //uint16_t ir, full;
  lightIR = lum >> 16;
  lightFull = lum & 0xFFFF;
  //Serial.print("[ "); Serial.print(millis()); Serial.print(" ms ] ");
  //Serial.print("IR: "); Serial.print(ir);  Serial.print("  ");
  //Serial.print("Full: "); Serial.print(full); Serial.print("  ");
  // Serial.print("Visible: "); Serial.print(full - ir); Serial.print("  ");
  // Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));
  }
*/
//******************
// RTC Subroutines
//******************
void InitalizeRTC()
{
  // RTC Timer settings here
  if (! RTC.begin()) {
#if DEBUG == 1
    Serial.println("Couldn't find RTC");
#endif
    while (1);
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
  if (RTC.lostPower()) {
#if DEBUG == 1
    Serial.println("RTC lost power, lets set the time!");
#endif
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //clear any pending alarms
  clearAlarmFunction();

  // Querry Time and print
  DateTime now = RTC.now();
#if DEBUG == 1
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.print(now.second(), DEC); Serial.println();
#endif
  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setAlarmFunction();
}

// *********
// RTC helper function
// Function to query current RTC time and add the period to set next alarm cycle
// *********
void setAlarmFunction()
{
  DateTime now = RTC.now(); // Check the current time
  MIN = (now.minute() + WakePeriodMin) % 60; // wrap
  // Calculate new time
  // around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriodMin) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
  Serial.print("Resetting Alarm 1 for: "); Serial.print(HR); Serial.print(":"); Serial.println(MIN);
#endif

  //Set alarm1
  RTC.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC.alarmInterrupt(1, true);

}

//*********
// RTC helper function
// When exiting the sleep mode we clear the alarm
//*********
void clearAlarmFunction()
{
  //clear any pending alarms
  RTC.armAlarm(1, false);
  RTC.clearAlarm(1);
  RTC.alarmInterrupt(1, false);
  RTC.armAlarm(2, false);
  RTC.clearAlarm(2);
  RTC.alarmInterrupt(2, false);
}
//**********************
// Wakeup in SQW ISR
//********************
// Function to init PCI interrupt pin
// Pulled from: https://playground.arduino.cc/Main/PinChangeInterrupt

void pciSetup(byte pin)
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}


// Use one Routine to handle each group

ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
  if (digitalRead(11) == LOW)
    TakeSampleFlag = true;
}


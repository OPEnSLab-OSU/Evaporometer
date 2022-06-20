#define _dout A0 //MISO
#define _sclk A1 //SCK
#define _pdwn A2 //A2

#define sdPin 11      // Change to 11 if using HypnosV3.3
#define alarmPin 12   // Hypnos interrupt pin

#include "ADS1232.h"
#include "RTClib.h"
#include <SD.h>

ADS1232 weight = ADS1232(_pdwn, _sclk, _dout);
File myFile;
RTC_DS3231 rtc;

const float startingWeight = 800.00; // Known weight to divide by in grams
int unit = 0; // Units: {0 = grams, 1 = kilograms, 2 = ounces}
char buff[] = "Start time is hh:mm:ss DDD, DD MMM YYYY";

int packet = 0; // Packet number
int qty = 10; //Number of load cell readings to average over
float reading = 0; //weight values
float sum; // Collection of consecutive weight values
float avgWt; //average of weight values

void do_calibration() {
  long t_new_offset = 0;
  long t_raw_read = 0;
  float t_set_scale_value = 0;
  float t_weight = 0;

  // reset to default values
  weight.OFFSET = 0;
  weight.SCALE = 1.0;

/*
  // tare
  t_new_offset = weight.raw_read(100); // Takes 8 measurements and averages it
  weight.OFFSET = t_new_offset;
  Serial.print("Calibration offset = ");Serial.println(weight.OFFSET);
  Serial.println("You have 15 seconds to put a known weight on the scale");
  delay(15000);
  
  // do calibration based on a known weight
  t_raw_read = weight.raw_read(3);
  Serial.print("Units read = ");Serial.println(t_raw_read);
  t_set_scale_value = t_raw_read / startingWeight;  // divide it to a known weight
  weight.SCALE = t_set_scale_value;
  Serial.print("Calibration scale value = ");Serial.println(weight.SCALE); */


  // Force Calibrate
  weight.OFFSET = 8426198; // Set to "Calibration offset" from calibration (8527704)
  weight.SCALE = 214.89; // Set to "Calibration Scale value" from calibration (427.06) 

 
  // read weight
  t_weight = weight.units_read(3);
  Serial.print("Weight = ");Serial.println(t_weight, 5);

}

void print_units() {
    if (unit == 0)
    Serial.println(" g");
  else if (unit == 1)
    Serial.println(" kg");
  else if (unit == 2)
    Serial.println(" oz");
  else
    Serial.println("Unknown unit");
}

//https://github.com/JChristensen/DS3232RTC/blob/master/examples/TimeRTC/TimeRTC.ino
void printDigits(int digits)
{
    // utility function for digital clock display: prints leading 0
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void rtcSetup() {
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  rtc.disable32K();
  pinMode(alarmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(alarmPin), onAlarm, FALLING);

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);
  
  // turn off alarm 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  rtc.disableAlarm(2);
  
  DateTime now = rtc.now(); // Get current time

  // Print current time and date
  Serial.println(now.toString(buff));

}

void onAlarm() {
    Serial.println("Alarm occured!");
}


void sdSetup() {

  delay(500);
  
  // Checking if SD is active
  if (!SD.begin(sdPin)) {
    Serial.println("initialization failed!");
    return;
  }

  //Serial.println("SD initialized!");
  myFile = SD.open("data.csv", FILE_WRITE);
 
  // if the file opened okay, write to it:
  if (myFile) {
    
    myFile.print("Packet,Timestamp,Weight");
    myFile.println(",");
    
    // close the file:
    myFile.close();
    Serial.println("SD successfully logged.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("Error opening data.csv");
  }

}

void sdLog(DateTime now) {

  //DateTime now = rtc.now(); // Get current time
  myFile = SD.open("data.csv", FILE_WRITE);

  // Writes sensor values
  if (myFile) {

    // Packet number
    myFile.print(packet); 
    myFile.print(",");

    // Timestamp
    myFile.print("Date: ");
    myFile.print(now.year(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.day(), DEC);
    myFile.print(" Time: ");
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.print(now.minute(), DEC);
    //printDigits(now.minute());
    myFile.print(':');
    myFile.print(now.second(), DEC);
    //printDigits(now.second());
    myFile.print(",");

    // Weight
    myFile.println(avgWt, 5); // Weight
    
    myFile.close();

    Serial.println("Data successfully logged");
  }
  else {
      // if the file didn't open, print an error
      Serial.println("Error opening data.csv");
  }
}

void setup() {
  
  Serial.begin(9600);
  //while(!Serial);

  // Initializing and setting pinouts
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  pinMode(sdPin, OUTPUT);

  // Setting up scale and calibrating it
  weight.power_up();
  do_calibration();

  // Setting up RTC and SD
  rtcSetup();
  sdSetup();

  // schedule an alarm 10 seconds in the future
  if(!rtc.setAlarm1(
          rtc.now() + TimeSpan(10),
          DS3231_A1_Minute // this mode triggers the alarm when the seconds match. See Doxygen for other options
  )) {
      Serial.println("Error, alarm wasn't set!");
  }else {
      Serial.println("Alarm will happen in 10 seconds!");  
  }
}

void loop() {
  if(rtc.alarmFired(1)){
    rtc.clearAlarm(1);
    
    packet++;
    DateTime now = rtc.now(); // Get current time
    // Get raw and print each ADC value
  
    // Print data on serial monitor
    // Packet Number
    //Serial.println("Packet: " + String(packet));
  
    // Timestamp
    Serial.print("Date: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" Time: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    //Serial.print(now.minute(), DEC);
    printDigits(now.minute());
    Serial.print(':');
    //Serial.print(now.second(), DEC);
    printDigits(now.second());
    Serial.println();
  
    // Weight
      sum = 0;
      for (int i = 0; i < qty; ++i) {
          reading = weight.units_read(1);
          //Serial.print("Scaled load reading ");
          //Serial.print(i);
          //Serial.print(", ");
          //Serial.println(reading,5);
          sum += reading; 
      }
      Serial.print("Averaged weight: ");
      avgWt = sum/qty;
      Serial.println(avgWt,5);
  
    // Log data to SD
    sdLog(now);
    
    // schedule an alarm 60 seconds in the future
    if(!rtc.setAlarm1(
            rtc.now() + TimeSpan(10),
            DS3231_A1_Minute // this mode triggers the alarm when the seconds match. See Doxygen for other options
    )) {
        Serial.println("Error, alarm wasn't set!");
    }else {
        Serial.println("Alarm will happen in 10 seconds!");  
    }
  }
}

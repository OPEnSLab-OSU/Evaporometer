/*
 *
 * Program for Windscreen Validation
 *
 * Needs to take measurments as fast as possible over 1 minute
 * average them out and save to SD card
 * Author: Evan Hockert
 */

 /*Note this program uses the millis() function which is NOT meant for use for longer than 50 days
  * NOT FOR LONG TERM DEPLOYMENT
  */
//Loom manager must be included first
#include <Loom_Manager.h>

//Including Hypnos (Loom), SD Manager (Loom), and ADS1232 (not Loom) libraries
#include <Hardware/Loom_Hypnos/Loom_Hypnos.h>
#include <Hardware/Loom_Hypnos/SDManager.h>
#include <Sensors/I2C/Loom_SHT31/Loom_SHT31.h>
#include <ADS1232_Lib.h>


/*
 * RELATED TO APRIL 2024 WINDSCREEN TESTING:
 * For windscreen unit:
 * pdwn, sclk, dout:  A2, A0, A1
 * For regular unit:
 * pdwn, sclk, dout: A2, A1, A0
 */

//Define ADS1232 PINS
#define pdwn A2
#define sclk A1
#define dout A0

Manager manager("Device", 1);

ADS1232_Lib ads(pdwn, sclk, dout);
Loom_SHT31  sht(manager);

/* Note the hypnos and sd manager are different
 * objects because using the hypnos logging functionaliy
 * requires the use of manager.measure whi`ch appears broken
 * for the ADS1232. Thus this script uses the ADS12303 library
 * to take measurements directly.
 */
Loom_Hypnos hypnos(manager, HYPNOS_VERSION::V3_3, TIME_ZONE::PST, true, false);
SDManager sd(&manager, 11);

void setup() {

  manager.beginSerial();
  hypnos.enable();

  manager.initialize();

  //set the SD card output pins (we are NOT using the hypnos log functionality)
  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT);
  pinMode(11, OUTPUT);
  sd.begin();

  //These are the calibration parameters for the load cell
  ads.set_offset(0);
  ads.set_scale(1);
  //not sure if this line is neccesary
  ads.power_up();

}


long measure_over_minute(){
  float measure;

  long sum = 0;
  
  int start_time = millis();
  int end_time = start_time + 60000;
  int num_measurments = 0;

  while(millis() < end_time){
    //take measurment

    measure = ads.units_read(100); //NOTE: possibly increase the number measurements taken every loop
    
    //average measurment
    sum = sum + measure;
    
    //increment counter
    num_measurments++;

    Serial.println("-Sum-#of measures-");
    Serial.println(sum);
    Serial.println(num_measurments);

  }

  Serial.println("Data reported: ");
  Serial.println(sum/num_measurments);

  return sum/num_measurments;

}

long measure_temp(){
  manager.measure();
  return sht.getTemperature();
}

void loop() {
  
  //measure for 1 minute
  long raw = measure_over_minute();
  
  char raw_str[16] = {};
  ltoa(raw, raw_str,10);

  //Get the current time from hypnos and save it as a c str
  char buf1[20] = {};
  DateTime now = hypnos.getCurrentTime();
  sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());

  long temp = measure_temp();

  char buf2[20] = {};
  sprintf(buf2, "%ld", temp);

  //Combine the time and measurments strings. Include "," to separate columns
  char date_and_data[64] = {};
  strcat(date_and_data, buf1);
  strcat(date_and_data, ",");
  strcat(date_and_data, raw_str);
  strcat(date_and_data,",");
  strcat(date_and_data, buf2);

  Serial.println(raw_str);

  //Write measurment and time to csv
  sd.writeLineToFile("Device0.csv", date_and_data);
}

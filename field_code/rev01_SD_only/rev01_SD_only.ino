/*
 * Title: Evaporometer SD Card Logging Only 
 *
 * Description: Uses ADS1232 to measure voltage 
 * across a load cell. Uses on board adc to measure votlage
 * across a thermistor
 *
 * Date: Feb 2, 2025
 */

//Loom manager must be included first
#include <Loom_Manager.h>
#include <ADS1232_Lib.h>
#include <Hardware/Loom_Hypnos/Loom_Hypnos.h>
#include <Hardware/Loom_Hypnos/SDManager.h>
#include <algorithm>


#define PDWN A5
#define SCLK A4
#define DOUT A3
#define VBATPIN A7

#define THRM A1

#define OFFSET 9338884
#define SCALE 1992.055

Manager     manager("Device", 1);
ADS1232_Lib ads(PDWN, SCLK, DOUT);
Loom_Hypnos hypnos(manager, HYPNOS_VERSION::V3_3, TIME_ZONE::PST, true, false);
SDManager   sd(&manager, 11);


/*
 *    Function: log_date
 *    Description: Logs weight and temp to SD card
 *    Logs by default to "Device0.csv"
 *
 */
void log_data(float weight, long temp, float vbat){
  //Get the current time from hypnos and save it as a c str
  char buf1[32] = {};
  DateTime now = hypnos.getCurrentTime();
  
  sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  
          now.hour(), now.minute(), now.second(),
          now.day(), now.month(), now.year());
  
  char buf2[32] = {};
  sprintf(buf2, "%.2f", weight);

  char buf3[32] = {};
  sprintf(buf3, "%ld", temp);

  char buf4[32] = {};
  sprintf(buf4, "%f", vbat);

  //Combine the time and measurments strings. Include "," to separate columns
  char date_and_data[128] = {};
  strcat(date_and_data, buf1);
  strcat(date_and_data, ",");
  strcat(date_and_data, buf2);
  strcat(date_and_data,",");
  strcat(date_and_data, buf3);
  strcat(date_and_data, ",");
  strcat(date_and_data, buf4);

  sd.writeLineToFile("Device0.csv", date_and_data);
}

float get_weight() {
  float weights[3] = {};
  int n = sizeof(weights)/sizeof(weights[0]);

  for(int i = 0; i < 3; i++) {
    weights[i] = ads.units_read(10);
  }

  std::sort(weights, weights + n);

  return weights[1];
}

float get_vbat(){
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat);
  return measuredvbat;
}


void isr_Trigger(){
  hypnos.wakeup();
}

void setup() {
  manager.beginSerial();
  Serial.println("HERE");
  hypnos.enable();
  manager.initialize();

  hypnos.registerInterrupt(isr_Trigger);
  // set SD card write pins
  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT);
  pinMode(11, OUTPUT);
  sd.begin();
  
  // set thermistor reading pin A1
  pinMode(THRM, INPUT);

  ads.set_offset(OFFSET);
  ads.set_scale(SCALE);
  Serial.println("End of Setup");
}

void loop() {
  // Startup
  // get the weight
  Serial.println("Taking weight");
  ads.power_up();
  manager.pause(1000);
  float weight = get_weight();
  Serial.println(weight);
  // get the temp
  Serial.println("Taking temp");
  long temp = analogRead(THRM);
  Serial.println(temp);
  // get the battery voltage
  float vbat = get_vbat();
  
  //log data
  Serial.println("Logging Data");
  log_data(weight, temp, vbat);

  Serial.println(weight);

  hypnos.setInterruptDuration(TimeSpan(0, 0, 0, 20));
  hypnos.reattachRTCInterrupt();
  hypnos.sleep();
}

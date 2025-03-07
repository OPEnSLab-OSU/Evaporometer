/*
 * Title: Evaporometer Calibration Readings 
 *
 * Description: Uses ADS1232 to constantly print readings 
 * to serial monitor. Used for manual calibration
 *
 * Date: Mar 7, 2025
 */

//Loom manager must be included first
#include <Loom_Manager.h>
#include <ADS1232_Lib.h>
#include <Hardware/Loom_Hypnos/Loom_Hypnos.h>
#include <Hardware/Loom_Hypnos/SDManager.h>

#define PDWN A5
#define SCLK A4
#define DOUT A3
#define VBATPIN A7

#define THRM A1

Manager     manager("Device", 1);
ADS1232_Lib ads(PDWN, SCLK, DOUT);
Loom_Hypnos hypnos(manager, HYPNOS_VERSION::V3_3, TIME_ZONE::PST, true, false);

void setup() {
  manager.beginSerial();
  Serial.println("Starting Setup");
  manager.initialize();
  hypnos.enable();
  
  // set thermistor reading pin A1
  pinMode(THRM, INPUT);

  Serial.println("End of Setup");
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Taking Weight");
  ads.power_up();
  manager.pause(500);
  long weight_counts = ads.raw_read(50);
  Serial.println(weight_counts);
  
  Serial.println("Taking Temp");
  int temp = analogRead(THRM);
  Serial.println(temp);
}

#include <Loom.h>
#include <ADS1232.h>

const char* json_config =
#include "config.h"
;

using namespace Loom;
Loom::Manager Feather{};

////////////////////////// ADS1232 Specific Stuff ////////////////////////////////////
#define _dout A0 //MISO
#define _sclk A1 //SCK
#define _pdwn A2 //A2

ADS1232 weight = ADS1232(_pdwn, _sclk, _dout);

const float startingWeight = 800.00; // Known weight to divide by in grams
int unit = 0; // Units: {0 = grams, 1 = kilograms, 2 = ounces}

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
  weight.OFFSET = 8403613; // Set to "Calibration offset" from calibration (8527704)
  weight.SCALE = 2041.46; // Set to "Calibration Scale value" from calibration (427.06) 

 
  // read weight
  //t_weight = weight.units_read(3);
}

void ADS1232_measure() {
      // Weight
      sum = 0;
      for (int i = 0; i < qty; ++i) {
          reading = weight.units_read(3);
          //Serial.print("Scaled load reading ");
          //Serial.print(i);
          //Serial.print(", ");
          //Serial.println(reading,5);
          sum += reading; 
      }
      avgWt = sum/qty;
}

void ADS1232_package() {
    // Adds measurements to Loom JSON
    JsonArray contents = Feather.internal_json()["contents"];
    JsonObject obj = contents.createNestedObject();
    obj["module"] = "Weight";
    JsonObject data = obj.createNestedObject("data");
    data["g"] = avgWt;
}
////////////////////////// End ADS1232 Specific Stuff ////////////////////////////////

void wakeISR_RTC() {
  detachInterrupt(12);
}

void setup() {
  pinMode(5, OUTPUT);    // Enable control of 3.3V rail
  pinMode(6, OUTPUT);   // Enable control of 5V rail

  digitalWrite(5, LOW); // Enable 3.3V rail
  digitalWrite(6, HIGH);  // Enable 5V rail
  
  Feather.begin_serial(true);
  Feather.parse_config(json_config);
  Feather.print_config();

  getInterruptManager(Feather).register_ISR(12, wakeISR_RTC, LOW, ISR_Type::IMMEDIATE);

  // Setting up scale and calibrating it
  weight.power_up();
  do_calibration();

  LPrintln("\n ** Setup Complete ** ");
}

void loop() {
  Feather.measure();
  Feather.package();

  // Dirty integrate the ADS1232
  ADS1232_measure();
  ADS1232_package();

  Feather.display_data();

  getSD(Feather).log();

  // set the RTC alarm to a duration of 3 seconds with TimeSpan
  getInterruptManager(Feather).RTC_alarm_duration(TimeSpan(0,0,0,30));
  getInterruptManager(Feather).reconnect_interrupt(12);

  digitalWrite(5, HIGH); // Disable 3.3V rail
  digitalWrite(6, LOW);  // Disable 5V rail

  // Disable SD pins
  pinMode(23, INPUT);
  pinMode(24, INPUT);
  pinMode(11, INPUT);

  Feather.power_down();
  getSleepManager(Feather).sleep();
  // Execution is paused here until interrupt is triggered by RTC

  digitalWrite(5, LOW); // Enable 3.3V rail
  digitalWrite(6, HIGH);  // Enable 5V rail

  // Re-enable SD pins
  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT);
  pinMode(11, OUTPUT);

  Feather.power_up();
  delay(1000); // Some sort of delay needed after powerup
}

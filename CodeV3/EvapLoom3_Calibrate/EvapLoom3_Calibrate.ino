//#include <Loom.h>
#include "ADS1232.h"

////////////////////////// ADS1232 Specific Stuff ////////////////////////////////////
#define _dout A0 //MISO
#define _sclk A1 //SCK
#define _pdwn A2 //A2

ADS1232 weight = ADS1232(_pdwn, _sclk, _dout);

const float startingWeight = 800.00; // Known weight to divide by in grams
int unit = 0; // Units: {0 = grams, 1 = kilograms, 2 = ounces}

int qty = 8; //Number of load cell readings to average over
float reading = 0; //weight values
float sum; // Collection of consecutive weight values
float avgWt; //average of weight values

void do_calibration() {
  long t_new_offset = 0;
  long t_raw_read = 0;
  float t_set_scale_value = 0;
  float t_weight = 0;
  
  Serial.println("Resetting weight params to default ");
  // reset to default values
  weight.OFFSET = 0;
  weight.SCALE = 1.0;
// Start Calibration Comment

  // tare
  Serial.println("Performing Tare, Calibration offset");
  t_new_offset = weight.raw_read(100); // Takes 64 measurements and averages it
  weight.OFFSET = t_new_offset;
  Serial.print("Calibration offset = ");Serial.println(weight.OFFSET);
  Serial.println("You have 10 seconds to put a known weight on the scale");
  delay(10000);
  
  // do calibration based on a known weight
  t_raw_read = weight.raw_read(3);
  Serial.print("Units read = ");Serial.println(t_raw_read);
  t_set_scale_value = t_raw_read / startingWeight;  // divide it to a known weight
  weight.SCALE = t_set_scale_value;
  Serial.print("Calibration scale value = ");Serial.println(weight.SCALE); 

 // End Calibration Comment
  
  

 
  // read weight
  //t_weight = weight.units_read(3);
}


////////////////////////// End ADS1232 Specific Stuff ////////////////////////////////
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

void setup() {
  pinMode(5, OUTPUT);    // Enable control of 3.3V rail
  pinMode(6, OUTPUT);   // Enable control of 5V rail

  digitalWrite(5, LOW); // Enable 3.3V rail
  digitalWrite(6, HIGH);  // Enable 5V rail
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("** User Connected, Powering up cell... ** ");
  // Setting up scale and calibrating it
  weight.power_up();
  Serial.println("Powered up, Calibrating... ** ");
  do_calibration();

  Serial.println("\n ** Calibration Complete ** ");
}

void loop() {
  /*
  // Report weight values with new calibration
  int load_read = weight.units_read(3);
  Serial.print("New Weight Read: ");Serial.println(load_read);
  */
  ADS1232_measure();  // This can take up to 4 seconds
  Serial.print("New Weight Read: ");Serial.println(avgWt);

  delay(5000);
}

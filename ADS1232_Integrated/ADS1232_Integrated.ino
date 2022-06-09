#include <Loom.h>

const char* json_config =
#include "config.h"
;

using namespace Loom;
Loom::Manager Feather{};

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

  LPrintln("\n ** Setup Complete ** ");
}

void loop() {
  Feather.measure();
  Feather.package();

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

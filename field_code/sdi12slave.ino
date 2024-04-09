/*
 * SDI-12 Evaporometer
 *
 * MANAGER MUST BE INCLUDED FIRST IN ALL CODE
 */

/*
 * This code is mean to run on a SAMD21 Feather m0
 * It interfaces between a spring gauge load cell + sht31 and an SDI-12 datalogger
 * The code uses the Loom library to interface with the sensors
 * Essentially it waits for an SDI-12 command comming into pin 10 (by default)
 * then it executes the command
 */

/*
 * Adapted from h_sdi_12_slave_implementation example from SDI-12 library
 * https://www.arduino.cc/reference/en/libraries/sdi-12/
 */



#include <Loom_Manager.h>

#include <Sensors/Loom_Analog/Loom_Analog.h>
#include <Sensors/SPI/Loom_ADS1232/Loom_ADS1232.h>
#include <Hardware/Loom_Hypnos/Loom_Hypnos.h>
#include <Sensors/I2C/Loom_SHT31/Loom_SHT31.h>

#include <SDI12.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

Manager manager("Device", 1);

// Set ADS1232 reading variables
#define PACKET 100
#define OFFSET 11071400.8
#define SCALE 2221.0

#define DATA_PIN 10   /*!< The pin of the SDI-12 data bus */
#define POWER_PIN -1 /*!< The sensor power pin (or -1 if not switching power) */

char sensorAddress = '5';
int  state         = 0;

//used in SDI-12 contorl flow
#define WAIT 0
#define INITIATE_CONCURRENT 1
#define INITIATE_MEASUREMENT   2

// Create object by which to communicate with the SDI-12 bus on SDIPIN
SDI12 slaveSDI12(DATA_PIN);

Loom_Hypnos hypnos(manager, HYPNOS_VERSION::V3_3, TIME_ZONE::PST,false, false);
Loom_ADS1232 ads(manager, PACKET, OFFSET, SCALE);
Loom_Analog analog(manager);

//Loom_SHT31 sht(manager);

void pollSensor(float* measurementValues) {

  manager.measure();

  measurementValues[0] = ads.getWeight();
  //measurementValues[1] = sht.getTemperature();
}

void parseSdi12Cmd(String command, String* dValues, float* measurementValues) {

  char garb_chars[5] = {'@',' ', '`', '|','~'};
  int garb_chars_length = sizeof(garb_chars) / sizeof(garb_chars[0]);
  //if the first character is in an array of common garbage characters then remove it
  for (int i = 0; i < garb_chars_length; i++){
    if (command.charAt(0) == garb_chars[i]){
      command.remove(0, 1);
    }
  }

  //if the first character is not the address or a question mark
  if (command.charAt(0) != sensorAddress && command.charAt(0) != '?') {
    Serial.print("wrong addr: ");
    Serial.println(command);
  return;
  }

  Serial.println("COMMAND RECIEVED: " + command);

  String responseStr = "";
  if (command.length() > 1) {
    switch (command.charAt(1)) {
      case '?':

        responseStr = "5";
        break;
      case 'I':
        // Identify command
        // Slave should respond with ID message: 2-char SDI-12 version + 8-char
        // company name + 6-char sensor model + 3-char sensor version + 0-13 char S/N
        responseStr = "14EVAPRMTR0000011.0001";  // Substitute proper ID String here
        break;
      case 'M':
        // Initiate measurement command
        // Slave should immediately respond with: "tttnn":
        //    3-digit (seconds until measurement is available) +
        //    1-digit (number of values that will be available)
        // Slave should also start a measurment but may keep control of the data line
        // until advertised time elapsed OR measurement is complete and service request
        // sent
        responseStr =
          "0112";  
        state = INITIATE_MEASUREMENT;
        break;
        // NOTE: "aM1...9!" commands may be added by duplicating this case and adding
        //       additional states to the state flag

      case 'D':
        // Send data command
        // Slave should respond with a String of values
        // Values to be returned must be split into Strings of 35 characters or fewer
        // (75 or fewer for concurrent).  The number following "D" in the SDI-12 command
        // specifies which String to send
        responseStr = dValues[(int)command.charAt(2) - 48];
        break;
      case 'A':
        // Change address command
        // Slave should respond with blank message (just the [new] address + <CR> +
        // <LF>)
        sensorAddress = command.charAt(2);
        break;
      default:
        // Mostly for debugging; send back UNKN if unexpected command received
        responseStr = "UNKN";
        break;
    }
  }

  if (command.charAt(0) == '?'){
    responseStr = "";
  }

  // Issue the response speficied in the switch-case structure above.
  Serial.println(String(sensorAddress) + responseStr + "\r\n");
  //SslaveSDI12.forceHold();
  slaveSDI12.sendResponse(String(sensorAddress) + responseStr + "\r\n");
  slaveSDI12.forceListen();
}


void formatOutputSDI(float* measurementValues, String* dValues, unsigned int maxChar) {
  /* Ingests an array of floats and produces Strings in SDI-12 output format */

  dValues[0] = "";
  int j      = 0;

  // upper limit on i should be number of elements in measurementValues
  for (int i = 0; i < 2; i++) {
    // Read float value "i" as a String with 6 deceimal digits
    // (NOTE: SDI-12 specifies max of 7 digits per value; we can only use 6
    //  decimal place precision if integer part is one digit)
    String valStr = String(measurementValues[i], 6);
    // Explictly add implied + sign if non-negative
    if (valStr.charAt(0) != '-') { valStr = '+' + valStr; }
    // Append dValues[j] if it will not exceed 35 (aM!) or 75 (aC!) characters
    if (dValues[j].length() + valStr.length() < maxChar) {
      dValues[j] += valStr;
    }
    // Start a new dValues "line" if appending would exceed 35/75 characters
    else {
      dValues[++j] = valStr;
    }
  }

  // Fill rest of dValues with blank strings
  while (j < 9) { dValues[++j] = ""; }
}

void setup() {

  Serial.begin(115200);
  manager.beginSerial(false);

  // Enable the hypnos rails
  hypnos.enable();

  // Initialize the manager
  manager.initialize();

  slaveSDI12.begin();
  delay(500);
  slaveSDI12.forceListen();  // sets SDIPIN as input to prepare for incoming message
  

}

void loop() {

  static float measurementValues[9];  // 9 floats to hold simulated sensor data
  static String dValues[10];  // 10 String objects to hold the responses to aD0!-aD9! commands
  static String commandReceived = "";  // String object to hold the incoming command
  
  // If a byte is available, an SDI message is queued up. Read in the entire message
  // before proceding.  It may be more robust to add a single character per loop()
  // iteration to a static char buffer; however, the SDI-12 spec requires a precise
  // response time, and this method is invariant to the remaining loop() contents.
  int avail = slaveSDI12.available();
  if (avail < 0) {
    Serial.println("buffer was full");
    slaveSDI12.clearBuffer();
  }  // Buffer is full; clear
  
   if (avail > 0) {
    for (int a = 0; a < avail; a++) {
      char charReceived = slaveSDI12.read();
      // Character '!' indicates the end of an SDI-12 command; if the current
      // character is '!', stop listening and respond to the command
      if (charReceived == '!') {
        // Command string is completed; do something with it
        parseSdi12Cmd(commandReceived, dValues, measurementValues);
        // Clear command string to reset for next command
        commandReceived = "";
        // '!' should be the last available character anyway, but exit the "for" loop
        // just in case there are any stray characters
        slaveSDI12.clearBuffer();
        // eliminate the chance of getting anything else after the '!'
        //slaveSDI12.forceHold();
        break;
      }
      // If the current character is anything but '!', it is part of the command
      // string.  Append the commandReceived String object.
      else {
        // Append command string with new character
        commandReceived += String(charReceived);
      }
    }
  }

  switch (state) {
    case WAIT: break;
    case INITIATE_MEASUREMENT:

      pollSensor(measurementValues);
      // Populate the "dValues" String array with the values in SDI-12 format
      formatOutputSDI(measurementValues, dValues, 35);
      // For aM!, Send "service request" (<address><CR><LF>) when data is ready
      //slaveSDI12.sendResponse(String(sensorAddress) + "\r\n");
      state = WAIT;
      slaveSDI12.clearBuffer();
      slaveSDI12.forceListen();  // sets SDI-12 pin as input to prepare for incoming
                                 // message AGAIN
      break;
  }
}  

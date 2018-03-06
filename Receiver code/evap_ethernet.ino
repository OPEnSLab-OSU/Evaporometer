# Current working code as of 03/06/18#
#changes: transmitting RSSI Values from Radio Handles OSU Network more effectively#
#include <SPI.h>
#include <Ethernet2.h>
#include <RH_RF95.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const byte mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x9e, 0x98 };
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 56, 166);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

unsigned long lastConnectionTime = 0;             // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 1000L; // delay between updates, in milliseconds
// the "L" is needed to use long type numbers

//Your secret DevID from PushingBox.com. You can use multiple DevID  on multiple Pin if you want
const char DEVID1[] = "vB550C997E3F64E4";       //Scenario : "The mailbox is open"
uint8_t pinDevid1 = 3;

const char pushGetHeader[] = "GET /pushingbox?devid=";
char serverName[] = "api.pushingbox.com";
boolean pinDevid1State = false;                // Save the last state of the Pin for DEVID1
boolean lastConnected = false;                 // State of the connection last time through the main loop

#if defined(ESP8266)
  // default for ESPressif
  #define WIZ_CS 15
#elif defined(ESP32)
  #define WIZ_CS 33
#elif defined(ARDUINO_STM32_FEATHER)
  // default for WICED
  #define WIZ_CS PB4
#elif defined(TEENSYDUINO)
  #define WIZ_CS 10
#elif defined(ARDUINO_FEATHER52)
  #define WIZ_CS 11
#else   // default for 328p, 32u4 and m0
  #define WIZ_CS 10
#endif


//Set-up
/* for feather32u4 */
const short RFM95_CS = 8;
const short RFM95_RST = 4;
const short RFM95_INT = 7;

// Change to 434.0 or other frequency, must match RX's freq!
const int RF95_FREQ = 915.0;
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// place to save parsed received data
char *array[8]; // should be size of how many expected comma delimited elements in the buffer

void setup() {
  // LoRa Reset
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);
  
#if defined(WIZ_RESET)
  pinMode(WIZ_RESET, OUTPUT);
  digitalWrite(WIZ_RESET, HIGH);
  delay(100);
  digitalWrite(WIZ_RESET, LOW);
  delay(100);
  digitalWrite(WIZ_RESET, HIGH);
#endif
/*
#if !defined(ESP8266) 
  while (!Serial); // wait for serial port to connect.
#endif
*/
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  delay(1000);
    
  Serial.println("\nEthernet FeatherWing Initializing");
  Ethernet.init(WIZ_CS);
  
  // give the ethernet module time to boot up:
  delay(1000);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  
  // print the Ethernet board/shield's IP address:
  Serial.print("\nEthernet Initialized");
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  Serial.println("Feather LoRa Rec. Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
    rf95.setFrequency(RF95_FREQ);

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);
}

void loop() 
{
  if (rf95.available()) 
  {
      // Should be a message for us now   
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
    
      if (rf95.recv(buf, &len)) {
        RH_RF95::printBuffer("Received: ", buf, len);
        Serial.print("Got: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        delay(10);

        // Parse comma delimited data
        // sourced from: https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
        char *pt;
        short i = 0; 
        pt = strtok ((char*)buf,",");
        while (pt != NULL) {
          array[i++] = pt; // Save data into pointer array, defined in global vars
          pt = strtok (NULL, ",");
        }
     
        Serial.println("now the parse");
         // Now access them by index
        for (i = 0; i < 8; ++i) {
          Serial.println(array[i]);
        }

        // Send a reply
        rf95.send(array[0], sizeof(array[0])+1); // Send Device ID back to transmitter in reply
        rf95.waitPacketSent();
        Serial.println("Sent a reply");
        Serial.println(array[0]);

        sendToPushingBox(DEVID1);
      }
      else 
      {
        Serial.println("Receive failed");
      }
  }
  else
  {
    //Loop until there is a message available
    Serial.println("Waiting for transmission...\n");
    //delay(10);
  }
  //sendToPushingBox(DEVID1);
}

void sendToPushingBox(char devid[])
{
  client.stop();
  Serial.println("connecting...");
  if (client.connect(serverName, 80)) {
    Serial.println("connected");
    Serial.println("sending request");
      
    client.print("GET /pushingbox?devid="); client.print(devid); 
    client.print("&IDtag=");client.print(array[0]);
    client.print("&TimeStamp=");client.print(array[1]);
    client.print("&TempC=");client.print(array[2]);
    client.print("&Humid=");client.print(array[3]);
    client.print("&LoadCell=");client.print(array[4]);
    client.print("&IRLight=");client.print(array[5]);
    client.print("&FullLight=");client.print(array[6]);
    client.print("&BatVolt=");client.print(array[7]);
    client.print("&RSSI=");client.print(rf95.lastRssi());
    client.println(" HTTP/1.1");
    client.print("Host: "); client.println(serverName);
    client.println("User-Agent: Arduino");
    client.println();

    Serial.println("Request Sent!");
  } 
  else {
    Serial.println("connection failed");
  }
}

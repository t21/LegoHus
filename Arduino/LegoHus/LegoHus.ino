/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board:
  ----> https://www.adafruit.com/product/2471

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiUdp.h>
#include <TimeLib.h> 

/****************************** Pins ******************************************/

const uint8_t BLUE_LED = D0;  // GPIO16, Onboard blue LED
const uint8_t LED1 = D1;  // GPIO5
const uint8_t LED2 = D2;  // GPIO4
//const uint8_t LED3 = D3;  // GPIO0, pulled up to 3V3, used for flashing (also FLASH button)
//const uint8_t LED4 = D4;  // GPIO2, pulled up to 3V3, used for flashing
const uint8_t LED3 = D5;  // GPIO14
const uint8_t LED4 = D6;  // GPIO12
const uint8_t LED5 = D7;  // GPIO13
//const uint8_t LED5 = D8;  // GPIO15, pulled low, used for flashing
//const uint8_t LED5 = D9;  // RXD0, used for flashing
//const uint8_t LED5 = D10; // TXD0, used for flashing
const uint8_t LED6 = 9; // GPIO9, used for module SPI flash?
const uint8_t LED7 = 10; // GPIO10, used for module SPI flash?

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Simpsons"
#define WLAN_PASS       "6DskwPNYqtkm4V"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Tompa21"
#define AIO_KEY         "df7dbaf71dd98c09cf64115c672d252c687045b3"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//const char PHOTOCELL_FEED[] PROGMEM = AIO_USERNAME "/feeds/photocell";
//Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, PHOTOCELL_FEED);

// Setup a feed called 'LED1' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//const char LED1_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_led1";
//Adafruit_MQTT_Subscribe led1 = Adafruit_MQTT_Subscribe(&mqtt, LED1_FEED);

// Setup a feed called 'LEDs' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char ALL_LEDS_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_all_leds";
Adafruit_MQTT_Subscribe all_leds = Adafruit_MQTT_Subscribe(&mqtt, ALL_LEDS_FEED);
// Setup a feed for all LEDs for publishing.
Adafruit_MQTT_Publish all_leds_pub_feed = Adafruit_MQTT_Publish(&mqtt, ALL_LEDS_FEED);

// Setup a feed called 'LED7_VALUE' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char LED7_VALUE_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_led7_value";
Adafruit_MQTT_Subscribe led7_value = Adafruit_MQTT_Subscribe(&mqtt, LED7_VALUE_FEED);


/*************************** Sketch Code ************************************/

unsigned long lastPingTime;
unsigned int led7_light_value = 0;
bool leds_on = false;
bool leds_time_state = false;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP; 
// time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
const int timeZone = 1;     // Central European Time

unsigned lastNtpTime;

// Function declarations
void MQTT_connect();
void sendNTPpacket(IPAddress &address);
void receiveNtpTime();
time_t getNtpTime();
void printDigits(int digits);
void calculateLightsState();


void setup() 
{
    initGpio();
    initDebugUart();
//  Serial.begin(115200);
//  delay(1000);

//  Serial.println(); Serial.println();
//  Serial.println(F("Lego Hus"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP());

  // Setup MQTT subscription for the ledx onoff feeds.
//  mqtt.subscribe(&led1);
  mqtt.subscribe(&all_leds);
  mqtt.subscribe(&led7_value);

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  // Setup the time server sync
  setSyncProvider(getNtpTime);  
}


void loop() {
  // Calculate if the lights should be on or off
  calculateLightsState();
  
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(30000))) {

    
    // check if there are any all_leds events
    if (subscription == &all_leds) {
      // write the current state to all LEDs
      if (strcmp((char *)all_leds.lastread, "ON") == 0) {
        leds_on = true;
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
        digitalWrite(LED4, HIGH);
        digitalWrite(LED5, HIGH);
        digitalWrite(LED6, HIGH);
        analogWrite(LED7, led7_light_value);
      } else {
        leds_on = false;
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        digitalWrite(LED4, LOW);
        digitalWrite(LED5, LOW);
        digitalWrite(LED6, LOW);
        analogWrite(LED7, 0);
      }
    }
    
    // check if there are any led7_value events
    if (subscription == &led7_value) {
      // write the current state to LED7      
      char *value = (char *)led7_value.lastread;
      led7_light_value = atoi(value);
      Serial.println(led7_light_value);
      if (leds_on) {
        analogWrite(LED7, led7_light_value);
      } else {
        analogWrite(LED7, 0);
      }
    }

  }

  // Now we can publish stuff!
  // TODO: Skicka mer sällan?
  // TODO: Skicka bara om tiden är "rätt"
  Serial.print(F("\nSending leds_state "));
  if (leds_time_state) {
    Serial.print("ON");
    Serial.print("...");
    if (!all_leds_pub_feed.publish("ON")) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
  } else {
    Serial.print("OFF");
    Serial.print("...");
    if (!all_leds_pub_feed.publish("OFF")) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
  }

  if (millis() - lastPingTime > 120000) {
    lastPingTime = millis();
    // ping the server to keep the mqtt connection alive
    if(!mqtt.ping()) {
      Serial.println("Ping error");
      mqtt.disconnect();
    }
  }

  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year()); 
  Serial.println(); 

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  digitalWrite(BLUE_LED, HIGH);
  
  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  
  digitalWrite(BLUE_LED, LOW);
  lastPingTime = millis();
  Serial.println("MQTT Connected!");
}



void receiveNtpTime()
{
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
    Serial.println();
  }

}

/*-------- NTP code ----------*/

//const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  WiFi.hostByName(ntpServerName, timeServerIP);   //get a random server from the pool
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void calculateLightsState()
{
  TimeElements tm;
  tm.Second = 0;
  tm.Minute = 0;
  tm.Hour = 0;
  tm.Day = 1;
  tm.Month = 1;
  tm.Year = year() - 1970;

  time_t t = makeTime(tm);

  int dayOfYear = (now() - t) / (60 * 60 * 24);

  // Calculate Sun Up time
  TimeElements sunUp;
  sunUp.Second = 0;
  sunUp.Day = day();
  sunUp.Month = month();
  sunUp.Year = year() - 1970;
  time_t sunUpSeconds;
  
  if (dayOfYear < (365/2)) {
    double temp = 8.7 - (8.7-4.6) * 2 * dayOfYear / 365.0;
    sunUp.Hour = temp / 1;
    sunUp.Minute = (temp - sunUp.Hour) * 60;
    sunUpSeconds = makeTime(sunUp);
    Serial.print("Sun up: "); Serial.print(hour(sunUpSeconds)); Serial.print(":"); Serial.println(minute(sunUpSeconds));
  } else {
    double temp = 4.6 + (8.7-4.6) * (2 * dayOfYear - 365) / 365.0;
    sunUp.Hour = temp / 1;
    sunUp.Minute = (temp - sunUp.Hour) * 60;
    sunUpSeconds = makeTime(sunUp);
    Serial.print("Sun up: "); Serial.print(hour(sunUpSeconds)); Serial.print(":"); Serial.println(minute(sunUpSeconds));
  }

  // Calculate Sun Down time
  TimeElements sunDown;
  sunDown.Second = 0;
  sunDown.Day = day();
  sunDown.Month = month();
  sunDown.Year = year() - 1970;
  time_t sunDownSeconds;
  
  if (dayOfYear < (365/2)) {
    double temp = 15.7 + (21.8-15.7) * 2 * dayOfYear / 365.0;
    sunDown.Hour = temp / 1;
    sunDown.Minute = (temp - sunDown.Hour) * 60;
    sunDownSeconds = makeTime(sunDown);
    Serial.print("Sun down: "); Serial.print(hour(sunDownSeconds)); Serial.print(":"); Serial.println(minute(sunDownSeconds));
  } else {
    double temp = 21.8 - (21.8-15.7) * (2 * dayOfYear - 365) / 365.0;
    sunDown.Hour = temp / 1;
    sunDown.Minute = (temp - sunDown.Hour) * 60;
    sunDownSeconds = makeTime(sunDown);
    Serial.print("Sun down: "); Serial.print(hour(sunDownSeconds)); Serial.print(":"); Serial.println(minute(sunDownSeconds));
  }

  //Set morning on time
  TimeElements morningOn;
  morningOn.Second = 0;
  morningOn.Minute = 0;
  morningOn.Hour = 6;
  morningOn.Day = day();
  morningOn.Month = month();
  morningOn.Year = year() - 1970;
  // TODO: Fixa sommartid
  time_t morningOnSeconds = makeTime(morningOn);

  //Set evening off time
  TimeElements eveningOff;
  eveningOff.Second = 0;
  eveningOff.Minute = 59;
  eveningOff.Hour = 23;
  eveningOff.Day = day();
  eveningOff.Month = month();
  eveningOff.Year = year() - 1970;
  // TODO: Fixa sommartid
  time_t eveningOffSeconds = makeTime(eveningOff);

  // Set light state
  
  if (((now() > morningOnSeconds) && (now() < sunUpSeconds)) || 
     ((now() > sunDownSeconds) && (now() < eveningOffSeconds)))
  {
    Serial.println("Lights ON");
    leds_time_state = true;
  } else {
    Serial.println("Lights OFF");
    leds_time_state = false;
  }
  
}


/**
 * Function that initializes the GPIOs
 */
void initGpio()
{
  // set power switch tail pin as an output
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, HIGH);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(LED6, OUTPUT);
  pinMode(LED7, OUTPUT);
}


/**
 * Function that initializes the debug UART aka Serial
 */
void initDebugUart() 
{
    delay(2000);
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB
        delay(100);
    }
    Serial.println("*** Booting up LegoHus ... ***");
}


#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <TimeLib.h> 


#define ENABLE_DEBUG_PRINTS
#define ENABLE_ERROR_PRINTS

#ifdef ENABLE_DEBUG_PRINTS
  #define DBG_PRINT(...) { Serial.print(__VA_ARGS__); }
  #define DBG_PRINTLN(...) { Serial.println(__VA_ARGS__); }
#else
  #define DBG_PRINT(...) {}
  #define DBG_PRINTLN(...) {}
#endif
#ifdef ENABLE_ERR_PRINTS
  #define ERR_PRINT(...) { Serial.print(__VA_ARGS__); }
  #define ERR_PRINTLN(...) { Serial.println(__VA_ARGS__); }
#else
  #define ERR_PRINT(...) {}
  #define ERR_PRINTLN(...) {}
#endif

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
#define AIO_SERVERPORT  8883
#define AIO_USERNAME    "Tompa21"
#define AIO_KEY         "df7dbaf71dd98c09cf64115c672d252c687045b3"

/************ Global State (you don't need to change this!) ******************/

WiFiClientSecure client;

//// Store the MQTT server, username, and password in flash memory.
//// This is required for using the Adafruit MQTT library.
//const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
//// Set a unique MQTT client ID using the AIO key + the date and time the sketch
//// was compiled (so this should be unique across multiple devices for a user,
//// alternatively you can manually set this to a GUID or other random value).
//const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
//const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
//const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
//Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

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
Adafruit_MQTT_Publish led7_pub_feed = Adafruit_MQTT_Publish(&mqtt, LED7_VALUE_FEED);

const char TIME_FEED[] PROGMEM = "time/seconds";
Adafruit_MQTT_Subscribe time_sub = Adafruit_MQTT_Subscribe(&mqtt, TIME_FEED);
Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

const char WILL_FEED[] PROGMEM = AIO_USERNAME "/feeds/disconnect";

/*************************** Error Reporting *********************************/

const char ERROR_FEED[] PROGMEM = AIO_USERNAME "/errors";
Adafruit_MQTT_Subscribe errors = Adafruit_MQTT_Subscribe(&mqtt, ERROR_FEED);

const char THROTTLE_FEED[] PROGMEM = AIO_USERNAME "/throttle";
Adafruit_MQTT_Subscribe throttle = Adafruit_MQTT_Subscribe(&mqtt, THROTTLE_FEED);

/*************************** Sketch Code ************************************/

unsigned long lastPingTime;
unsigned int led7_light_value = 0;
bool leds_on = false;
bool leds_time_state = false;
bool sunUp = false;
bool firstTime = true;
bool lastState;
bool currentState;

time_t timeNow;

// A UDP instance to let us send and receive packets over UDP
//WiFiUDP udp;

//unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServerIP; 
// time.nist.gov NTP server address
//const char* ntpServerName = "time.nist.gov";

//const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
//byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
//const int timeZone = 1;     // Central European Time

//unsigned lastNtpTime;

// Function declarations
//void MQTT_connect();
//void sendNTPpacket(IPAddress &address);
//void receiveNtpTime();
//time_t getNtpTime();
//void printDigits(int digits);
//void calculateLightsState();


void setup() 
{
//    initGpio();
    initDebugUart();
    initWifi();
    initMqtt();
    
    timefeed.setCallback(timecallback);
    mqtt.subscribe(&timefeed);

//    while (1);
}


void loop() {
  int8_t code;

    // connect to adafruit io if not connected
    if (!mqtt.connected()) {

        Serial.print("Connecting to Adafruit IO... ");
        if((code = mqtt.connect()) != 0) {
            Serial.println(mqtt.connectErrorString(code));
            while(1);
        }
        Serial.println("connected!");
    }

//    // Ensure the connection to the MQTT server is alive (this will make the first
//    // connection and automatically reconnect when disconnected).  See the MQTT_connect
//    // function definition further below.
//    MQTT_connect();
//
//    // this is our 'wait for incoming subscription packets' busy subloop
//    Adafruit_MQTT_Subscribe *subscription;
//    while ((subscription = mqtt.readSubscription(500))) {
//
//        if (subscription == &time_sub) {
//            time_sub.lastread[time_sub.datalen - 3] = 0;    // Removing last 3 characters = dividing by 1000
//            timeNow = atol((char *)time_sub.lastread);
//            setTime(timeNow);
//            debugPrintTime();
//            if ((timeNow > getSunriseTime()) && (timeNow < getSunsetTime())) {
//                DBG_PRINTLN("Sun is up");
//                currentState = 1;
//                if (firstTime || (lastState != currentState)) {
//                    firstTime = false;
//                    DBG_PRINTLN("Sending lights off ...")
//                    if (!all_leds_pub_feed.publish("OFF")) {
//                        ERR_PRINTLN(F("Publish Failed ..."));
//                    }
//                }
//                lastState = currentState;
//            } else {
//                DBG_PRINTLN("Sun is down");
//                currentState = 0;
//                if (firstTime || (lastState != currentState)) {
//                    firstTime = false;
//                    DBG_PRINTLN("Sending lights on ...")
//                    if (!all_leds_pub_feed.publish("ON")) {
//                        ERR_PRINTLN(F("Publish Failed ..."));
//                    }
//                    if (!led7_pub_feed.publish(30)) {
//                        ERR_PRINTLN(F("Publish Failed ..."));
//                    }
//                }
//                lastState = currentState;
//            }
//        } else if (subscription == &all_leds) {
//            // write the current state to all LEDs
//            if (strcmp((char *)all_leds.lastread, "ON") == 0) {
//                leds_on = true;
//                digitalWrite(LED1, HIGH);
//                digitalWrite(LED2, HIGH);
//                digitalWrite(LED3, HIGH);
//                digitalWrite(LED4, HIGH);
//                digitalWrite(LED5, HIGH);
//                digitalWrite(LED6, HIGH);
//                analogWrite(LED7, led7_light_value);
//            } else {
//                leds_on = false;
//                digitalWrite(LED1, LOW);
//                digitalWrite(LED2, LOW);
//                digitalWrite(LED3, LOW);
//                digitalWrite(LED4, LOW);
//                digitalWrite(LED5, LOW);
//                digitalWrite(LED6, LOW);
//                analogWrite(LED7, 0);
//            }
//        } else if (subscription == &led7_value) {
//            // write the current state to LED7      
//            char *value = (char *)led7_value.lastread;
//            led7_light_value = atoi(value);
//            DBG_PRINTLN(led7_light_value);
//            if (leds_on) {
//                analogWrite(LED7, led7_light_value);
//            } else {
//                analogWrite(LED7, 0);
//            }
//        } else if(subscription == &errors) {
//            ERR_PRINT(F("ERROR: "));
//            ERR_PRINTLN((char *)errors.lastread);
//        } else if(subscription == &throttle) {
//            ERR_PRINTLN((char *)throttle.lastread);
//        }
//    }

    // Send keep alive
    if (millis() - lastPingTime > 120000) {
        lastPingTime = millis();
        // ping the server to keep the mqtt connection alive
        if(!mqtt.ping()) {
            ERR_PRINTLN("Ping error");
            mqtt.disconnect();
        }
    }

    mqtt.processPackets(1000);
    mqtt.ping();

}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
    int8_t ret;

    DBG_PRINTLN("Hit kommer vi ... ");
    
    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }

    digitalWrite(BLUE_LED, HIGH);
  
    DBG_PRINTLN("Connecting to MQTT... ");

    while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        DBG_PRINTLN(mqtt.connectErrorString(ret));
        DBG_PRINTLN(ret);
        DBG_PRINTLN("Retrying MQTT connection in 30 seconds...");
        mqtt.disconnect();
        delay(30000);  // wait 30 seconds
    }
  
    digitalWrite(BLUE_LED, LOW);
    lastPingTime = millis();
    DBG_PRINTLN("MQTT Connected!");
}

void timecallback(uint32_t current) {

//  // stash previous hour
//  int previous = hour;
//
//  // adjust to local time zone
//  current += (timeZone * 60 * 60);
//
//  // calculate current hour
//  hour = (current / 60 / 60) % 24;
//
//  // only trigger on interval
//  if((hour != previous) && (hour % interval) == 0) {
//    Serial.println("Run your code here");
//  }

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
    DBG_PRINTLN();
    DBG_PRINTLN("*** Booting up LegoHus ... ***");
}


/**
 * Function that initializes the Wifi
 */
void initWifi()
{
    DBG_PRINTLN("Initializing Wifi ...");

//    listNetworks();
    
    // attempt to connect to Wifi network:
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    
//    DBG_PRINT("Attempting to connect to SSID: ");
//    DBG_PRINTLN(WLAN_SSID);
    
    while (WiFi.status() != WL_CONNECTED) {
        // wait for connection:
        delay(500);
        DBG_PRINT('.');
        printWifiConnectionStatus();
    }
    
    DBG_PRINTLN();
    
    // you're connected now, so print out the status:
    printWifiStatus();
}


void initMqtt()
{
    DBG_PRINTLN("Initializing MQTT ...");
    
    // Setup MQTT subscriptions.
//    mqtt.subscribe(&all_leds);
//    mqtt.subscribe(&led7_value);
//    mqtt.subscribe(&time_sub);

    // Setup MQTT subscriptions for throttle & error messages
//    mqtt.subscribe(&throttle);
//    mqtt.subscribe(&errors);
    
//    mqtt.will(FPSTR(WILL_FEED), "LegoHus disconnected", 0, 1);
//    mqtt.will(FPSTR(WILL_FEED), "LegoHus", 0, 1);
    
    DBG_PRINTLN("MQTT ...");
}


void printDigits(int digits)
{
    // utility for digital clock display: prints preceding colon and leading 0
    DBG_PRINT(":");
    if(digits < 10) {
        DBG_PRINT('0');
    }
    DBG_PRINT(digits);
}


time_t getSunriseTime()
{
    TimeElements tm;
    int dayOfYear;

    dayOfYear = getDayOfYear(now());

    tm.Year = year() - 1970;
    tm.Month = month();
    tm.Day = day();
    tm.Second = 0;

    double winter = 6.7;
    double summer = 2.6;

    if (dayOfYear < (365/2)) {
        double temp = winter - (winter - summer) * 2 * dayOfYear / 365.0;
        tm.Hour = temp / 1;
        tm.Minute = (temp - tm.Hour) * 60;
    } else {
        double temp = summer + (winter - summer) * (2 * dayOfYear - 365) / 365.0;
        tm.Hour = temp / 1;
        tm.Minute = (temp - tm.Hour) * 60;
    }

    DBG_PRINTLN(makeTime(tm)); 
    DBG_PRINT("Sun rises at: "); 
    DBG_PRINT(tm.Year + 1970);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Month);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Day);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Hour);
    DBG_PRINT(":"); 
    DBG_PRINTLN(tm.Minute);
        
    return makeTime(tm);
}


time_t getSunsetTime()
{
    TimeElements tm;
    int dayOfYear;

    dayOfYear = getDayOfYear(now());

    tm.Year = year() - 1970;
    tm.Month = month();
    tm.Day = day();
    tm.Second = 0;

    double winter = 13.7;
    double summer = 19.8;
    
    if (dayOfYear < (365/2)) {
        double temp = winter + (summer - winter) * 2 * dayOfYear / 365.0;
        tm.Hour = temp / 1;
        tm.Minute = (temp - tm.Hour) * 60;
    } else {
        double temp = summer - (summer - winter) * (2 * dayOfYear - 365) / 365.0;
        tm.Hour = temp / 1;
        tm.Minute = (temp - tm.Hour) * 60;
    }

    DBG_PRINTLN(makeTime(tm)); 
    DBG_PRINT("Sun sets at: "); 
    DBG_PRINT(tm.Year + 1970);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Month);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Day);
    DBG_PRINT(" ");
    DBG_PRINT(tm.Hour);
    DBG_PRINT(":"); 
    DBG_PRINTLN(tm.Minute);
        
    return makeTime(tm);
}


int getDayOfYear(time_t epoch)
{
    TimeElements tm;
    tm.Second = 0;
    tm.Minute = 0;
    tm.Hour = 0;
    tm.Day = 1;
    tm.Month = 1;
    tm.Year = year() - 1970;
    time_t t = makeTime(tm);
    int dayOfYear = (epoch - t) / (60 * 60 * 24);
    DBG_PRINTLN(dayOfYear);

    return dayOfYear;
}


//void calculateLightsState()
//{
//  TimeElements tm;
//  tm.Second = 0;
//  tm.Minute = 0;
//  tm.Hour = 0;
//  tm.Day = 1;
//  tm.Month = 1;
//  tm.Year = year() - 1970;
//
//  time_t t = makeTime(tm);
//
//  int dayOfYear = (now() - t) / (60 * 60 * 24);
//
//  // Calculate Sun Up time
//  TimeElements sunUp;
//  sunUp.Second = 0;
//  sunUp.Day = day();
//  sunUp.Month = month();
//  sunUp.Year = year() - 1970;
//  time_t sunUpSeconds;
//  
//  if (dayOfYear < (365/2)) {
//    double temp = 8.7 - (8.7-4.6) * 2 * dayOfYear / 365.0;
//    sunUp.Hour = temp / 1;
//    sunUp.Minute = (temp - sunUp.Hour) * 60;
//    sunUpSeconds = makeTime(sunUp);
//    Serial.print("Sun up: "); Serial.print(hour(sunUpSeconds)); Serial.print(":"); Serial.println(minute(sunUpSeconds));
//  } else {
//    double temp = 4.6 + (8.7-4.6) * (2 * dayOfYear - 365) / 365.0;
//    sunUp.Hour = temp / 1;
//    sunUp.Minute = (temp - sunUp.Hour) * 60;
//    sunUpSeconds = makeTime(sunUp);
//    Serial.print("Sun up: "); Serial.print(hour(sunUpSeconds)); Serial.print(":"); Serial.println(minute(sunUpSeconds));
//  }
//
//  // Calculate Sun Down time
//  TimeElements sunDown;
//  sunDown.Second = 0;
//  sunDown.Day = day();
//  sunDown.Month = month();
//  sunDown.Year = year() - 1970;
//  time_t sunDownSeconds;
//  
//  if (dayOfYear < (365/2)) {
//    double temp = 15.7 + (21.8-15.7) * 2 * dayOfYear / 365.0;
//    sunDown.Hour = temp / 1;
//    sunDown.Minute = (temp - sunDown.Hour) * 60;
//    sunDownSeconds = makeTime(sunDown);
//    Serial.print("Sun down: "); Serial.print(hour(sunDownSeconds)); Serial.print(":"); Serial.println(minute(sunDownSeconds));
//  } else {
//    double temp = 21.8 - (21.8-15.7) * (2 * dayOfYear - 365) / 365.0;
//    sunDown.Hour = temp / 1;
//    sunDown.Minute = (temp - sunDown.Hour) * 60;
//    sunDownSeconds = makeTime(sunDown);
//    Serial.print("Sun down: "); Serial.print(hour(sunDownSeconds)); Serial.print(":"); Serial.println(minute(sunDownSeconds));
//  }
//
//  //Set morning on time
//  TimeElements morningOn;
//  morningOn.Second = 0;
//  morningOn.Minute = 0;
//  morningOn.Hour = 6;
//  morningOn.Day = day();
//  morningOn.Month = month();
//  morningOn.Year = year() - 1970;
//  // TODO: Fixa sommartid
//  time_t morningOnSeconds = makeTime(morningOn);
//
//  //Set evening off time
//  TimeElements eveningOff;
//  eveningOff.Second = 0;
//  eveningOff.Minute = 59;
//  eveningOff.Hour = 23;
//  eveningOff.Day = day();
//  eveningOff.Month = month();
//  eveningOff.Year = year() - 1970;
//  // TODO: Fixa sommartid
//  time_t eveningOffSeconds = makeTime(eveningOff);
//
//  // Set light state
//  
//  if (((now() > morningOnSeconds) && (now() < sunUpSeconds)) || 
//     ((now() > sunDownSeconds) && (now() < eveningOffSeconds)))
//  {
//    Serial.println("Lights ON");
//    leds_time_state = true;
//  } else {
//    Serial.println("Lights OFF");
//    leds_time_state = false;
//  }
//  
//}


void listNetworks() {
  // scan for nearby networks:
  DBG_PRINTLN("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    ERR_PRINTLN("Couldn't get a wifi connection");
    while (true);
  }

  // print the list of networks seen:
  DBG_PRINT("number of available networks:");
  DBG_PRINTLN(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    DBG_PRINT(thisNet);
    DBG_PRINT(") ");
    DBG_PRINT(WiFi.SSID(thisNet));
    DBG_PRINT("\tSignal: ");
    DBG_PRINT(WiFi.RSSI(thisNet));
    DBG_PRINT(" dBm");
    DBG_PRINT("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
    Serial.flush();
  }
}


void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      DBG_PRINTLN("WEP");
      break;
    case ENC_TYPE_TKIP:
      DBG_PRINTLN("WPA");
      break;
    case ENC_TYPE_CCMP:
      DBG_PRINTLN("WPA2");
      break;
    case ENC_TYPE_NONE:
      DBG_PRINTLN("None");
      break;
    case ENC_TYPE_AUTO:
      DBG_PRINTLN("Auto");
      break;
  }
}


void printWifiStatus() {
    printWifiConnectionStatus();

    // print the SSID of the network you're attached to:
    DBG_PRINT("SSID: ");
    DBG_PRINTLN(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    DBG_PRINT("IP Address: ");
    DBG_PRINTLN(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    DBG_PRINT("signal strength (RSSI):");
    DBG_PRINT(rssi);
    DBG_PRINTLN(" dBm");
    DBG_PRINTLN();
}


void printWifiConnectionStatus()
{
    DBG_PRINT("Wifi status: ");
    switch (WiFi.status()) {
        case WL_CONNECTED:
            DBG_PRINTLN("WL_CONNECTED");
            break;
            
        case WL_NO_SHIELD:
            DBG_PRINTLN("WL_NO_SHIELD");
            break;
            
        case WL_IDLE_STATUS:
            DBG_PRINTLN("WL_IDLE_STATUS");
            break;
            
        case WL_NO_SSID_AVAIL:
            DBG_PRINTLN("WL_NO_SSID_AVAIL");
            break;
            
        case WL_SCAN_COMPLETED:
            DBG_PRINTLN("WL_SCAN_COMPLETED");
            break;
            
        case WL_CONNECT_FAILED:
            DBG_PRINTLN("WL_CONNECT_FAILED");
            break;
            
        case WL_CONNECTION_LOST:
            DBG_PRINTLN("WL_CONNECTION_LOST");
            break;
            
        case WL_DISCONNECTED:
            DBG_PRINTLN("WL_DISCONNECTED");
            break;
    }
}


void debugPrintTime()
{
    DBG_PRINTLN();
    DBG_PRINTLN(timeNow);
    
    // print the hour, minute and second:
    DBG_PRINT("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    DBG_PRINT((timeNow  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    DBG_PRINT(':');
    if ( ((timeNow % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      DBG_PRINT('0');
    }
    DBG_PRINT((timeNow  % 3600) / 60); // print the minute (3600 equals secs per minute)
    DBG_PRINT(':');
    if ( (timeNow % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      DBG_PRINT('0');
    }
    DBG_PRINTLN(timeNow % 60); // print the second
}


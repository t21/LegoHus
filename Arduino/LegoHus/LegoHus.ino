#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define ENABLE_DEBUG_PRINTS

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

WiFiClientSecure client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe timeFeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

const char ALL_LEDS_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_all_leds";
Adafruit_MQTT_Subscribe allLedsFeed = Adafruit_MQTT_Subscribe(&mqtt, ALL_LEDS_FEED);
Adafruit_MQTT_Publish all_leds_pub = Adafruit_MQTT_Publish(&mqtt, ALL_LEDS_FEED);

const char LED7_VALUE_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_led7_value";
Adafruit_MQTT_Subscribe led7ValueFeed = Adafruit_MQTT_Subscribe(&mqtt, LED7_VALUE_FEED);
Adafruit_MQTT_Publish led7ValuePub = Adafruit_MQTT_Publish(&mqtt, LED7_VALUE_FEED);

const char WILL_FEED[] PROGMEM = AIO_USERNAME "/feeds/disconnect";

/*************************** Error Reporting *********************************/

const char ERROR_FEED[] PROGMEM = AIO_USERNAME "/errors";
Adafruit_MQTT_Subscribe errors = Adafruit_MQTT_Subscribe(&mqtt, ERROR_FEED);

const char THROTTLE_FEED[] PROGMEM = AIO_USERNAME "/throttle";
Adafruit_MQTT_Subscribe throttle = Adafruit_MQTT_Subscribe(&mqtt, THROTTLE_FEED);


typedef enum {
    STATE_NONE,
    STATE_SUN_UP,
    STATE_SUN_DOWN
} state_t;

state_t currentState = STATE_NONE;



void timeCallback(uint32_t timeNow) {

    Serial.println(timeNow);
    Serial.println(getDayOfYear(timeNow));
    Serial.print("Sunrise:");
    Serial.println(getSunriseTime(timeNow));
    Serial.print("Sunset:");
    Serial.println(getSunsetTime(timeNow));
    
    if ((timeNow > getSunriseTime(timeNow)) && (timeNow < getSunsetTime(timeNow))) {
        Serial.println("Sun is up");
        if (currentState == STATE_NONE) {
            currentState = STATE_SUN_UP;
            Serial.println("Sending lights off ...");
            if (!all_leds_pub.publish("OFF")) {
                Serial.println(F("Publish Failed ..."));
            }
        } else if (currentState == STATE_SUN_DOWN) {
            currentState = STATE_SUN_UP;
            Serial.println("Sending lights off ...");
            if (!all_leds_pub.publish("OFF")) {
                Serial.println(F("Publish Failed ..."));
            }
        }
    } else {
        Serial.println("Sun is down");
        if (currentState == STATE_NONE) {
            currentState = STATE_SUN_DOWN;
            Serial.println("Sending lights on ...");
            if (!all_leds_pub.publish("ON")) {
                Serial.println(F("Publish Failed ..."));
            }
            if (!led7ValuePub.publish(30)) {
                Serial.println(F("Publish Failed ..."));
            }
        } else if (currentState == STATE_SUN_UP) {
            currentState = STATE_SUN_DOWN;
            Serial.println("Sending lights on ...");
            if (!all_leds_pub.publish("ON")) {
                Serial.println(F("Publish Failed ..."));
            }
            if (!led7ValuePub.publish(30)) {
                Serial.println(F("Publish Failed ..."));
            }
        }
    }
}


void allLedsCallback(char *str, uint16_t len) {
    if (strcmp((char *)str, "ON") == 0) {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
        digitalWrite(LED4, HIGH);
        digitalWrite(LED5, HIGH);
        digitalWrite(LED6, HIGH);
        analogWrite(LED7, 30);
        if (!led7ValuePub.publish(30)) {
            Serial.println(F("Publish Failed ..."));
        }
    } else {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        digitalWrite(LED4, LOW);
        digitalWrite(LED5, LOW);
        digitalWrite(LED6, LOW);
        analogWrite(LED7, 0);
        if (!led7ValuePub.publish(0)) {
            Serial.println(F("Publish Failed ..."));
        }
    }   
}


void led7ValueCallback(uint32_t value) {
    analogWrite(LED7, value);
}


void errorCallback(char *str, uint16_t len) {
    Serial.print(F("ERROR: "));
    Serial.println(str);
}


void throttleCallback(char *str, uint16_t len) {
    Serial.println(str);
}


void setup() 
{
    initGpio();
    
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println(F("*** Booting up LegoHus ... ***"));

    initWifi();
    
    timeFeed.setCallback(timeCallback);
    mqtt.subscribe(&timeFeed);

    allLedsFeed.setCallback(allLedsCallback);
    mqtt.subscribe(&allLedsFeed);

    led7ValueFeed.setCallback(led7ValueCallback);
    mqtt.subscribe(&led7ValueFeed);

    mqtt.will(FPSTR(WILL_FEED), "LegoHus", 0, 1);
}


void loop() 
{
    int8_t err_code;
    unsigned long lastPingTime;

    // connect to adafruit io if not connected
    if (!mqtt.connected()) {
        Serial.print("Connecting to Adafruit IO... ");
        if((err_code = mqtt.connect()) != 0) {
            Serial.println(mqtt.connectErrorString(err_code));
            while(1);
        }
        Serial.println("connected!");
    }

    mqtt.processPackets(1000);
  
    // Send keep alive
    if (millis() - lastPingTime > 120000) {
        lastPingTime = millis();
        // ping the server to keep the mqtt connection alive
        if(!mqtt.ping()) {
            Serial.println("Ping error");
            mqtt.disconnect();
        }
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
 * Function that initializes the Wifi
 */
void initWifi()
{
    Serial.println("Initializing Wifi ...");
    
    listNetworks();

    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WLAN_SSID);
    
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println(F(" WiFi connected."));
    
    printWifiStatus();    
}






void printDigits(int digits)
{
    // utility for digital clock display: prints preceding colon and leading 0
    Serial.print(":");
    if(digits < 10) {
        Serial.print('0');
    }
    Serial.print(digits);
}


unsigned long getSunriseTime(unsigned long timeNow)
{
    int dayOfYear;
    unsigned int startOfDayInSeconds;
    double timeOfDay;

    dayOfYear = getDayOfYear(timeNow);
    startOfDayInSeconds = getStartOfDayInSeconds(timeNow);

    double winter = 6.7;
    double summer = 2.6;

    if (dayOfYear < (365/2)) {
        timeOfDay = winter - (winter - summer) * 2 * dayOfYear / 365.0;
    } else {
        timeOfDay = summer + (winter - summer) * (2 * dayOfYear - 365) / 365.0;
    }

    return (startOfDayInSeconds + timeOfDay * 3600);
}


unsigned long getSunsetTime(unsigned long timeNow)
{
    int dayOfYear;
    unsigned int startOfDayInSeconds;
    double timeOfDay;

    dayOfYear = getDayOfYear(timeNow);
    startOfDayInSeconds = getStartOfDayInSeconds(timeNow);

    double winter = 13.7;
    double summer = 19.8;

    if (dayOfYear < (365/2)) {
        timeOfDay = winter + (summer - winter) * 2 * dayOfYear / 365.0;
    } else {
        timeOfDay = summer - (summer - winter) * (2 * dayOfYear - 365) / 365.0;
    }

    return (startOfDayInSeconds + timeOfDay * 3600);
}

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

int getDayOfYear(unsigned long timeInput)
{
    uint8_t year;
    uint8_t month, monthLength;
    uint32_t time;
    unsigned long days;

    time = (uint32_t)timeInput;
//  tm.Second = time % 60;
    time /= 60; // now it is minutes
//  tm.Minute = time % 60;
    time /= 60; // now it is hours
//  tm.Hour = time % 24;
    time /= 24; // now it is days
//  tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1 
  
    year = 0;  
    days = 0;
    while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
        year++;
    }
//  tm.Year = year; // year is offset from 1970 
  
    days -= LEAP_YEAR(year) ? 366 : 365;

    time  -= days; // now it is days in this year, starting at 0
    
    return time;
}


unsigned long getStartOfDayInSeconds(unsigned long timeInput)
{
    uint8_t year;
    uint8_t month, monthLength;
    uint32_t time;
    unsigned long days;
    unsigned long seconds;
    unsigned long minutes;
    unsigned long hours;
    unsigned long secondsToday;

    time = (uint32_t)timeInput;
    seconds = time % 60;
    time /= 60; // now it is minutes
    minutes = time % 60;
    time /= 60; // now it is hours
    hours = time % 24;

    secondsToday = seconds + minutes * 60 + hours * 60 * 60;

    return (timeInput - secondsToday);
}


void listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
    Serial.flush();
  }
}


void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      Serial.println("WEP");
      break;
    case ENC_TYPE_TKIP:
      Serial.println("WPA");
      break;
    case ENC_TYPE_CCMP:
      Serial.println("WPA2");
      break;
    case ENC_TYPE_NONE:
      Serial.println("None");
      break;
    case ENC_TYPE_AUTO:
      Serial.println("Auto");
      break;
  }
}


void printWifiStatus() {
    printWifiConnectionStatus();

    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println();
}


void printWifiConnectionStatus()
{
    Serial.print("Wifi status: ");
    switch (WiFi.status()) {
        case WL_CONNECTED:
            Serial.println("WL_CONNECTED");
            break;
            
        case WL_NO_SHIELD:
            Serial.println("WL_NO_SHIELD");
            break;
            
        case WL_IDLE_STATUS:
            Serial.println("WL_IDLE_STATUS");
            break;
            
        case WL_NO_SSID_AVAIL:
            Serial.println("WL_NO_SSID_AVAIL");
            break;
            
        case WL_SCAN_COMPLETED:
            Serial.println("WL_SCAN_COMPLETED");
            break;
            
        case WL_CONNECT_FAILED:
            Serial.println("WL_CONNECT_FAILED");
            break;
            
        case WL_CONNECTION_LOST:
            Serial.println("WL_CONNECTION_LOST");
            break;
            
        case WL_DISCONNECTED:
            Serial.println("WL_DISCONNECTED");
            break;
    }
}


void debugPrintTime(unsigned int timeNow)
{
    Serial.println();
    Serial.println(timeNow);
    
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((timeNow  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((timeNow % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((timeNow  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (timeNow % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(timeNow % 60); // print the second
}


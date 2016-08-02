#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <math.h>

//#define ENABLE_DEBUG_PRINTS

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

WiFiClientSecure client;

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  8883
#define AIO_USERNAME    "Tompa21"
#define AIO_KEY         "df7dbaf71dd98c09cf64115c672d252c687045b3"

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe timeFeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

const char ALL_LEDS_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_all_leds";
Adafruit_MQTT_Subscribe allLedsFeed = Adafruit_MQTT_Subscribe(&mqtt, ALL_LEDS_FEED);
Adafruit_MQTT_Publish all_leds_pub = Adafruit_MQTT_Publish(&mqtt, ALL_LEDS_FEED);

const char LED7_VALUE_FEED[] PROGMEM = AIO_USERNAME "/feeds/lego_hus_led7_value";
Adafruit_MQTT_Subscribe led7ValueFeed = Adafruit_MQTT_Subscribe(&mqtt, LED7_VALUE_FEED);
Adafruit_MQTT_Publish led7ValuePub = Adafruit_MQTT_Publish(&mqtt, LED7_VALUE_FEED);

const char WILL_FEED[] PROGMEM = AIO_USERNAME "/feeds/disconnect";

const char ERROR_FEED[] PROGMEM = AIO_USERNAME "/errors";
Adafruit_MQTT_Subscribe errorFeed = Adafruit_MQTT_Subscribe(&mqtt, ERROR_FEED);

const char THROTTLE_FEED[] PROGMEM = AIO_USERNAME "/throttle";
Adafruit_MQTT_Subscribe throttleFeed = Adafruit_MQTT_Subscribe(&mqtt, THROTTLE_FEED);

/************************* Global variables *********************************/
typedef enum {
    STATE_NONE,
    STATE_SUN_UP,
    STATE_SUN_DOWN
} state_t;

state_t currentState = STATE_NONE;

const double MY_LATITUDE = 55.70584;
const double MY_LONGITUDE = 13.19321;

/**
 * 
 */
void timeCallback(uint32_t timeNow) {

    #ifdef ENABLE_DEBUG_PRINTS
        Serial.print(timeNow);
        Serial.print(" ");
        debugPrintTime(timeNow);
        Serial.print("SunRise:"); 
        Serial.print(calculateSunrise(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0));
        Serial.print(" ");
        debugPrintTime(calculateSunrise(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0));
        Serial.print("SunSet:"); 
        Serial.print(calculateSunset(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0));
        Serial.print(" ");
        debugPrintTime(calculateSunset(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0));
    #endif
    
    if ((timeNow > calculateSunrise(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0)) && 
        (timeNow < calculateSunset(timeNow, MY_LATITUDE, MY_LONGITUDE, 0, 0))) 
    {
        #ifdef ENABLE_DEBUG_PRINTS
            Serial.println("Sun is up");
        #endif
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
        #ifdef ENABLE_DEBUG_PRINTS
            Serial.println("Sun is down");
        #endif
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
    Serial.print("allLedsCallback: ");
    Serial.println((char *)str);
    
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

    errorFeed.setCallback(errorCallback);
    mqtt.subscribe(&errorFeed);
    
    throttleFeed.setCallback(throttleCallback);
    mqtt.subscribe(&throttleFeed);

    mqtt.will(FPSTR(WILL_FEED), "LegoHus", 0, 1);
}


void loop() 
{
    int8_t err_code;
    unsigned long lastPingTime;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi connection lost ...");
        connectWifi();
    }

    if (mqtt.connected()) {
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
    } else {
        Serial.print("Connecting to Adafruit IO... ");
        if((err_code = mqtt.connect()) != 0) {
            Serial.println(mqtt.connectErrorString(err_code));
            delay(60000);
        } else {
            Serial.println("connected!");
        }    
    }
}


/**
 * Function that initializes the GPIOs
 */
void initGpio()
{
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

//    Serial.print("Attempting to connect to SSID: ");
//    Serial.println(WLAN_SSID);
    
//    WiFi.begin(WLAN_SSID, WLAN_PASS);
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        Serial.print(F("."));
//    }
//    Serial.println(F(" WiFi connected."));

    connectWifi();
    
//    printWifiStatus();    
}


void connectWifi()
{
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WLAN_SSID);
    
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
    }

    Serial.println();
    
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
    unsigned long startOfDayInSeconds;
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
    unsigned long startOfDayInSeconds;
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

    time++; // Add 1 so days are 1-365
    
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


void debugPrintTime(unsigned long timeNow)
{
//    Serial.println();
//    Serial.println(timeNow);
    
    // print the hour, minute and second:
//    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
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


#define ZENITH -.83

unsigned long calculateSunrise(uint32_t timeInSeconds, double lat, double lng, int localOffset, int daylightSavings) {
    /*
    localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0
    */
    //1. first calculate the day of the year
    double N = getDayOfYear(timeInSeconds);
    //Serial.print("N:"); Serial.println(N);

    //2. convert the longitude to hour value and calculate an approximate time
    double lngHour = lng / 15.0;      
    double t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    //float t = N + ((18 - lngHour) / 24)   //if setting time is desired:

    //3. calculate the Sun's mean anomaly   
    double M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
    double L = M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634;
    //Serial.print("L:"); Serial.println(L);
    if (L > 360) {
        L -= 360.0;
    } else if (L < 0) {
        L += 360.0;
    }

    //5a. calculate the Sun's right ascension      
    double RA = 180/PI*atan(0.91764 * tan((PI/180)*L));
    //Serial.print("RA:"); Serial.println(RA);
    if (RA > 360) {
        RA -= 360.0;
    } else if (RA < 0) {
        RA += 360.0;
    }

    //5b. right ascension value needs to be in the same quadrant as L   
    double Lquadrant  = floor( L/90) * 90;
    double RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours   
    RA = RA / 15;

    //6. calculate the Sun's declination
    double sinDec = 0.39782 * sin((PI/180)*L);
    double cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    double cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
    /*   
    if (cosH >  1) 
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    double H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    //float H = acos(cosH) //   if setting time is desired:      
    H = H / 15;

    //8. calculate local mean time of rising/setting      
    double T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
    double UT = T - lngHour;
    //Serial.print("UT:"); Serial.println(UT);
    if (UT > 24) {
        UT -= 24.0;
    } else if (UT < 0) {
        UT += 24.0;
    }

    //10. convert UT value to local time zone of latitude/longitude
//    return UT + localOffset + daylightSavings;
    double timeOfDay = UT + localOffset + daylightSavings;

    unsigned long startOfDayInSeconds = getStartOfDayInSeconds(timeInSeconds);

    return (startOfDayInSeconds + timeOfDay * 3600);
}


unsigned long calculateSunset(uint32_t timeInSeconds, double lat, double lng, int localOffset, int daylightSavings) {
    /*
    localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0
    */
    //1. first calculate the day of the year
    double N = getDayOfYear(timeInSeconds);
    //Serial.print("N:"); Serial.println(N);

    //2. convert the longitude to hour value and calculate an approximate time
    double lngHour = lng / 15.0;      
    //double t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    double t = N + ((18 - lngHour) / 24);   //if setting time is desired:

    //3. calculate the Sun's mean anomaly   
    double M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
    double L = M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634;
    //Serial.print("L:"); Serial.println(L);
    if (L > 360) {
        L -= 360.0;
    } else if (L < 0) {
        L += 360.0;
    }

    //5a. calculate the Sun's right ascension      
    double RA = 180/PI*atan(0.91764 * tan((PI/180)*L));
    //Serial.print("RA:"); Serial.println(RA);
    if (RA > 360) {
        RA -= 360.0;
    } else if (RA < 0) {
        RA += 360.0;
    }

    //5b. right ascension value needs to be in the same quadrant as L   
    double Lquadrant  = floor( L/90) * 90;
    double RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours   
    RA = RA / 15;

    //6. calculate the Sun's declination
    double sinDec = 0.39782 * sin((PI/180)*L);
    double cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    double cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
    /*   
    if (cosH >  1) 
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    //double H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    double H = (180/PI)*acos(cosH); //   if setting time is desired:      
    H = H / 15;

    //8. calculate local mean time of rising/setting      
    double T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
    double UT = T - lngHour;
    //Serial.print("UT:"); Serial.println(UT);
    if (UT > 24) {
        UT -= 24.0;
    } else if (UT < 0) {
        UT += 24.0;
    }

    //10. convert UT value to local time zone of latitude/longitude
//    return UT + localOffset + daylightSavings;
    double timeOfDay = UT + localOffset + daylightSavings;

    unsigned long startOfDayInSeconds = getStartOfDayInSeconds(timeInSeconds);

    return (startOfDayInSeconds + timeOfDay * 3600);
}


float calculateSunrise2(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings) {
    /*
    localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0
    */
    //1. first calculate the day of the year
    float N1 = floor(275 * month / 9);
    float N2 = floor((month + 9) / 12);
    float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    float N = N1 - (N2 * N3) + day - 30;
    //Serial.print("N:"); Serial.println(N);

    //2. convert the longitude to hour value and calculate an approximate time
    float lngHour = lng / 15.0;      
    float t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    //float t = N + ((18 - lngHour) / 24)   //if setting time is desired:

    //3. calculate the Sun's mean anomaly   
    float M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
//    float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634, 360.0);
    float L = M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634;
    //Serial.print("L:"); Serial.println(L);
    if (L > 360) {
        L -= 360.0;
    } else if (L < 0) {
        L += 360.0;
    }

    //5a. calculate the Sun's right ascension      
//    float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)), 360.0);
    float RA = 180/PI*atan(0.91764 * tan((PI/180)*L));
    //Serial.print("RA:"); Serial.println(RA);
    if (RA > 360) {
        RA -= 360.0;
    } else if (RA < 0) {
        RA += 360.0;
    }

    //5b. right ascension value needs to be in the same quadrant as L   
    float Lquadrant  = floor( L/90) * 90;
    float RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours   
    RA = RA / 15;

    //6. calculate the Sun's declination
    float sinDec = 0.39782 * sin((PI/180)*L);
    float cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    float cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
    /*   
    if (cosH >  1) 
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    float H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    //float H = acos(cosH) //   if setting time is desired:      
    H = H / 15;

    //8. calculate local mean time of rising/setting      
    float T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
//    float UT = fmod(T - lngHour,24.0);
    float UT = T - lngHour;
    //Serial.print("UT:"); Serial.println(UT);
    if (UT > 24) {
        UT -= 24.0;
    } else if (UT < 0) {
        UT += 24.0;
    }

    //10. convert UT value to local time zone of latitude/longitude
    return UT + localOffset + daylightSavings;

}

//void printSunrise() {
//    float localT = calculateSunrise(/*args*/);
//    double hours;
//    float minutes = modf(localT,&hours)*60;
//    printf("%.0f:%.0f",hours,minutes);
//}
    

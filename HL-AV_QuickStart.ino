/*
 * Sample sketch using SWIR_MQTTClient library. V0.8
 * Purpose : Enable Sierra Wireless HL serie module to send/receive MQTT data to/from AirVantage server
 *
 * Requirements:
 *      Paho's Embedded MQTTClient C client shall be installed within Arduino's IDE
 *      HL module shall be inserted into the "HL shield", stacked over Arduino Mega2560 (tested)
 *      Pin 0 (RX) & Pin 1 (TX) are physically linked to HL module's UART using jumpers J6 & J8 (HL Shield)
 *      SIM card inserted into slot 1 & Antenna connected
 *      USB to TTL Serial Cable's RX/TX are connected to Pin12 & Pin13 (by default). For program log output
 *      Register your module on AirVantage portal (IMEI & SN may be copied from the program log output)
 *      Assign the password (as set in AirVantage portal) to the flag below: MODULE_AV_PASSWORD
 *
 * Use this sample sketch as a starting point to create your app:
 *      Provide your one-time setup code in doCustomSetup(), instead of modifying setup()
 *      Place your code to perform task on a regular basis in doCustomLoopTask(), instead of modifying loop()
 *      Align your Publish/Subscribe keys to your AirVantage Application Model
 *      Handle incoming messages from AirVantage in messageArrived()
 *      Call postData(keyName, keyValue) to publish key/value data to AirVantage
 *
 * Nhon Chu, May 2015
 *
 */

#include <SPI.h>
#include <SoftwareSerial.h>
#include <MQTTClient.h>
#include "swir_mqtt.h"
#include "swir_debug.h"




//--------------------------
//--- Baseline Configuration
#define APN                     "orangswir.vp"              //APN for Orange
#define APN_LOGIN               ""                          //APN login
#define APN_PASSWORD            ""                          //APN password

#define SIM_PIN_CODE            0                           //default is 0000, it's an integer!

#define BAUDRATE                115200                      //Default baudrate for HL Serie Modules

#define AUTH_WITH_MODULE_IMEI                               //Use Module's IMEI and MODULE_AV_PASSWORD for authentication
#define MODULE_AV_PASSWORD      "sierra"                    //Specify your module's password, as set in AirVantage

                                                            //disable AUTH_WITH_MODULE_IMEI to use below alternate login/pwd
#define ALTERNATE_IDENTIFIER    "359569050023259"           //Specify your system's identifier (alternate option)
#define ALTERNATE_PASSWORD      "toto"                      //Specify your system's password (alternate option)

#define QOS_PUBLISH             0                           //Level of Quality of Service: could be 0, 1 or 2

#define TRACE_ENABLE                                        //Trace toggle. Disable this flag to turn off tracing capability
#define TRACE_RX_PIN            12                          //Tracing Debug info using USB to TTL serial cable, RX PIN
#define TRACE_TX_PIN            13                          //Tracing Debug info using USB to TTL serial cable, TX PIN
#define LED_PIN                 22                          //Tracing Debug info using USB to TTL serial cable, TX PIN
#define BUTTON_PIN              23

#define DELAY_TRY_CONNECTION    5000                        //Delay (ms) to make new attempt to connect to AirVantage
#define DELAY_DO_TASK           10000                       //Periodical delay (ms) to execute application defined task

#define ON_VALUE                "true"                      //default positive value for a boolean data key
#define PUBLISH_DATA_NAME       "MilliSec"                  //Name of the data being published to AirVantage
#define LATITUDE                "_LATITUDE"
#define LONGITUDE               "_LONGITUDE"
#define INCOMING_DATA_NAME      "back_to_normal_mode."               //default incoming data key


//-----------------------------
//--- Baseline Global variables
SWIR_MQTTClient                 _mqttClient(Serial);        //Use Hardware serial to communicate with HL module
int                             _nIsSimReady = 0;           //Track SIM state, set SIM PIN only once
char                            frameGLL[64];
int                             _delay = DELAY_DO_TASK;
//char                            _Latitude[16];
//char                            _Longitude[16];
char*                            _Latitude="4295293.545,N";
char*                            _Longitude="2034934.324,E";
char*                           _offset1 = NULL;
char*                           _offset2 = NULL;
int                             buttonState = 0;         // variable for reading the pushbutton status
unsigned long                   time = 0;
unsigned long                   period_send_gps = 0;
unsigned long                   emergencyDetected;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);   
    
    attachInterrupt(0, blink, CHANGE);
    
    digitalWrite(LED_PIN, LOW);  
      
    period_send_gps= millis() +_delay;
    
    Serial.begin(BAUDRATE);                                 //Select the working baud rate for the HL Module
    //memset(&frameGLL,0,64);
    #ifdef TRACE_ENABLE                                     //USB to TTL serial cable connected to pre-defined Pins
    _mqttClient.setDebugPort(TRACE_RX_PIN, TRACE_TX_PIN, BAUDRATE);
    #endif                                                  //from this point, we can use SWIR_TRACE macro

    SWIR_TRACE(F("Hello, let's get started !\r\nBoot your HL Module now!"));

    _mqttClient.setPublishQos(QOS_PUBLISH);                 //Set Quality of Service
    
    _mqttClient.initGPS();

    doCustomSetup();                                        //provide your one-time setup code in doCustomSetup()
}

void loop()
{
  SWIR_TRACE(F("LOOP"));
 
    if (!_mqttClient.isConnected())
    {
        if (!_nIsSimReady)
        {
            _nIsSimReady = _mqttClient.setSimPIN(SIM_PIN_CODE);                //set PIN code
        }

        if (_mqttClient.isDataCallReady())
        {
            _mqttClient.setAPN(APN, APN_LOGIN, APN_PASSWORD);   //set APN

            #ifdef AUTH_WITH_MODULE_IMEI
            SWIR_TRACE(F("sierra1"));
            _mqttClient.connect(MODULE_AV_PASSWORD);                            //recommended option, try establishing mqtt session
            #else
            SWIR_TRACE(F("sierra2"));
            _mqttClient.connect(ALTERNATE_IDENTIFIER, ALTERNATE_PASSWORD);      //alternative option, use a different identifier/pwd
            #endif

            if (_mqttClient.isConnected())
            {
                _mqttClient.subscribe(messageArrived);          //if connected, then provided the callback to receive mqtt msg from AirVantage
            }
        }        
        delay(DELAY_TRY_CONNECTION);                        //if not connected, let's retry in DELAY_TRY_CONNECTION (ms)
    }
    else
    {
        int nRSSI, nBER, nEcLo;
                                                            //retrieving RSSI (Received Signal Strength Indication)
        if (_mqttClient.getSignalQuality(nRSSI, nBER, nEcLo)) 
        {
            if (nEcLo == -99)
            {
                SWIR_TRACE(F("2G signal strength: RSSI=%d"), nRSSI);
            }
            else
            {
                SWIR_TRACE(F("3G signal strength: RSSI=%d"), nRSSI);
            }
        }
        _mqttClient.loop();                                 //Must be called. Enable SWIR_MQTTClient to handle background tasks

        doCustomLoopTask();                                 //let's perform this task on a regular basis
        
        //delay(DELAY_DO_TASK);                               //let's do this again in DELAY_DO_TASK (ms)
    }
}
void retrieveGPSData()
{
  int sizeofString=0;
  delay(_delay);
  memset(&frameGLL,0,64);
  memset(_Latitude,0,16);
  memset(_Longitude,0,16);
  
  _mqttClient.startGPS(frameGLL);

  _offset1 = strstr(frameGLL,",");
  _offset2 = strstr(_offset1+1,",");
  SWIR_TRACE(F("frameGLL=%d"), &frameGLL[0]);
  SWIR_TRACE(F("_offset1=%d"), _offset1);
  SWIR_TRACE(F("_offset2=%d"), _offset2);
  
  sizeofString=_offset2 - _offset1 + 1;
  
  SWIR_TRACE(F("sizeofString=%d"), sizeofString);
  
  strncpy (_Latitude, _offset1+1,sizeofString);
  
  _offset1 = _offset2 + 2;
  
  _offset2 = strstr(_offset1+1,",");
  
  SWIR_TRACE(F("_offset1=%d"), _offset1);
  SWIR_TRACE(F("_offset2=%d"), _offset2);
  
  sizeofString=_offset2 - _offset1 + 1;
  
  SWIR_TRACE(F("sizeofString=%d"), sizeofString);
  
  strncpy (_Longitude, _offset1+1, sizeofString);
  
  SWIR_TRACE(F("_Latitude=%s"), _Latitude);
  SWIR_TRACE(F("_Longitude=%s"), _Longitude);
}

void postData(char* szDataKey, unsigned long ulDataValue)
{
    if (_mqttClient.isConnected())
    {
        _mqttClient.publish(szDataKey, ulDataValue);
    }
}

void postGPSData(char* szLatitudeKey, char* szLatitudeValue, char* szLongitudeKey, char* szLongitudeValue)
{
    if (_mqttClient.isConnected())
    {
        _mqttClient.publish(szLatitudeKey, szLatitudeValue, szLongitudeKey, szLongitudeValue);
    }
}

void doCustomSetup()
{
    //Place your one-time setup code here

    
}

void doCustomLoopTask()
{
    //Replace the below sample task by your code to be executed on a regular basis
          
    buttonState = digitalRead(BUTTON_PIN);
  
    if (buttonState == HIGH) 
    {    
      // turn LED on:    
      digitalWrite(LED_PIN, HIGH);  
      postData("avep_demo.alarm_status", 1);
      //_mqttClient.emergencyCall("+33698977572");
      _delay = 5000;
    }
  
   if(period_send_gps < millis())
   {
      retrieveGPSData();
    
      //Get GLL structure
      postGPSData(LATITUDE, _Latitude ,LONGITUDE, _Longitude);   
    
      period_send_gps = millis() +_delay;
   }
    
}


void messageArrived(const char* szKey, const char* szValue, const char* szTimestamp)
{
    SWIR_TRACE(F("\r\n>>> Message from AirVantage: %s = %s @ %s\r\n"), szKey, szValue, szTimestamp);
    
    //based on szKey and szValue (depending on Application Model defined in AirVantage), you can trigger an action...

    String sKey = String(szKey);
    String sValue = String(szValue);

    if (sKey == INCOMING_DATA_NAME)
    {
        _delay = DELAY_DO_TASK;
        digitalWrite(LED_PIN, LOW); 
        postData("avep_demo.alarm_status", 0);
    }
}

void interrupt()
{
  state = !state;
}

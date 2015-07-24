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
#define APN                     "internet.maingate.se"      //APN for Maingate
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

#define DELAY_TRY_CONNECTION    5000                        //Delay (ms) to make new attempt to connect to AirVantage
#define DELAY_DO_TASK           10000                       //Periodical delay (ms) to execute application defined task

#define ON_VALUE                "true"                      //default positive value for a boolean data key
#define PUBLISH_DATA_NAME       "MilliSec"                  //Name of the data being published to AirVantage
#define INCOMING_DATA_NAME      "home.TurnOn"               //default incoming data key


//-----------------------------
//--- Baseline Global variables
SWIR_MQTTClient                 _mqttClient(Serial);        //Use Hardware serial to communicate with HL module
int                             _nIsSimReady = 0;           //Track SIM state, set SIM PIN only once


void setup()
{
    Serial.begin(BAUDRATE);                                 //Select the working baud rate for the HL Module

    #ifdef TRACE_ENABLE                                     //USB to TTL serial cable connected to pre-defined Pins
    _mqttClient.setDebugPort(TRACE_RX_PIN, TRACE_TX_PIN, BAUDRATE);
    #endif                                                  //from this point, we can use SWIR_TRACE macro

    SWIR_TRACE(F("Hello, let's get started !\r\nBoot your HL Module now!"));

    _mqttClient.setPublishQos(QOS_PUBLISH);                 //Set Quality of Service

    doCustomSetup();                                        //provide your one-time setup code in doCustomSetup()
}

void loop()
{
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
            _mqttClient.connect(MODULE_AV_PASSWORD);                            //recommended option, try establishing mqtt session
            #else
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
        
        delay(DELAY_DO_TASK);                               //let's do this again in DELAY_DO_TASK (ms)
    }
}

void postData(char* szDataKey, unsigned long ulDataValue)
{
    if (_mqttClient.isConnected())
    {
        _mqttClient.publish(szDataKey, ulDataValue);
    }
}

void doCustomSetup()
{
    //Place your one-time setup code here

    
}

void doCustomLoopTask()
{
    //Replace the below sample task by your code to be executed on a regular basis

    postData(PUBLISH_DATA_NAME, millis());   //let's publish the number of milliseconds since the Arduino has started
}


void messageArrived(const char* szKey, const char* szValue, const char* szTimestamp)
{
    SWIR_TRACE(F("\r\n>>> Message from AirVantage: %s = %s @ %s\r\n"), szKey, szValue, szTimestamp);
    
    //based on szKey and szValue (depending on Application Model defined in AirVantage), you can trigger an action...

    String sKey = String(szKey);
    String sValue = String(szValue);

    if (sKey == INCOMING_DATA_NAME)
    {
        if (sValue == ON_VALUE)
        {
            //perform action corresponding to INCOMING_DATA_NAME = ON_VALUE
        }
        else
        {
            //Perform action corresponding to INCOMING_DATA_NAME != ON_VALUE
        }
    }
}

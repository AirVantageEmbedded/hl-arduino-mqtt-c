/*******************************************************************************
 * MQTT class for Sierra Wireless' HL Serie Modules
 * wrapping Paho's MQTTClient and using TCP_HL layer (replacing the default ipstack)
 *
 * Nhon Chu - May 2015 - V0.8
 *******************************************************************************/

#ifndef SWIR_MQTTCLIENT_H
#define SWIR_MQTTCLIENT_H

#include "arduino.h"

#include "MQTTClient.h"
#include "Countdown.h"

#include "swir_tcp_hl.h"

#include "swir_debug.h"

#include "minmea.h"

//#define MAX_LENGTH1			64
//#define MAX_LENGTH2			256
#define MAX_PAYLOAD_SIZE	256

class SWIR_MQTTClient
{
	public:
		typedef void (*inMessageHandler)(const char* szKey, const char* szValue, const char* szTimestamp);
		typedef void (*messageHandler)(MQTT::MessageData&);

		SWIR_MQTTClient(
					HardwareSerial	&module);
		int	setSimPIN(
					int 			nPinCode = -1);
		int		isSimReady();
		int		setAPN(
					char*			szAPN,
					char*			szAPNlogin = NULL,
					char*			szAPNpassword = NULL);
		int		connect(
					char*			szPassword,				//IMEI is used as identifier
					unsigned long	ulKeepAlive = 60);  	//default keeplive, 30 seconds
		int		connect(
					char*			szIdentifier,
					char*			szPassword,
					unsigned long	ulKeepAlive = 60);
		int 	isDataCallReady();
		int 	getSignalQuality(
					int 			&nRssi,
					int 			&nBer,
					int 			&nEcLo);
		int		isConnected();
		void	disconnect();
		void	loop();
		void	setPublishQos(
					int 			nQoS);					//nQOS = 0, 1 or 2
		int 	publish(
					char*			szKey,
					char*			szValue,
					unsigned long	ulTimestamp = 0);
		int		publish(
					char*			szKey,
					double			dValue,
					unsigned long	ulTimestamp = 0);
		int		publish(
					char*			szKey,
					int 			nValue,
					unsigned long	ulTimestamp = 0);
		int		publish(
					char*			szKey,
					unsigned long	ulValue,
					unsigned long	ulTimestamp = 0);
		int 	publish(
					char* szKey1, 
					char* szValue1, 
					char* szKey2, 
					char* szValue2, 
					unsigned long ulTimestamp = 0);
		int		subscribe(
					inMessageHandler pfnHandler);

		void	setDebugPort(
					int 			nRXport,
					int 			nTXport,
					long 			nBaudrate);
		void 	initGPS();
		void  	startGPS(
					char* frame);
		void 	emergencyCall(
					char* number);
		
		
		
	protected:
		static void		incomingMessageHandler(
							MQTT::MessageData&	md);
		static void		handleCallback(
							char* szPath, char* szKey, char* szValue, char* szTimestamp);
		
		SWIR_TCP_HL													_swirModule;
		MQTT::Client<SWIR_TCP_HL, Countdown, MAX_PAYLOAD_SIZE, 1>*	_pMqttClient;
		char*														_pszSubscribedTopic;
		char*														_pszPublishTopic;
		enum MQTT::QoS												_ePublishQoS;

		static inMessageHandler										_pfnInMsgCallback;
		#ifdef _SWIR_OUTPUT_
		swirOutput													_tracer;
		#endif
};

#endif

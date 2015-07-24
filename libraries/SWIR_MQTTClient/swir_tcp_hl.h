

/*******************************************************************************
 * TCP Socket Layer adapter for Sierra Wireless' HL Serie Modules
 * V0.8
 * Nhon Chu - May 2015
 *******************************************************************************/

 
#ifndef __SWIR_TCP_HL__
#define __SWIR_TCP_HL__

#define _VERBOSE_DEBUG_		//enable this flag for further debug trace

#include "minmea.h"

class SWIR_TCP_HL
{
	public:
		SWIR_TCP_HL();

		void	setSerialObject(
					HardwareSerial	&module);
		int 	setSimPIN(
					int				nPinCode);

		int 	isSimReady();

		int		setAPN(
					char*			szAPN,
					char*			szAPNlogin,
					char*			szAPNpassword);
		int 	getIMEI(
					char*			szIMEI,
					int				nBufferSize);
		int 	getSN(
					char*			szSN,
					int				nBufferSize);
		int 	getModuleType(
					char*			szModuleType,
					int				nBufferSize);
		int 	canConnect();
		int 	connect(
					char*			szHostName,
					int 			nHostPort);
		int 	isConnected();
		int 	read(
					unsigned char*	pBuffer,
					int				nReadLength,
					int				nTimeout);
		int 	write(
					unsigned char*	pBuffer,
					int				nWriteLength,
					int				nTimeout);
		void 	disconnect();
		int 	getSignalQuality(
					int 			&nRssi,
					int 			&nBer,
					int 			&nEcLo);
		void 	initGPSHL();
		void 	startGPSHL(
					char* frame);
		void 	emergencyCall(
					char* number);
		

	private:
		int 	readAtTCPresponse(
					char* 			szLine,
					int				nLineBufSize,
					unsigned long	ulReadDelay);
		int 	readATresponseLine(
					char* 			szLine,
					int 			nLineBufSize,
					char*			szResponseFilter,
					unsigned long	ulDelay);
		int 	readATresponseLine(
					char*			aLine[],
					int				nMaxLine,
					unsigned long	ulDelay); 
		int 	sendATcmd(
					char * 			szCmd,
					char* 			szResponse,
					int 			nResponseBufSize,
					char * 			szResponseFilter,
					unsigned long	ulWaitDelay=2000);
		int 	sendATcmd(
					char * 			szCmd,
					char* 			aLine[],
					int 			nMaxLine,
					unsigned long	ulWaitDelay=2000);
		int 	sendATbyte(
					char * 			pByte,
					int 			nLength,
					char* 			szResponse,
					int 			nResponseBufSize,
					char * 			szResponseFilter,
					int 			nWaitDelay=1000);

		char*   findPattern(
					char*			cBuffer,
					unsigned long	ulBufferSize,
					char*			szPattern);
					
		int 	findGLLFrame(
					char* frame);

		int							_nTCPsession;
		int							_nApnIndex;
		HardwareSerial*				_pModule;
		int							_nSimReady;
		int							_nSimPin;
		char						_szModuleType[16];
		
};

#endif

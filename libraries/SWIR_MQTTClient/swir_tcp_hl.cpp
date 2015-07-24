

/*******************************************************************************
 * TCP Socket Layer adapter for Sierra Wireless' HL Serie Modules
 * V0.8
 * Nhon Chu - May 2015
 *******************************************************************************/

#define EOF_PATTERN 	"--EOF--Pattern--"
#define EOF_OK_PATTERN 	"--EOF--Pattern--\r\nOK"
#define CONNECT_PATTERN	"CONNECT\r\n"
#define KCNXCFG_PATTERN	"+KCNXCFG: "
#define CGREG_PATTERN	"+CGREG: "
#define CLOSE_GPS		"+++"

#define MAX_SIZE1024	1024
#define MAX_SIZE512		512
#define MAX_SIZE256		256
#define MAX_SIZE128		128
#define MAX_SIZE64		64

#include "arduino.h"

#include "swir_tcp_hl.h"
#include "swir_debug.h"

SWIR_TCP_HL::SWIR_TCP_HL()
{
	_nTCPsession = 0;
	_nApnIndex = -1;
	_nSimReady = 0;
	_nSimPin = -1;
	memset(_szModuleType, 0, sizeof(_szModuleType));
}

void SWIR_TCP_HL::setSerialObject(
		HardwareSerial					&module)
{
	_pModule = &module;
}

void SWIR_TCP_HL::initGPSHL()
{
	char szResponse[MAX_SIZE64];
	
	if (0 == sendATcmd("AT+GPSINIT=41", szResponse, sizeof(szResponse), "OK"))
	{
	#ifdef _SWIR_OUTPUT_
			SWIR_TRACE(F("AT+GPSINIT=41 OK!!"));
	#endif
	}
	else
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("AT+GPSINIT=41 KO!!"));
	#endif
	}
	
	if (0 == sendATcmd("AT+GPSSTART=1", szResponse, sizeof(szResponse), "OK"))
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("AT+GPSSTART=1 OK!!"));
	#endif
	}
	else
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("AT+GPSSTART=1 KO!!"));
	#endif
	}
		
}


void SWIR_TCP_HL::startGPSHL(char* frame)
{
	int nRet = 0;
	
	char* gllTest = "$GPGLL,5106.94086,N,01701.51680,E,123204.00,A,A*63";
		
	char szResponse[MAX_SIZE1024];
	
	if (0 == sendATcmd("AT+GPSNMEA=1,1,10", szResponse, sizeof(szResponse), "CONNECT"))
	{
	#ifdef _SWIR_OUTPUT_
			SWIR_TRACE(F("AT+GPSNMEA=1,1 OK!!"));
	#endif
	}
	else
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("AT+GPSNMEA=1,1 KO!!"));
	#endif
	}

	//Detect if the GNGLL
	findGLLFrame(frame);
	
	//delay(10000);
	
	if (0 == sendATcmd("+++", szResponse, sizeof(szResponse), "OK"))
	{
	#ifdef _SWIR_OUTPUT_
			SWIR_TRACE(F("+++ OK!!"));
	#endif
	}
	else
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("+++ KO!!"));
	#endif
	}
	
	delay(3000);
}

int SWIR_TCP_HL::isSimReady()
{
	char szResponse[MAX_SIZE64];
	
	if (0 == sendATcmd("AT+CPIN?", szResponse, sizeof(szResponse), "+CPIN: READY"))
	{
		_nSimReady = 1;
#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("HL_TCP::setSimPIN - READY!!"));
#endif
	}
	else
	{
		_nSimReady = 0;
		
	}
	
	return _nSimReady;
}

int SWIR_TCP_HL::setSimPIN(int nPinCode)
{
	_nSimPin = nPinCode;
	
	if (!isSimReady())
	{
		if (_nSimReady > -1)
		{
			char szATcmd[24];
			char szResponse[MAX_SIZE64];

			sprintf(szATcmd, "AT+CPIN=\"%04d\"", nPinCode);

			sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK");
			
			isSimReady();
		}
	}
	
	return _nSimReady;
}

int SWIR_TCP_HL::readAtTCPresponse(char* szLine, int nLineBufSize, unsigned long ulReadDelay)
{
	int 	nRet = 1;
	int 	index = 0;
	unsigned long ulExpire = ulReadDelay + millis();
	char*   pszPattern;
 
	pszPattern = (char *) malloc(strlen(EOF_OK_PATTERN)+1);
	strcpy(pszPattern, EOF_OK_PATTERN);

	int     nMatchCount = strlen(pszPattern);
	int     nSearchIndex = 0;
	int     bFound = 0;
	int     nTCPcontentEndIndex = -1;

	memset(szLine, 0, nLineBufSize);
				
	do
	{
		while (_pModule->available())
		{
			char buf;

			buf = _pModule->read();

			szLine[index++] = buf;

			if (buf == pszPattern[nSearchIndex])
			{
				nSearchIndex++;
				if (nSearchIndex == nMatchCount)
				{
					bFound = 1;
					nTCPcontentEndIndex = index - nMatchCount;
					//SWIR_TRACE(F("Found EOF"));
				}
			}
			else
			{
				nSearchIndex = 0;
			}
		}

		if (bFound)
		{
			break;
		}

	} while (ulExpire > millis());

#ifdef _VERBOSE_DEBUG_        
	if (bFound)
	{
		SWIR_TRACE(F("Found EOF"));
	}
	SWIR_TRACE(F("readAtTCPresponseLine: %d bytes\n"), index);
	SWIR_TRACE(F("readAtTCPresponseLine: %s\n"), szLine);
#endif

	nRet = 0;

	free(pszPattern);
								
	return nRet;
}

int SWIR_TCP_HL::findGLLFrame(char* frame)
{
	bool gllFrameFound = false;
	char* test;
	unsigned long	ulExpire = millis() + 5000;
	
	do
	{
		//SWIR_TRACE(F("_pModule->available()1\n"));
		if (_pModule->available())
		{
			
			String sStr;
			sStr = _pModule->readStringUntil('\n');
			int nLen = sStr.length();
			if (nLen > 1)
			{
				test = (char *) malloc(nLen+1);
				sStr.toCharArray(test, nLen);
				//SWIR_TRACE(F("_pModule->available()3 %s\n"),test);
				if(strstr(test,"$GNGLL") != NULL)
				{
					//SWIR_TRACE(F("_pModule->available()4\n"));
	
					strcpy(frame,test);
					gllFrameFound = true;
				
				}
				free(test);
			}
		}            

	} while (gllFrameFound != true || ulExpire < millis());
	return 0;
}

void 	SWIR_TCP_HL::emergencyCall(
					char* number)
{
	char szATcmd[24];
	char szResponse[MAX_SIZE64];

	sprintf(szATcmd, "ATD%s;", number);
						
	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK"))
	{
	#ifdef _SWIR_OUTPUT_
			SWIR_TRACE(F("+++ OK!!"));
	#endif
	}
	else
	{
	#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("+++ KO!!"));
	#endif
	}			
}

int SWIR_TCP_HL::readATresponseLine(char* szLine, int nLineBufSize, char* szResponseFilter, unsigned long ulDelay)
{
	#define			NB_LINE	10
	int 			nbLine = 0;
	int 			index = 0;
	char*			aLine[NB_LINE];	//reserved 10 lines
	unsigned long	ulExpire = millis() + ulDelay;
	char * 			pszSubstr = NULL;

	memset(szLine, 0, nLineBufSize);

	do
	{
		if (_pModule->available())
		{
			String sStr;
			sStr = _pModule->readStringUntil('\n');
			int nLen = sStr.length();
			if (nLen > 1)
			{
				aLine[nbLine] = (char *) malloc(nLen+1);
				sStr.toCharArray(aLine[nbLine], nLen);
				aLine[nbLine][nLen] = 0;

				pszSubstr = strstr(aLine[nbLine], "OK");
				if (pszSubstr != NULL)
				{
					nbLine++;
					#ifdef _VERBOSE_DEBUG_
					SWIR_TRACE(F("Found OK"));
					#endif
					break;	
				}

				nbLine++;
				
			}
		}            
		if (nbLine >= NB_LINE)
		{
				break;
		}
	} while (ulExpire > millis());

#ifdef _VERBOSE_DEBUG_
	SWIR_TRACE(F("readATresponseLine: %d line(s)\n"), nbLine);
#endif

	int i;

	int nRet = 3;

	for (i=0; i<nbLine; i++)
	{
		#ifdef _VERBOSE_DEBUG_
		SWIR_TRACE(F("line[%d]: %s\n"), i, aLine[i]);
		#endif

		if (szResponseFilter == NULL)
		{
			//Not filtering response
			strcpy(szLine, aLine[i]);
			nRet = 0;
		}
		else if (strlen(szResponseFilter) > 0)
		{
			//Filtering Response
			char * pszSubstr = NULL;

			pszSubstr = strstr(aLine[i], szResponseFilter);
			if (pszSubstr != NULL)
			{
				pszSubstr += strlen(szResponseFilter);
				while (isspace( *pszSubstr))	//trim heading
				{
					pszSubstr++;
				}
				char * pTemp = pszSubstr;
				while (pTemp < (aLine[i]+strlen(aLine[i])))		//trim ending
				{
					if (*pTemp == '\n')	//remove cariage return
					{
						*pTemp = 0;	//zero terminate string
						break;
					}
					pTemp++;
				}
				#ifdef _VERBOSE_DEBUG_
				SWIR_TRACE(F("Filtered response: %s\n"), pszSubstr);
				#endif
				strcpy(szLine, pszSubstr);
				nRet = 0;
			}
		}
		else
		{
			//Not filtering response
			strcpy(szLine, aLine[i]);
			nRet = 0;
		}

		free(aLine[i]);
	}

	return nRet;
}

int SWIR_TCP_HL::readATresponseLine(char* aLine[], int nMaxLine, unsigned long ulDelay)
{
	int 			nbLine = 0;
	int 			index = 0;
	unsigned long	ulExpire = millis() + ulDelay;
	char * 			pszSubstr = NULL;
				
	memset(aLine, 0, nMaxLine);

	do
	{
		if (_pModule->available())
		{
			String sStr;
			sStr = _pModule->readStringUntil('\n');
			int nLen = sStr.length();
			if (nLen > 1)
			{
				aLine[nbLine] = (char *) malloc(nLen+1);
				sStr.toCharArray(aLine[nbLine], nLen);
				aLine[nbLine][nLen] = 0;
				
				pszSubstr = strstr(aLine[nbLine], "OK");
				if (pszSubstr != NULL)
				{
					nbLine++;
					#ifdef _VERBOSE_DEBUG_
					SWIR_TRACE(F("Found OK"));
					#endif
					break;	
				}
				
				nbLine++;
			}
		}            
		if (nbLine >= nMaxLine)
		{
				break;
		}
	} while (ulExpire > millis());

#ifdef _VERBOSE_DEBUG_
	SWIR_TRACE(F("readATresponseLine2: %d line(s)\n"), nbLine);
#endif

	return nbLine;
}

int SWIR_TCP_HL::sendATcmd(char * szCmd, char* szResponse, int nResponseBufSize, char * szResponseFilter, unsigned long ulWaitDelay)
{
	int 	nRet = 0;

	memset(szResponse, 0, nResponseBufSize);

#ifdef _VERBOSE_DEBUG_
	SWIR_TRACE(F("sendATcmd (%s) - %d..."), szCmd, ulWaitDelay);
#else
	#ifdef _SWIR_OUTPUT_
	SWIR_TRACE(F("sendATcmd (%s)..."), szCmd);
	#endif
#endif

	SWIR_AVAILABLE_SRAM
	if(strcmp(szCmd, CLOSE_GPS) != 0){
		_pModule->println(szCmd);
	}
	else
	{
		_pModule->print(szCmd);
	}

	if (strcmp(szResponseFilter, CONNECT_PATTERN) == 0)
	{
		nRet = readAtTCPresponse(szResponse, nResponseBufSize, ulWaitDelay);
	}
	else
	{
		nRet = readATresponseLine(szResponse, nResponseBufSize, szResponseFilter, ulWaitDelay);
	}

#ifdef _SWIR_OUTPUT_
	if (nRet == 0)
	{
		SWIR_TRACE(F("...sendATcmd OK"));
	}
	else
	{
		SWIR_TRACE(F("...sendATcmd Fails"));
	}
#endif
	return nRet;
}

int SWIR_TCP_HL::sendATcmd(char * szCmd, char* aLine[], int nMaxLine, unsigned long ulWaitDelay)
{
	int 	nRet = 0;

#ifdef _SWIR_OUTPUT_
	SWIR_TRACE(F("sendATcmd2 (%s) - %d..."), szCmd, ulWaitDelay);
#endif

	SWIR_AVAILABLE_SRAM

	_pModule->println(szCmd);

	nRet = readATresponseLine(aLine, nMaxLine, ulWaitDelay);

#ifdef _SWIR_OUTPUT_
	if (nRet > 0)
	{
		SWIR_TRACE(F("...sendATcmd OK"));
	}
	else
	{
		SWIR_TRACE(F("...sendATcmd Fails"));
	}
#endif
	return nRet;
}

int SWIR_TCP_HL::sendATbyte(char * szByte, int nLength, char* szResponse, int nResponseBufSize, char * szResponseFilter, int nWaitDelay)
{
	int 	nRet = 0;

	memset(szResponse, 0, nResponseBufSize);

#ifdef _SWIR_OUTPUT_
	SWIR_TRACE(F("sending %d BYTES to module..."), nLength);
#endif

	if (_pModule->write(szByte, nLength) < nLength)
	{
#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("...send BYTES Fails"));
#endif
		nRet = 2;	//failed to write to port
	}
	else
	{
#ifdef _SWIR_OUTPUT_
		SWIR_TRACE(F("OK, now reading response..."));
#endif

#if 0
		delay(500);
#endif
		nRet = readATresponseLine(szResponse, nResponseBufSize, szResponseFilter, nWaitDelay);

	}
#ifdef _SWIR_OUTPUT_
	if (nRet == 0)
	{
		SWIR_TRACE(F("...send BYTES OK"));
	}
	else
	{
		SWIR_TRACE(F("...send BYTES Fails"));
	}
#endif
	return nRet;
}

int     SWIR_TCP_HL::getIMEI(
						char*                   szIMEI,
						int                     nBufferSize)
{
	char    szCmd[MAX_SIZE64];
	char*   aLine[NB_LINE];  //let's read up to 10 lines

	strcpy(szCmd, "AT+CGSN");
	int nNbLine = sendATcmd(szCmd, aLine, NB_LINE);
	
	SWIR_AVAILABLE_SRAM

	char*  sLine;
 
	memset(szIMEI, 0, sizeof(szIMEI));
	
	for (int i=0; i<nNbLine; i++)
	{
		sLine = aLine[i];
#ifdef _VERBOSE_DEBUG_
		SWIR_TRACE(F("getIMEI, line[%d]: %s\n"), i, aLine[i]);
#endif
		int nLen = strlen(sLine);
		if (nLen != 15)
		{
			continue;
		}
		for (int k=0; k<nLen; k++)
		{
			if (sLine[k] < '0' || sLine[k] > '9')
			{
				continue;
			}
		}
		strcpy(szIMEI, sLine);
		free(aLine[i]);
	}
				
	return (strlen(szIMEI) > 0 ? 0 : 1);
}

int      SWIR_TCP_HL::getSN(
						char*                   szSN,
						int                     nBufferSize)
{
	char* 	szBuffer;
	char 	szResponse[MAX_SIZE64];
	int 	nRet = 0;

	SWIR_AVAILABLE_SRAM

	memset(szSN, 0, sizeof(szSN));
				
	if (0 == sendATcmd("AT+KGSN=3", szResponse, sizeof(szResponse), "+KGSN: "))
	{
		strcpy(szSN, szResponse);
	}
				
	return (strlen(szSN) > 0 ? 0 : 1);
}

int     SWIR_TCP_HL::getModuleType(
						char*                   szModuleType,
						int                     nBufferSize)
{
	if (strlen(_szModuleType) == 0)
	{
		char    szCmd[MAX_SIZE64];
		char*   aLine[NB_LINE];  //let's read up to 10 lines
		strcpy(szCmd, "ATI");
		int nNbLine = sendATcmd(szCmd, aLine, NB_LINE);
		
		char*  sLine;
	 
	 	SWIR_AVAILABLE_SRAM

		memset(szModuleType, 0, sizeof(szModuleType));
		
		for (int i=0; i<nNbLine; i++)
		{
			sLine = aLine[i];
	#ifdef _VERBOSE_DEBUG_
			SWIR_TRACE(F("getModuleType, line[%d]: %s\n"), i, aLine[i]);
	#endif
			char * pszSubstr = NULL;

			pszSubstr = strstr(sLine, szCmd);
			if (pszSubstr != NULL)
			{
				continue;
			}
			pszSubstr = strstr(sLine, "OK");
			if (pszSubstr != NULL)
			{
				continue;
			}
			strcpy(szModuleType, sLine);
			strcpy(_szModuleType, sLine);
			free(aLine[i]);
		}
					
		return (strlen(szModuleType) > 0 ? 0 : 1);
	}
	else
	{
		strcpy(szModuleType, _szModuleType);
		return 1;
	}
}

int	SWIR_TCP_HL::setAPN(
					char*			szAPN,
					char*			szAPNlogin,
					char*			szAPNpassword)
{
	char 	szResponse[MAX_SIZE64];
	char 	szExpectedResponse[] = "OK";
	char    szCmd[MAX_SIZE64];

	memset(szCmd, 0, sizeof(szCmd));
	
	SWIR_AVAILABLE_SRAM

	char*    aLine[16];  //let's read up to 16 lines
	strcpy(szCmd, "AT+KCNXCFG?");
	int nNbLine = sendATcmd(szCmd, aLine, 16);
	int    nApnIndex = -1;
	int    nFirstAvailableIndex = -1;
	
	char*  sLine;
	
	for (int i=0; i<nNbLine; i++)
	{
		sLine = aLine[i];
#ifdef _VERBOSE_DEBUG_
		SWIR_TRACE(F("setAPN, line[%d]: %s\n"), i, aLine[i]);
#endif

		char * pszSubstr = NULL;

		pszSubstr = strstr(sLine, KCNXCFG_PATTERN);
		if (pszSubstr != NULL)
		{
			char szNum[2];
			if (strstr(sLine, szAPN) != NULL)
			{
				 szNum[0] = sLine[strlen(KCNXCFG_PATTERN)];
				 szNum[1] = 0;
				 nApnIndex = atoi(szNum);
			}
			if (nFirstAvailableIndex == -1)
			{
				if (strstr(sLine, "APN") != NULL)
				{
				 szNum[0] = sLine[strlen(KCNXCFG_PATTERN)];
				 szNum[1] = 0;
				 nFirstAvailableIndex = atoi(szNum);
				}
			 }
		}
		
		free(aLine[i]);
	}
				
	if (nApnIndex > -1)
	{
		_nApnIndex = nApnIndex;  //APN already exist
		//SWIR_TRACE(F("APN already exists"));
	}
	else if (nFirstAvailableIndex > -1)
	{
		_nApnIndex = nFirstAvailableIndex;
	}
	else
	{
		//handling HL8 case
		_nApnIndex = 1;	
	}

	if (_nApnIndex > -1)
	{
		sprintf(szCmd, "AT+KCNXCFG=%d,\"GPRS\",\"%s\",\"%s\",\"%s\"", _nApnIndex, szAPN, szAPNlogin, szAPNpassword);

		//SWIR_TRACE(szCmd);
					
		if (0 == sendATcmd(szCmd, szResponse, sizeof(szResponse), szExpectedResponse))
		{
			 return 0;	//OK
		}
	}
				
	_nApnIndex = -1;
	return 1;	//fail
}


int SWIR_TCP_HL::read(unsigned char* buffer, int len, int timeout_ms)
{
	int  nRet = 0;
	char szResponse[MAX_SIZE256];
	
	char 	szATcmd[16];
	sprintf(szATcmd, "AT+KTCPRCV=%d,%d", _nTCPsession, len);
	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), CONNECT_PATTERN, 10000))
	{
		char * pszConnectPattern = NULL;
		char*  pszEOFpattern = NULL;

		pszConnectPattern = strstr(szResponse, CONNECT_PATTERN);
		if (pszConnectPattern != NULL)
		{
			pszConnectPattern += strlen(CONNECT_PATTERN);
			#ifdef _VERBOSE_DEBUG_
			SWIR_TRACE(pszConnectPattern);
			#endif

			pszEOFpattern = findPattern(szResponse, sizeof(szResponse), EOF_PATTERN);
			#ifdef _VERBOSE_DEBUG_
			SWIR_TRACE(F("EOF pattern is: %s"), pszEOFpattern);
			#endif
															
			if (pszEOFpattern > pszConnectPattern)
			{
				#ifdef _SWIR_OUTPUT_
				SWIR_TRACE(F("TCP read OK %d bytes"), pszEOFpattern - pszConnectPattern);
				#endif

				memcpy(buffer, pszConnectPattern, len);

				#ifdef _VERBOSE_DEBUG_
				if (len > 2)
				{
					SWIR_TRACE(F("TCP content:\r\n%s"), pszConnectPattern+2);
				}
				#endif
				nRet = len;
			}
		}
	}

	return nRet;
}

char* SWIR_TCP_HL::findPattern(char* cBuffer, unsigned long ulBufferSize, char* szPattern)
{
	char*    pstrMatch = NULL;
	unsigned long i;
	int nMatchCount = strlen(szPattern);
	int nSearchIndex = 0;
	int bFound = 0;
	
	for (i=0; i<ulBufferSize; i++)
	{
		 if (cBuffer[i] == szPattern[nSearchIndex])
		 {
				nSearchIndex++;
				if (nSearchIndex == nMatchCount)
				{
					bFound = 1;
					break;
				}
		 }
		 else
		 {
			 nSearchIndex = 0;
		 }
	}
	
	if (bFound)
	{
		pstrMatch = cBuffer + (i - nMatchCount + 1);
	}
	
	return pstrMatch;
}

int SWIR_TCP_HL::write(unsigned char* buffer, int len, int timeout_ms)
{
	char 	szATcmd[16];
	char* 	szBuffer;
	char 	szResponse[MAX_SIZE64];
	int 	nRet = 0;


	sprintf(szATcmd, "AT+KTCPSND=%d,%d", _nTCPsession, len);

	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), "CONNECT"))
	{
		int nSize = len + strlen(EOF_PATTERN);
		szBuffer = (char *) malloc(nSize);
		memcpy(szBuffer, buffer, len);
		memcpy(szBuffer+len, EOF_PATTERN, strlen(EOF_PATTERN));
		//szBuffer[nSize-1] = 0;

		if (0 == sendATbyte(szBuffer, nSize, szResponse, sizeof(szResponse), "OK", 3000))
		{
			nRet = len;
		}

		free(szBuffer);
	}

	return nRet;
}


void SWIR_TCP_HL::disconnect()
{
	char 	szATcmd[20];
	char 	szResponse[MAX_SIZE64];

	sprintf(szATcmd, "AT+KTCPCLOSE=%d,1", _nTCPsession);

	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK"))
	{
		sprintf(szATcmd, "AT+KTCPDEL=%d", _nTCPsession);
		if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK"))
		{
			_nTCPsession = 0;
		}
	}
}

int SWIR_TCP_HL::canConnect()
{
	char 	szATcmd[16];
	char 	szResponse[MAX_SIZE64];
	
	sprintf(szATcmd, "AT+CGREG?");
	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), CGREG_PATTERN))
	{
		char * pszState = NULL;

		SWIR_AVAILABLE_SRAM

		pszState = strstr(szResponse, ",");
		if (pszState != NULL)
		{
			pszState++;
			if (*pszState == '1' || *pszState == '5')
			{
				return 1;	//ready to connect. CGREG equals to 1 (home) nor 5 (roaming)
			}
		}
	}

	#ifdef _SWIR_OUTPUT_
	SWIR_TRACE(F("Not ready to make data call..."));
	#endif

	return 0;	//not attached to GPRS yet
}

int SWIR_TCP_HL::connect(char* addr, int port)
{
	char 	szATcmd[MAX_SIZE128];
	char 	szResponse[MAX_SIZE64];
	char 	szExpectedResponse[] = "+KTCPCFG: ";

	if (_nTCPsession > 0)
	{
		disconnect();
	}

	sprintf(szATcmd, "AT+KTCPCFG=%d,0,\"%s\",%d", _nApnIndex, addr, port);

	if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), szExpectedResponse))
	{
		_nTCPsession = atoi(szResponse);
		//SWIR_TRACE(F("KTCP-ID= %d\n"), _nTCPsession);
#if 0
		sprintf(szATcmd, "AT+KCNXTIMER=%d,60,3,120", _nTCPsession);
		sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK", 5000);
#endif
		sprintf(szATcmd, "AT+KTCPCNX=%d", _nTCPsession);
		if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), "OK", 10000))
		{
			return 0;	//OK
		}
		else
		{
			disconnect();
		}
	}

	return 1;	//fail
}

int SWIR_TCP_HL::isConnected()
{
	SWIR_AVAILABLE_SRAM
	return (_nTCPsession > 0 ? 1 : 0);
}

int SWIR_TCP_HL::getSignalQuality(int &nRssi, int &nBer, int &nEcLo)
{
	if (getModuleType(_szModuleType, sizeof(_szModuleType)))
	{
		char 	szATcmd[16];
		char 	szFilter[16];
		char 	szResponse[MAX_SIZE64];
		char *	pszSearch = NULL;
		char*	pszStr = NULL;

		pszSearch = strstr(_szModuleType, "HL6");
		if (pszSearch != NULL)
		{	//HL6 module
			sprintf(szATcmd, "AT+CSQ");
			sprintf(szFilter, "+CSQ: ");
		}
		else
		{
			sprintf(szATcmd, "AT$CSQ");
			sprintf(szFilter, "$CSQ: ");
		}
		
		if (0 == sendATcmd(szATcmd, szResponse, sizeof(szResponse), szFilter))
		{
			pszStr = szResponse;
			pszSearch = strstr(pszStr, ",");
			if (pszSearch != NULL)
			{
				*pszSearch = 0;
				nRssi = atoi(pszStr);

				pszStr = pszSearch + 1;
				pszSearch = strstr(pszStr, ",");
				if (pszSearch != NULL)
				{
					*pszSearch = 0;
					nBer = atoi(pszStr);

					pszStr = pszSearch + 1;
					nEcLo = atoi(pszStr);
				}
				else
				{
					nBer = atoi(pszStr);
					nEcLo = -99;
				}

				return 1;
			}
		}
	}

	return 0;
}

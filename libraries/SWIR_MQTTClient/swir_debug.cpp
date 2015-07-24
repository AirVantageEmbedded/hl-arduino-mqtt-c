/*
 * Helper class for debug tracing on a selected pair of ports
 *
 * Nhon Chu - May 2015
 */

#include "arduino.h"

#include <SoftwareSerial.h>

#include "swir_debug.h"

#ifdef _SWIR_OUTPUT_

#define TRACE_BUFFER_SIZE	256

SoftwareSerial* 	swirOutput::_pSerial = NULL;

swirOutput::swirOutput()
{
	_pSerial = NULL;
}

swirOutput::swirOutput(int nRXport, int nTXport, long nBaudrate)
{
	_pSerial = NULL;

	if (nRXport > 0 && nTXport > 0 && nBaudrate > 0)
	{
		_pSerial = new SoftwareSerial(nRXport, nTXport);
		_pSerial->begin(nBaudrate);
		_pSerial->println(F("\r\nDebug port initialized!"));
	}
}

void swirOutput::setPort(int nRXport, int nTXport, long nBaudrate)
{
	if (nRXport > 0 && nTXport > 0 && nBaudrate > 0)
	{
		if (_pSerial != NULL)
		{
			_pSerial->end();
			delete _pSerial;
		}
		_pSerial = new SoftwareSerial(nRXport, nTXport);
		_pSerial->begin(nBaudrate);
		trace(F("\r\nDebug port initialized on Pins %d(RX) and %d(TX) !"), nRXport, nTXport);
	}

}

void swirOutput::trace(char* szTrace, ...)
{
	if (_pSerial != NULL)
	{
		char 	szBuf[TRACE_BUFFER_SIZE];
		va_list args;

		va_start(args, szTrace);
		vsnprintf(szBuf, sizeof(szBuf), szTrace, args);
		va_end(args);

		_pSerial->println(szBuf);
	}
}

void swirOutput::trace(const __FlashStringHelper * szTrace, ... )
{
	if (_pSerial != NULL)
	{
		char	szBuf[TRACE_BUFFER_SIZE];
		va_list args;

		va_start (args, szTrace);
		#ifdef __AVR__
		vsnprintf_P(szBuf, sizeof(szBuf), (const char *)szTrace, args); // progmem for AVR
		#else
		vsnprintf(szBuf, sizeof(szBuf), (const char *)szTrace, args); // for the rest of the world
		#endif
		va_end(args);

		_pSerial->println(szBuf);
	}
}

int swirOutput::availableSram()
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

#endif
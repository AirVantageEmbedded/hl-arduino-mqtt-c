/*
 * Helper class for debug tracing on a selected pair of ports
 *
 * Nhon Chu - May 2015
 */

#ifndef SWIR_OUTPUT_H
#define SWIR_OUTPUT_H

#define _SWIR_OUTPUT_		//disabling this flag will remove TRACING capabilty

#ifdef _SWIR_OUTPUT_

#include "SoftwareSerial.h"

#define SWIR_TRACE(...)		swirOutput::trace(__VA_ARGS__);

#if 0
	#define SWIR_AVAILABLE_SRAM	swirOutput::trace(F("Available SRAM: %d"), swirOutput::availableSram());
#else
	#define SWIR_AVAILABLE_SRAM
#endif
 
class swirOutput
{
	public:
		swirOutput();
		swirOutput(int nRXport, int nTXport, long nBaudrate);

		static void setPort(int nRXport, int nTXport, long nBaudrate);

		static void trace(char* szTrace, ...);

		static void trace(const __FlashStringHelper * szTrace, ... );
		
		static int 	availableSram();
	protected:
		static SoftwareSerial*		_pSerial;
};

#else
#define SWIR_TRACE(...)
#define SWIR_AVAILABLE_SRAM
#endif //_SWIR_OUTPUT_

#endif // SWIR_OUTPUT_H
